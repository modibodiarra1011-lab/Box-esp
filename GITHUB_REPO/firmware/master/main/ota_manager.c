#include "ota_manager.h"
#include "lab_config.h"
#include "storage.h"
#include "notifications.h"
#include "led_status.h"
#include "worker_pool.h"
#include "wifi_lab.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>

static const char *TAG = "ota";
static char token_s[17], path_s[220];
static int64_t token_expiry_ms = 0;
static char new_url[220], new_sha[65], new_ver[48], new_notes[256];
static bool available = false, approved = false;
static int64_t last_notify_ms = 0;

void ota_register_local_file(const char *t, const char *p)
{
    if (!t || !p) return;
    strlcpy(token_s, t, sizeof(token_s));
    strlcpy(path_s, p, sizeof(path_s));
    token_expiry_ms = esp_timer_get_time() / 1000 + 5LL * 60LL * 1000LL;
}

const char *ota_local_path_for_token(const char *t)
{
    return (t && token_s[0] && strcmp(t, token_s) == 0 && (esp_timer_get_time() / 1000) < token_expiry_ms) ? path_s : NULL;
}

static bool read_text_https(const char *url, char *out, size_t cap)
{
    esp_http_client_config_t cfg = {
        .url = url,
        .timeout_ms = 15000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    esp_http_client_handle_t h = esp_http_client_init(&cfg);
    if (!h) return false;

    esp_err_t r = esp_http_client_open(h, 0);
    if (r != ESP_OK) { esp_http_client_cleanup(h); return false; }
    int content_len = esp_http_client_fetch_headers(h);
    int code = esp_http_client_get_status_code(h);
    if (code != 200 || (content_len >= 0 && (size_t)content_len >= cap)) {
        esp_http_client_close(h);
        esp_http_client_cleanup(h);
        return false;
    }

    size_t used = 0;
    while (used + 1 < cap) {
        int n = esp_http_client_read(h, out + used, (int)(cap - used - 1));
        if (n < 0) { esp_http_client_close(h); esp_http_client_cleanup(h); return false; }
        if (n == 0) break;
        used += (size_t)n;
    }
    out[used] = 0;
    esp_http_client_close(h);
    esp_http_client_cleanup(h);
    return true;
}

static int vercmp(const char *a, const char *b)
{
    int x1 = 0, y1 = 0, z1 = 0, x2 = 0, y2 = 0, z2 = 0;
    if (!a || !b) return -1;
    sscanf(a, "%d.%d.%d", &x1, &y1, &z1);
    sscanf(b, "%d.%d.%d", &x2, &y2, &z2);
    if (x1 != x2) return x1 > x2 ? 1 : -1;
    if (y1 != y2) return y1 > y2 ? 1 : -1;
    if (z1 != z2) return z1 > z2 ? 1 : -1;
    return 0;
}

void ota_check_now(char *out, size_t cap)
{
    if (!out || cap == 0) return;
    if (!g_lab_cfg.update_manifest[0]) {
        snprintf(out, cap, "{\"available\":false,\"reason\":\"manifest non configure\"}");
        return;
    }
    if (!wifi_lab_sta_connected()) {
        snprintf(out, cap, "{\"available\":false,\"reason\":\"internet indisponible\"}");
        return;
    }

    char b[2400] = {0};
    if (!read_text_https(g_lab_cfg.update_manifest, b, sizeof(b))) {
        snprintf(out, cap, "{\"available\":false,\"reason\":\"manifest indisponible\"}");
        return;
    }

    cJSON *j = cJSON_Parse(b);
    if (!j) {
        snprintf(out, cap, "{\"available\":false,\"reason\":\"manifest invalide\"}");
        return;
    }

    cJSON *v = cJSON_GetObjectItem(j, "version");
    cJSON *u = cJSON_GetObjectItem(j, "master_url");
    cJSON *s = cJSON_GetObjectItem(j, "master_sha256");
    cJSON *n = cJSON_GetObjectItem(j, "notes");
    if (!cJSON_IsString(v) || !cJSON_IsString(u) || !cJSON_IsString(s) ||
        vercmp(v->valuestring, LAB_VERSION) <= 0 ||
        strncmp(u->valuestring, "https://", 8) != 0 || strlen(s->valuestring) != 64) {
        available = false;
        snprintf(out, cap, "{\"available\":false,\"current\":\"%s\"}", LAB_VERSION);
        cJSON_Delete(j);
        return;
    }

    strlcpy(new_ver, v->valuestring, sizeof(new_ver));
    strlcpy(new_url, u->valuestring, sizeof(new_url));
    strlcpy(new_sha, s->valuestring, sizeof(new_sha));
    strlcpy(new_notes, (n && cJSON_IsString(n)) ? n->valuestring : "", sizeof(new_notes));
    available = true;
    approved = false;
    int64_t now_ms = esp_timer_get_time() / 1000;
    if (now_ms - last_notify_ms > 60LL * 60LL * 1000LL) {
        notifications_send("ESP32 LAB : une nouvelle release est disponible. Consulte le dashboard avant approbation.");
        last_notify_ms = now_ms;
    }
    snprintf(out, cap, "{\"available\":true,\"version\":\"%s\",\"notes\":\"%s\"}", new_ver, new_notes);
    cJSON_Delete(j);
}

static esp_err_t download_to_sd(void)
{
    FILE *f = fopen("/sd/UPDATES/master.new.bin", "wb");
    if (!f) return ESP_FAIL;

    esp_http_client_config_t cfg = {
        .url = new_url,
        .timeout_ms = 30000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    esp_http_client_handle_t h = esp_http_client_init(&cfg);
    if (!h) { fclose(f); return ESP_FAIL; }

    esp_err_t r = esp_http_client_open(h, 0);
    if (r != ESP_OK) { esp_http_client_cleanup(h); fclose(f); return r; }
    int content_len = esp_http_client_fetch_headers(h);
    int code = esp_http_client_get_status_code(h);
    const esp_partition_t *p = esp_ota_get_next_update_partition(NULL);
    if (!p || code != 200 || (content_len > 0 && (size_t)content_len > p->size)) {
        esp_http_client_close(h);
        esp_http_client_cleanup(h);
        fclose(f);
        return ESP_ERR_INVALID_SIZE;
    }

    uint8_t buf[4096];
    size_t total = 0;
    while (1) {
        int n = esp_http_client_read(h, (char *)buf, sizeof(buf));
        if (n < 0) { r = ESP_FAIL; break; }
        if (n == 0) { r = ESP_OK; break; }
        if (total + (size_t)n > p->size) { r = ESP_ERR_INVALID_SIZE; break; }
        if (fwrite(buf, 1, (size_t)n, f) != (size_t)n) { r = ESP_FAIL; break; }
        total += (size_t)n;
    }
    esp_http_client_close(h);
    esp_http_client_cleanup(h);
    fclose(f);
    return r;
}

static esp_err_t apply_sd_image(void)
{
    const esp_partition_t *p = esp_ota_get_next_update_partition(NULL);
    if (!p) return ESP_ERR_NOT_FOUND;
    FILE *f = fopen("/sd/UPDATES/master.new.bin", "rb");
    if (!f) return ESP_FAIL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return ESP_FAIL; }
    long sz = ftell(f);
    if (sz <= 0 || (size_t)sz > p->size) { fclose(f); return ESP_ERR_INVALID_SIZE; }
    rewind(f);

    esp_ota_handle_t oh = 0;
    esp_err_t r = esp_ota_begin(p, (size_t)sz, &oh);
    if (r != ESP_OK) { fclose(f); return r; }

    uint8_t buf[4096];
    size_t nread;
    while ((nread = fread(buf, 1, sizeof(buf), f)) > 0) {
        r = esp_ota_write(oh, buf, nread);
        if (r != ESP_OK) break;
    }
    fclose(f);
    if (r != ESP_OK) { esp_ota_abort(oh); return r; }
    r = esp_ota_end(oh);
    if (r != ESP_OK) return r;
    return esp_ota_set_boot_partition(p);
}

static esp_err_t apply_update(void)
{
    esp_err_t r = download_to_sd();
    if (r != ESP_OK) return r;

    char got[65] = {0};
    r = storage_sha256_file("/sd/UPDATES/master.new.bin", got);
    if (r != ESP_OK || strcasecmp(got, new_sha) != 0) {
        ESP_LOGE(TAG, "SHA-256 mismatch got=%s expected=%s", got, new_sha);
        return ESP_ERR_INVALID_CRC;
    }
    return apply_sd_image();
}

static void task_apply(void *arg)
{
    (void)arg;
    led_status_mode("update");
    esp_err_t r = apply_update();
    char msg[220];
    if (r == ESP_OK) {
        snprintf(msg, sizeof(msg), "ESP32 LAB : mise a jour %s installee, redemarrage.", new_ver);
        notifications_send(msg);
        vTaskDelay(pdMS_TO_TICKS(700));
        esp_restart();
    }
    snprintf(msg, sizeof(msg), "ESP32 LAB : echec mise a jour %s (%s).", new_ver, esp_err_to_name(r));
    notifications_send(msg);
    ESP_LOGE(TAG, "%s", msg);
    led_status_mode("error");
    approved = false;
    vTaskDelete(NULL);
}

void ota_approve(char *out, size_t cap)
{
    if (!out || cap == 0) return;
    if (!available) {
        snprintf(out, cap, "{\"ok\":false,\"reason\":\"aucune mise a jour valide\"}");
        return;
    }
    if (approved) {
        snprintf(out, cap, "{\"ok\":false,\"reason\":\"deja en cours\"}");
        return;
    }
    approved = true;
    if (xTaskCreate(task_apply, "ota_apply", 8192, NULL, 7, NULL) != pdPASS) {
        approved = false;
        snprintf(out, cap, "{\"ok\":false,\"reason\":\"task impossible\"}");
        return;
    }
    snprintf(out, cap, "{\"ok\":true,\"version\":\"%s\"}", new_ver);
}

static void ota_check_task(void *arg)
{
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(15000));
    for (;;) {
        if (g_lab_cfg.auto_updates && wifi_lab_sta_connected()) {
            char out[400];
            ota_check_now(out, sizeof(out));
        }
        vTaskDelay(pdMS_TO_TICKS(UPDATE_INTERVAL_MS));
    }
}

void ota_manager_start(void)
{
    xTaskCreate(ota_check_task, "ota_check", 4096, NULL, 2, NULL);
}
