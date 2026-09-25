#include "worker_pool.h"
#include "lab_config.h"
#include "wifi_lab.h"
#include "storage.h"
#include "led_status.h"
#include "ota_manager.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "esp_http_client.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static const char *TAG = "workers";
static worker_info_t w[WORKER_MAX];
static int s_sock = -1;

size_t worker_pool_count(void) { size_t n = 0; for (int i = 0; i < WORKER_MAX; i++) if (w[i].seen) n++; return n; }
const worker_info_t *worker_pool_get(size_t i) { size_t n = 0; for (int k = 0; k < WORKER_MAX; k++) if (w[k].seen) { if (n++ == i) return &w[k]; } return NULL; }
const worker_info_t *worker_pool_get_by_id(uint8_t id) { for (int i = 0; i < WORKER_MAX; i++) if (w[i].seen && w[i].id == id) return &w[i]; return NULL; }
static worker_info_t *byid(int id) { for (int i = 0; i < WORKER_MAX; i++) if (w[i].seen && w[i].id == id) return &w[i]; return NULL; }
static worker_info_t *by_mac(const char *mac) { for (int i = 0; i < WORKER_MAX; i++) if (w[i].seen && strcmp(w[i].mac, mac) == 0) return &w[i]; return NULL; }
static int new_id(void) { for (int id = 1; id <= WORKER_MAX; id++) if (!byid(id)) return id; return -1; }

static void send_packet(const char *msg, const char *ip, int port)
{
    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP); if (s < 0) return;
    int b = 1; setsockopt(s, SOL_SOCKET, SO_BROADCAST, &b, sizeof(b));
    struct sockaddr_in a = {0}; a.sin_family = AF_INET; a.sin_port = htons(port);
    a.sin_addr.s_addr = ip ? inet_addr(ip) : INADDR_BROADCAST;
    sendto(s, msg, strlen(msg), 0, (struct sockaddr *)&a, sizeof(a)); close(s);
}

static void urlenc(const char *in, char *out, size_t cap)
{
    const char *hex = "0123456789ABCDEF"; size_t o = 0;
    while (*in && o + 4 < cap) { unsigned char c = (unsigned char)*in++;
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') out[o++] = (char)c;
        else { out[o++] = '%'; out[o++] = hex[c >> 4]; out[o++] = hex[c & 15]; }
    }
    out[o] = 0;
}

static void assign(worker_info_t *x)
{
    char msg[64]; snprintf(msg, sizeof(msg), "ASSIGN|%d|192.168.4.1", x->id);
    send_packet(msg, x->ip, DISCOVERY_PORT); x->last_assign_ms = esp_log_timestamp();
}

static void parse_line(char *p)
{
    if (strncmp(p, "HELLO|", 6) == 0) {
        char tmp[256]; strlcpy(tmp, p, sizeof(tmp)); char *save = NULL;
        strtok_r(tmp, "|", &save); char *mac = strtok_r(NULL, "|", &save); if (!mac) return;
        char *ver = strtok_r(NULL, "|", &save); char *ip = strtok_r(NULL, "|", &save);
        worker_info_t *x = by_mac(mac);
        if (!x) {
            int id = new_id(); if (id < 0) return;
            for (int i = 0; i < WORKER_MAX; i++) if (!w[i].seen) { x = &w[i]; memset(x, 0, sizeof(*x)); x->seen = true; x->id = id; break; }
            strlcpy(x->mac, mac, sizeof(x->mac)); strlcpy(x->state, "READY", sizeof(x->state));
        }
        strlcpy(x->ip, ip ? ip : "-", sizeof(x->ip)); strlcpy(x->version, ver ? ver : "-", sizeof(x->version));
        x->last_seen_ms = esp_log_timestamp();
        if (esp_log_timestamp() - x->last_assign_ms > 15000) assign(x);
    } else if (strncmp(p, "HB|", 3) == 0) {
        char tmp[360]; strlcpy(tmp, p, sizeof(tmp)); char *save = NULL;
        strtok_r(tmp, "|", &save); char *s = strtok_r(NULL, "|", &save); int id = s ? atoi(s) : 0;
        worker_info_t *x = byid(id); if (!x) return;
        s = strtok_r(NULL, "|", &save); if (s) strlcpy(x->mac, s, sizeof(x->mac));
        s = strtok_r(NULL, "|", &save); if (s) strlcpy(x->state, s, sizeof(x->state));
        s = strtok_r(NULL, "|", &save); x->progress = s ? (uint32_t)atoi(s) : 0;
        s = strtok_r(NULL, "|", &save); x->uptime_ms = s ? (uint32_t)strtoul(s, NULL, 10) : 0;
        s = strtok_r(NULL, "|", &save); if (s) strlcpy(x->ip, s, sizeof(x->ip));
        s = strtok_r(NULL, "|", &save); if (s) strlcpy(x->job, s, sizeof(x->job));
        s = strtok_r(NULL, "|", &save); x->rssi = s ? (int32_t)atoi(s) : -127;
        s = strtok_r(NULL, "|", &save); x->cpu_mhz = s ? (uint16_t)atoi(s) : 0;
        s = strtok_r(NULL, "|", &save); x->heap_min = s ? (uint32_t)strtoul(s, NULL, 10) : 0;
        s = strtok_r(NULL, "|", &save); x->flash_size = s ? (uint32_t)strtoul(s, NULL, 10) : 0;
        s = strtok_r(NULL, "|", &save); x->cores = s ? (uint8_t)atoi(s) : 0;
        s = strtok_r(NULL, "|", &save); x->psram_size = s ? (uint32_t)strtoul(s, NULL, 10) : 0;
        s = strtok_r(NULL, "|", &save); x->resume_available = s ? atoi(s) != 0 : false;
        s = strtok_r(NULL, "|", &save); x->checkpoint_progress = s ? (uint32_t)strtoul(s, NULL, 10) : 0;
        x->last_seen_ms = esp_log_timestamp();
    }
}

static void rx_task(void *arg)
{
    (void)arg; s_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP); if (s_sock < 0) vTaskDelete(NULL);
    struct sockaddr_in a = {0}; a.sin_family = AF_INET; a.sin_port = htons(DISCOVERY_PORT); a.sin_addr.s_addr = INADDR_ANY;
    bind(s_sock, (struct sockaddr *)&a, sizeof(a)); struct timeval tv = {.tv_sec = 1, .tv_usec = 0}; setsockopt(s_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    while (1) { char b[512]; int n = recv(s_sock, b, sizeof(b) - 1, 0); if (n > 0) { b[n] = 0; parse_line(b); }
        int64_t now = esp_log_timestamp(); for (int i = 0; i < WORKER_MAX; i++) if (w[i].seen && now - w[i].last_seen_ms > WORKER_HEARTBEAT_TIMEOUT_MS) strlcpy(w[i].state, "OFFLINE", sizeof(w[i].state));
    }
}

static void log_task(void *arg)
{
    (void)arg; int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP); if (s < 0) vTaskDelete(NULL);
    struct sockaddr_in a = {0}; a.sin_family = AF_INET; a.sin_port = htons(LOG_PORT); a.sin_addr.s_addr = INADDR_ANY;
    bind(s, (struct sockaddr *)&a, sizeof(a));
    while (1) { char b[512]; int n = recv(s, b, sizeof(b) - 1, MSG_DONTWAIT); if (n > 0) { b[n] = 0; storage_append_text("/sd/LOGS/workers.log", b); storage_append_text("/sd/LOGS/workers.log", "\n"); } vTaskDelay(pdMS_TO_TICKS(50)); }
}

void worker_pool_push_ap_config(void)
{
    char ss[120], pp[140], msg[300]; urlenc(g_lab_cfg.ap_ssid, ss, sizeof(ss)); urlenc(g_lab_cfg.ap_pass, pp, sizeof(pp));
    snprintf(msg, sizeof(msg), "APCFG|%s|%s", ss, pp);
    for (int i = 0; i < WORKER_MAX; i++) if (w[i].seen && strcmp(w[i].state, "OFFLINE") != 0) send_packet(msg, w[i].ip, DISCOVERY_PORT);
}

void worker_pool_start(void)
{
    xTaskCreate(rx_task, "worker_rx", 4096, NULL, 8, NULL);
    xTaskCreate(log_task, "worker_log", 4096, NULL, 4, NULL);
    send_packet("DISCOVER|ESP32-LAB|" LAB_VERSION, NULL, DISCOVERY_PORT);
}

esp_err_t worker_send_job(uint8_t id, const char *type, int priority)
{
    worker_info_t *x = byid(id); if (!x || strcmp(x->state, "OFFLINE") == 0) return ESP_ERR_NOT_FOUND;
    char url[80], body[180]; snprintf(url, sizeof(url), "http://%s/api/job", x->ip); snprintf(body, sizeof(body), "type=%s&priority=%d", type, priority);
    esp_http_client_config_t c = {.url = url, .method = HTTP_METHOD_POST, .timeout_ms = 5000};
    esp_http_client_handle_t h = esp_http_client_init(&c); if (!h) return ESP_FAIL;
    esp_http_client_set_header(h, "Content-Type", "application/x-www-form-urlencoded"); esp_http_client_set_post_field(h, body, strlen(body));
    esp_err_t r = esp_http_client_perform(h); int code = esp_http_client_get_status_code(h); esp_http_client_cleanup(h);
    if (r == ESP_OK && code >= 200 && code < 300) { strlcpy(x->job, type, sizeof(x->job)); strlcpy(x->state, "BUSY", sizeof(x->state)); return ESP_OK; }
    return r != ESP_OK ? r : ESP_FAIL;
}

esp_err_t worker_cancel_job(uint8_t id)
{
    worker_info_t *x = byid(id);
    if (!x) return ESP_ERR_NOT_FOUND;
    char ep[96];
    snprintf(ep, sizeof(ep), "http://%s/api/cancel", x->ip);
    esp_http_client_config_t c = {.url = ep, .method = HTTP_METHOD_POST, .timeout_ms = 3000};
    esp_http_client_handle_t h = esp_http_client_init(&c);
    if (!h) return ESP_FAIL;
    esp_err_t r = esp_http_client_perform(h);
    int code = esp_http_client_get_status_code(h);
    esp_http_client_cleanup(h);
    if (r == ESP_OK && code >= 200 && code < 300) {
        strlcpy(x->state, "CANCELLING", sizeof(x->state));
        x->progress = 0;
        x->job[0] = 0;
        return ESP_OK;
    }
    return r != ESP_OK ? r : ESP_FAIL;
}


esp_err_t worker_reboot(uint8_t id)
{
    worker_info_t *x = byid(id);
    if (!x) return ESP_ERR_NOT_FOUND;
    char ep[96];
    snprintf(ep, sizeof(ep), "http://%s/api/reboot", x->ip);
    esp_http_client_config_t c = {.url = ep, .method = HTTP_METHOD_POST, .timeout_ms = 3000};
    esp_http_client_handle_t h = esp_http_client_init(&c);
    if (!h) return ESP_FAIL;
    esp_err_t r = esp_http_client_perform(h);
    int code = esp_http_client_get_status_code(h);
    esp_http_client_cleanup(h);
    if (r == ESP_OK && code >= 200 && code < 300) {
        strlcpy(x->state, "RECONNECTING", sizeof(x->state));
        x->job[0] = 0;
        x->progress = 0;
        return ESP_OK;
    }
    return r != ESP_OK ? r : ESP_FAIL;
}

static esp_err_t worker_post_flash(worker_info_t *x, const char *url, const char *sha)
{
    char body[560], ep[96]; snprintf(body, sizeof(body), "url=%s&sha256=%s", url, sha); snprintf(ep, sizeof(ep), "http://%s/api/flash", x->ip);
    esp_http_client_config_t c = {.url = ep, .method = HTTP_METHOD_POST, .timeout_ms = 10000}; esp_http_client_handle_t h = esp_http_client_init(&c); if (!h) return ESP_FAIL;
    esp_http_client_set_header(h, "Content-Type", "application/x-www-form-urlencoded"); esp_http_client_set_post_field(h, body, strlen(body));
    esp_err_t r = esp_http_client_perform(h); int code = esp_http_client_get_status_code(h); esp_http_client_cleanup(h); return (r == ESP_OK && code >= 200 && code < 300) ? ESP_OK : (r != ESP_OK ? r : ESP_FAIL);
}

esp_err_t worker_flash(uint8_t id, const char *sd_path)
{
    worker_info_t *x = byid(id); if (!x || !storage_ready()) return ESP_ERR_NOT_FOUND;
    char sha[65]; ESP_RETURN_ON_ERROR(storage_sha256_file(sd_path, sha), TAG, "sha");
    char token[17]; uint32_t seed = (uint32_t)esp_log_timestamp() ^ (uint32_t)esp_random(); const char *hex = "0123456789abcdef";
    for (int i = 0; i < 16; i++) {
        token[i] = hex[(seed >> ((i % 8) * 4)) & 0x0F];
    }
    token[16] = 0;
    ota_register_local_file(token, sd_path); char url[160]; snprintf(url, sizeof(url), "http://192.168.4.1/api/fw/%s", token);
    led_status_mode("flash"); return worker_post_flash(x, url, sha);
}
