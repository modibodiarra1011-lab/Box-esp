#include "web_server.h"
#include "web_ui.h"
#include "web_admin.h"
#include "lab_config.h"
#include "wifi_lab.h"
#include "worker_pool.h"
#include "job_engine.h"
#include "storage.h"
#include "notifications.h"
#include "agent.h"
#include "ota_manager.h"
#include "usb_avr.h"
#include "reports.h"
#include "cJSON.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>

static const char *TAG = "web";
static httpd_handle_t s_server = NULL;
static char s_session[33] = "";
static int64_t s_session_exp = 0;
static int64_t s_agent_last_ms = 0;

httpd_handle_t web_server_handle(void) { return s_server; }

static bool body_read(httpd_req_t *r, char *out, size_t cap)
{
    if (!r || !out || cap < 2 || r->content_len < 0 || r->content_len >= (int)cap) return false;
    int left = r->content_len, pos = 0;
    while (left > 0) {
        int n = httpd_req_recv(r, out + pos, left);
        if (n <= 0) return false;
        pos += n; left -= n;
    }
    out[pos] = 0;
    return true;
}

static const char *form_param(const char *src, const char *key, char *out, size_t cap)
{
    if (!src || !key || !out || cap == 0) return NULL;
    char pat[64]; snprintf(pat, sizeof(pat), "%s=", key);
    const char *p = strstr(src, pat); if (!p) return NULL; p += strlen(pat);
    size_t i = 0;
    while (*p && *p != '&' && i + 1 < cap) {
        out[i++] = *p++;
    }
    out[i] = 0;
    return out;
}

static void url_decode(char *s)
{
    char *t = s, *o = s;
    while (t && *t) {
        if (*t == '+') { *o++ = ' '; t++; }
        else if (*t == '%' && t[1] && t[2]) { char h[3] = {t[1], t[2], 0}; *o++ = (char)strtol(h, NULL, 16); t += 3; }
        else *o++ = *t++;
    }
    if (o) *o = 0;
}

static bool auth(httpd_req_t *r)
{
    char c[192] = {0};
    if (httpd_req_get_hdr_value_str(r, "Cookie", c, sizeof(c)) != ESP_OK) return false;
    char *e = strstr(c, "LABSESS="); if (!e) return false; e += 8;
    char tok[33] = {0}; strlcpy(tok, e, sizeof(tok)); char *semi = strchr(tok, ';'); if (semi) *semi = 0;
    return s_session[0] && strcmp(tok, s_session) == 0 && (esp_timer_get_time() / 1000) < s_session_exp;
}

static void send_json(httpd_req_t *r, const char *json)
{
    httpd_resp_set_type(r, "application/json; charset=utf-8");
    httpd_resp_send(r, json ? json : "{}", HTTPD_RESP_USE_STRLEN);
}

static esp_err_t index_get(httpd_req_t *r)
{
    httpd_resp_set_type(r, "text/html; charset=utf-8");
    return httpd_resp_send(r, lab_web_index, lab_web_index_len);
}

static esp_err_t state_get(httpd_req_t *r)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *m = cJSON_CreateObject();
    if (!root || !m) { cJSON_Delete(root); cJSON_Delete(m); return ESP_ERR_NO_MEM; }
    cJSON_AddStringToObject(m, "version", LAB_VERSION);
    cJSON_AddStringToObject(m, "ap_ip", "192.168.4.1");
    cJSON_AddStringToObject(m, "sta_ip", wifi_lab_sta_ip());
    cJSON_AddBoolToObject(m, "internet", wifi_lab_sta_connected());
    cJSON_AddBoolToObject(m, "sd", storage_ready());
    cJSON_AddBoolToObject(m, "usb_avr", usb_avr_ready());
    cJSON_AddStringToObject(m, "state", "READY");
    cJSON_AddNumberToObject(m, "heap", esp_get_free_heap_size());
    cJSON_AddNumberToObject(m, "heap_min", heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL));
    cJSON_AddNumberToObject(m, "psram", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    cJSON_AddNumberToObject(m, "psram_total", heap_caps_get_total_size(MALLOC_CAP_SPIRAM));
    cJSON_AddNumberToObject(m, "cpu_mhz", CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);
    cJSON_AddNumberToObject(m, "uptime_ms", esp_timer_get_time() / 1000);
    extern float g_temp, g_humidity;
    cJSON_AddNumberToObject(m, "temp", g_temp);
    cJSON_AddNumberToObject(m, "humidity", g_humidity);
    cJSON_AddItemToObject(root, "master", m);

    cJSON *a = cJSON_CreateArray();
    if (!a) { cJSON_Delete(root); return ESP_ERR_NO_MEM; }
    for (size_t i = 0; i < worker_pool_count(); i++) {
        const worker_info_t *w = worker_pool_get(i);
        cJSON *x = cJSON_CreateObject(); if (!w || !x) continue;
        cJSON_AddNumberToObject(x, "id", w->id); cJSON_AddStringToObject(x, "mac", w->mac);
        cJSON_AddStringToObject(x, "ip", w->ip); cJSON_AddStringToObject(x, "state", w->state);
        cJSON_AddStringToObject(x, "version", w->version); cJSON_AddStringToObject(x, "job", w->job);
        cJSON_AddNumberToObject(x, "progress", w->progress); cJSON_AddNumberToObject(x, "heap", w->heap);
        cJSON_AddNumberToObject(x, "heap_min", w->heap_min); cJSON_AddNumberToObject(x, "rssi", w->rssi);
        cJSON_AddNumberToObject(x, "cpu_mhz", w->cpu_mhz); cJSON_AddNumberToObject(x, "cores", w->cores);
        cJSON_AddNumberToObject(x, "flash_size", w->flash_size); cJSON_AddNumberToObject(x, "psram_size", w->psram_size);
        cJSON_AddNumberToObject(x, "uptime_ms", w->uptime_ms); cJSON_AddItemToArray(a, x);
    }
    cJSON_AddItemToObject(root, "workers", a);
    char *s = cJSON_PrintUnformatted(root);
    if (!s) { cJSON_Delete(root); return ESP_ERR_NO_MEM; }
    send_json(r, s); free(s); cJSON_Delete(root); return ESP_OK;
}

static esp_err_t jobs_get(httpd_req_t *r)
{
    char b[4096]; job_json(b, sizeof(b)); send_json(r, b); return ESP_OK;
}

static esp_err_t logs_get(httpd_req_t *r)
{
    httpd_resp_set_type(r, "text/plain; charset=utf-8");
    FILE *f = fopen("/sd/LOGS/workers.log", "r");
    if (!f) return httpd_resp_send(r, "", HTTPD_RESP_USE_STRLEN);
    char b[8192]; size_t n = fread(b, 1, sizeof(b) - 1, f); fclose(f); b[n] = 0;
    return httpd_resp_send(r, b, n);
}

static bool safe_job_type(const char *type)
{
    return type && (!strcmp(type, "PING") || !strcmp(type, "SYSTEM_TEST") || !strcmp(type, "CHECKUP") ||
                    !strcmp(type, "BENCHMARK") || !strcmp(type, "FS_TEST"));
}

static esp_err_t job_post(httpd_req_t *r)
{
    char b[256] = {0}, type[64] = {0}, pr[16] = {0};
    if (!body_read(r, b, sizeof(b))) return ESP_ERR_INVALID_SIZE;
    form_param(b, "type", type, sizeof(type)); form_param(b, "priority", pr, sizeof(pr)); url_decode(type);
    if (!safe_job_type(type) && !auth(r)) return httpd_resp_send_err(r, HTTPD_401_UNAUTHORIZED, "locked");
    int priority = atoi(pr); int id = job_create(type, priority);
    char out[96]; snprintf(out, sizeof(out), "{\"accepted\":%s,\"id\":%d}", id > 0 ? "true" : "false", id); send_json(r, out); return ESP_OK;
}

static esp_err_t control_get(httpd_req_t *r)
{
    if (!auth(r)) {
        httpd_resp_set_type(r, "text/html; charset=utf-8");
        const char *html = "<!doctype html><html lang='fr'><meta name='viewport' content='width=device-width,initial-scale=1'><body style='background:#06111f;color:#eaf6ff;font:16px system-ui;padding:28px'><h2>ESP32 LAB</h2><p>Accès privé</p><form method='POST'><input name='password' type='password' autocomplete='current-password' placeholder='Mot de passe' required style='padding:12px;border-radius:10px'><button style='padding:12px;margin-left:6px'>Entrer</button></form></body></html>";
        return httpd_resp_send(r, html, HTTPD_RESP_USE_STRLEN);
    }
    httpd_resp_set_type(r, "text/html; charset=utf-8"); return httpd_resp_send(r, lab_web_admin, lab_web_admin_len);
}

static esp_err_t control_post(httpd_req_t *r)
{
    if (strcmp(r->uri, g_lab_cfg.control_path) != 0) return ESP_ERR_NOT_FOUND;
    char b[240] = {0}, pw[100] = {0}; if (!body_read(r, b, sizeof(b))) return ESP_ERR_INVALID_SIZE;
    form_param(b, "password", pw, sizeof(pw)); url_decode(pw);
    if (strcmp(pw, g_lab_cfg.admin_pass) != 0) return httpd_resp_send_err(r, HTTPD_403_FORBIDDEN, "acces refuse");
    uint32_t x = (uint32_t)esp_timer_get_time(); snprintf(s_session, sizeof(s_session), "%08x%08x", x, (unsigned)esp_random());
    s_session_exp = esp_timer_get_time() / 1000 + WEB_SESSION_MS;
    char h[100]; snprintf(h, sizeof(h), "LABSESS=%s; HttpOnly; SameSite=Strict", s_session); httpd_resp_set_hdr(r, "Set-Cookie", h);
    httpd_resp_set_status(r, "303 See Other"); httpd_resp_set_hdr(r, "Location", g_lab_cfg.control_path);
    return httpd_resp_send(r, "", 0);
}

static bool public_sd_path(const char *p)
{
    if (!p) return false;
    const char *ok[] = {"/sd/PROJECTS", "/sd/FIRMWARE", "/sd/COMPONENTS", "/sd/TESTS", "/sd/REPORTS"};
    for (size_t i = 0; i < sizeof(ok) / sizeof(ok[0]); i++) if (strncmp(p, ok[i], strlen(ok[i])) == 0) return true;
    return false;
}

static bool valid_sd_path(const char *p, bool allow_root)
{
    if (!p || strstr(p, "..") || strlen(p) >= 240 || strncmp(p, "/sd", 3) != 0) return false;
    if (allow_root && strcmp(p, "/sd") == 0) return true;
    return strncmp(p, "/sd/", 4) == 0;
}

static esp_err_t sd_list(httpd_req_t *r)
{
    char q[260] = {0}, path[240] = "/sd";
    if (httpd_req_get_url_query_len(r) > 0 && httpd_req_get_url_query_str(r, q, sizeof(q)) == ESP_OK) {
        char v[220] = {0}; if (httpd_query_key_value(q, "path", v, sizeof(v)) == ESP_OK) strlcpy(path, v, sizeof(path));
    }
    if (!valid_sd_path(path, true)) return httpd_resp_send_err(r, HTTPD_400_BAD_REQUEST, "chemin invalide");
    if (strcmp(path, "/sd") != 0 && !public_sd_path(path) && !auth(r)) return httpd_resp_send_err(r, HTTPD_401_UNAUTHORIZED, "locked");
    DIR *d = opendir(path); if (!d) return httpd_resp_send_err(r, HTTPD_404_NOT_FOUND, "dossier introuvable");
    char *out = malloc(12000); if (!out) { closedir(d); return ESP_ERR_NO_MEM; }
    size_t n = 0; n += snprintf(out + n, 12000 - n, "["); bool first = true; struct dirent *e;
    while ((e = readdir(d)) && n < 11500) {
        if (!first) {
            n += snprintf(out + n, 12000 - n, ",");
        }
        first = false;
        n += snprintf(out + n, 12000 - n, "{\"name\":\"%s\",\"type\":\"%c\"}", e->d_name, e->d_type == DT_DIR ? 'd' : 'f');
    }
    closedir(d); snprintf(out + n, 12000 - n, "]"); send_json(r, out); free(out); return ESP_OK;
}

static esp_err_t sd_download(httpd_req_t *r)
{
    char q[260] = {0}, v[240] = {0};
    if (httpd_req_get_url_query_len(r) <= 0 || httpd_req_get_url_query_str(r, q, sizeof(q)) != ESP_OK || httpd_query_key_value(q, "path", v, sizeof(v)) != ESP_OK)
        return httpd_resp_send_err(r, HTTPD_400_BAD_REQUEST, "path");
    if (!valid_sd_path(v, false)) return httpd_resp_send_err(r, HTTPD_400_BAD_REQUEST, "path");
    if (!public_sd_path(v) && !auth(r)) return httpd_resp_send_err(r, HTTPD_401_UNAUTHORIZED, "locked");
    FILE *f = fopen(v, "rb"); if (!f) return httpd_resp_send_404(r);
    httpd_resp_set_type(r, "application/octet-stream; charset=binary"); char b[4096]; size_t n;
    while ((n = fread(b, 1, sizeof(b), f)) > 0) if (httpd_resp_send_chunk(r, b, n) != ESP_OK) break;
    fclose(f); return httpd_resp_send_chunk(r, NULL, 0);
}

static esp_err_t sd_upload(httpd_req_t *r)
{
    if (!auth(r) || r->content_len < 0 || r->content_len > MAX_UPLOAD_BYTES) return httpd_resp_send_err(r, HTTPD_400_BAD_REQUEST, "upload");
    char path[240]; if (httpd_req_get_hdr_value_str(r, "X-Path", path, sizeof(path)) != ESP_OK || !valid_sd_path(path, false)) return httpd_resp_send_err(r, HTTPD_400_BAD_REQUEST, "X-Path");
    FILE *f = fopen(path, "wb"); if (!f) return httpd_resp_send_err(r, HTTPD_500_INTERNAL_SERVER_ERROR, "open");
    char b[4096]; int rem = r->content_len; while (rem > 0) { int k = rem > (int)sizeof(b) ? (int)sizeof(b) : rem; int n = httpd_req_recv(r, b, k); if (n <= 0) { fclose(f); return ESP_FAIL; } if (fwrite(b, 1, (size_t)n, f) != (size_t)n) { fclose(f); return ESP_FAIL; } rem -= n; }
    fclose(f); return httpd_resp_sendstr(r, "OK");
}

static esp_err_t fw_local(httpd_req_t *r)
{
    const char *token = strrchr(r->uri, '/'); if (!token || !*(++token)) return ESP_FAIL;
    const char *p = ota_local_path_for_token(token); if (!p) return httpd_resp_send_404(r); FILE *f = fopen(p, "rb"); if (!f) return httpd_resp_send_404(r);
    httpd_resp_set_type(r, "application/octet-stream"); char b[4096]; size_t n; while ((n = fread(b, 1, sizeof(b), f)) > 0) httpd_resp_send_chunk(r, b, n); fclose(f); return httpd_resp_send_chunk(r, NULL, 0);
}

static esp_err_t worker_reboot_post(httpd_req_t *r)
{
    if (!auth(r)) return httpd_resp_send_err(r, HTTPD_401_UNAUTHORIZED, "locked");
    char b[100] = {0}, idv[16] = {0};
    if (!body_read(r, b, sizeof(b))) return ESP_ERR_INVALID_SIZE;
    form_param(b, "id", idv, sizeof(idv));
    int id = atoi(idv);
    esp_err_t e = worker_reboot((uint8_t)id);
    char out[120]; snprintf(out, sizeof(out), "{\"ok\":%s,\"err\":%d}", e == ESP_OK ? "true" : "false", e);
    send_json(r, out);
    return ESP_OK;
}

static esp_err_t worker_flash_post(httpd_req_t *r)
{
    if (!auth(r)) return httpd_resp_send_err(r, HTTPD_401_UNAUTHORIZED, "locked");
    char b[340] = {0}, id[16] = {0}, path[220] = {0}; if (!body_read(r, b, sizeof(b))) return ESP_ERR_INVALID_SIZE;
    form_param(b, "id", id, sizeof(id)); form_param(b, "path", path, sizeof(path)); url_decode(path);
    if (!valid_sd_path(path, false)) return httpd_resp_send_err(r, HTTPD_400_BAD_REQUEST, "path");
    esp_err_t e = worker_flash((uint8_t)atoi(id), path); char out[160]; snprintf(out, sizeof(out), "{\"ok\":%s,\"err\":%d}", e == ESP_OK ? "true" : "false", e); send_json(r, out); return ESP_OK;
}

static esp_err_t avr_flash_post(httpd_req_t *r)
{
    if (!auth(r)) return httpd_resp_send_err(r, HTTPD_401_UNAUTHORIZED, "locked");
    char b[360] = {0}, path[220] = {0}, profile[64] = {0}; if (!body_read(r, b, sizeof(b))) return ESP_ERR_INVALID_SIZE;
    form_param(b, "path", path, sizeof(path)); form_param(b, "profile", profile, sizeof(profile)); url_decode(path); url_decode(profile);
    if (!valid_sd_path(path, false)) return httpd_resp_send_err(r, HTTPD_400_BAD_REQUEST, "path");
    char result[256] = {0}; esp_err_t e = usb_avr_flash_hex(path, profile, result, sizeof(result)); char out[420];
    cJSON *j = cJSON_CreateObject(); if (!j) return ESP_ERR_NO_MEM; cJSON_AddBoolToObject(j, "ok", e == ESP_OK); cJSON_AddStringToObject(j, "message", result); cJSON_AddNumberToObject(j, "err", e); char *s = cJSON_PrintUnformatted(j); cJSON_Delete(j); if (!s) return ESP_ERR_NO_MEM; send_json(r, s); free(s); return ESP_OK;
}

static esp_err_t job_cancel_post(httpd_req_t *r)
{
    if (!auth(r)) return httpd_resp_send_err(r, HTTPD_401_UNAUTHORIZED, "locked");
    char b[100]={0}, idv[16]={0};
    if (!body_read(r,b,sizeof(b))) return ESP_ERR_INVALID_SIZE;
    form_param(b,"id",idv,sizeof(idv));
    int id=atoi(idv);
    bool ok=job_cancel(id);
    send_json(r, ok ? "{\"ok\":true}" : "{\"ok\":false}");
    return ESP_OK;
}

static esp_err_t job_cancel_all_post(httpd_req_t *r)
{
    if (!auth(r)) return httpd_resp_send_err(r, HTTPD_401_UNAUTHORIZED, "locked");
    size_t n = job_cancel_all();
    char out[96];
    snprintf(out, sizeof(out), "{\"ok\":true,\"cancelled\":%u}", (unsigned)n);
    send_json(r, out);
    return ESP_OK;
}

static esp_err_t update_check(httpd_req_t *r)
{ if (!auth(r)) return httpd_resp_send_err(r, HTTPD_401_UNAUTHORIZED, "locked"); char out[640]; ota_check_now(out, sizeof(out)); send_json(r, out); return ESP_OK; }
static esp_err_t update_approve(httpd_req_t *r)
{ if (!auth(r)) return httpd_resp_send_err(r, HTTPD_401_UNAUTHORIZED, "locked"); char out[256]; ota_approve(out, sizeof(out)); send_json(r, out); return ESP_OK; }
static esp_err_t notify_test(httpd_req_t *r)
{ if (!auth(r)) return httpd_resp_send_err(r, HTTPD_401_UNAUTHORIZED, "locked"); char out[256]; notifications_test(out, sizeof(out)); send_json(r, out); return ESP_OK; }

static esp_err_t admin_config_get(httpd_req_t *r)
{
    if (!auth(r)) return httpd_resp_send_err(r, HTTPD_401_UNAUTHORIZED, "locked");
    cJSON *j = cJSON_CreateObject(); if (!j) return ESP_ERR_NO_MEM;
    cJSON_AddBoolToObject(j, "ok", true); cJSON_AddStringToObject(j, "ap_ssid", g_lab_cfg.ap_ssid); cJSON_AddStringToObject(j, "sta_ssid", g_lab_cfg.sta_ssid);
    cJSON_AddStringToObject(j, "ai_endpoint", g_lab_cfg.ai_endpoint); cJSON_AddStringToObject(j, "ai_model", g_lab_cfg.ai_model); cJSON_AddStringToObject(j, "search_endpoint", g_lab_cfg.search_endpoint);
    cJSON_AddStringToObject(j, "update_manifest", g_lab_cfg.update_manifest); cJSON_AddNumberToObject(j, "rgb_gpio", g_lab_cfg.rgb_gpio); cJSON_AddBoolToObject(j, "auto_updates", g_lab_cfg.auto_updates);
    char *s = cJSON_PrintUnformatted(j); cJSON_Delete(j); if (!s) return ESP_ERR_NO_MEM; send_json(r, s); free(s); return ESP_OK;
}

static void copy_json_string(cJSON *obj, const char *key, char *dst, size_t cap, bool keep_blank)
{
    cJSON *v = cJSON_GetObjectItem(obj, key); if (!cJSON_IsString(v)) return; if (!keep_blank && v->valuestring[0] == 0) return; strlcpy(dst, v->valuestring, cap);
}

static esp_err_t admin_config_post(httpd_req_t *r)
{
    if (!auth(r) || r->content_len < 0 || r->content_len > 4000) return httpd_resp_send_err(r, HTTPD_401_UNAUTHORIZED, "locked");
    char b[4001]; if (!body_read(r, b, sizeof(b))) return ESP_ERR_INVALID_SIZE; cJSON *j = cJSON_Parse(b); if (!j) return httpd_resp_send_err(r, HTTPD_400_BAD_REQUEST, "JSON invalide");
    lab_config_t next = g_lab_cfg;
    copy_json_string(j, "ap_ssid", next.ap_ssid, sizeof(next.ap_ssid), true); copy_json_string(j, "ap_pass", next.ap_pass, sizeof(next.ap_pass), false);
    copy_json_string(j, "sta_ssid", next.sta_ssid, sizeof(next.sta_ssid), true); copy_json_string(j, "sta_pass", next.sta_pass, sizeof(next.sta_pass), false);
    copy_json_string(j, "whatsapp_phone", next.whatsapp_phone, sizeof(next.whatsapp_phone), true); copy_json_string(j, "whatsapp_api", next.whatsapp_api, sizeof(next.whatsapp_api), false);
    copy_json_string(j, "ai_endpoint", next.ai_endpoint, sizeof(next.ai_endpoint), true); copy_json_string(j, "ai_key", next.ai_key, sizeof(next.ai_key), false); copy_json_string(j, "ai_model", next.ai_model, sizeof(next.ai_model), true);
    copy_json_string(j, "search_endpoint", next.search_endpoint, sizeof(next.search_endpoint), true); copy_json_string(j, "update_manifest", next.update_manifest, sizeof(next.update_manifest), true);
    cJSON *gpio = cJSON_GetObjectItem(j, "rgb_gpio"); if (cJSON_IsNumber(gpio)) next.rgb_gpio = gpio->valueint;
    cJSON *au = cJSON_GetObjectItem(j, "auto_updates"); if (cJSON_IsBool(au)) next.auto_updates = cJSON_IsTrue(au);
    cJSON_Delete(j);
    if (lab_config_save(&next) != ESP_OK) return httpd_resp_send_err(r, HTTPD_400_BAD_REQUEST, "configuration invalide");
    worker_pool_push_ap_config();
    notifications_send("ESP32 LAB : configuration mise a jour. Le MASTER redemarre.");
    send_json(r, "{\"ok\":true,\"restart\":true}");
    vTaskDelay(pdMS_TO_TICKS(400)); esp_restart(); return ESP_OK;
}

static esp_err_t agent_chat_post(httpd_req_t *r)
{
    int64_t now = esp_timer_get_time() / 1000; if (now - s_agent_last_ms < 1500) { httpd_resp_set_status(r, "429 Too Many Requests"); return httpd_resp_send(r, "trop rapide", HTTPD_RESP_USE_STRLEN); } s_agent_last_ms = now;
    char b[560] = {0}, q[460] = {0}; if (!body_read(r, b, sizeof(b))) return ESP_ERR_INVALID_SIZE; form_param(b, "q", q, sizeof(q)); url_decode(q); char out[2200]; agent_chat(q, out, sizeof(out)); send_json(r, out); return ESP_OK;
}

static esp_err_t ws(httpd_req_t *r)
{
    if (r->method == HTTP_GET) return ESP_OK;
    httpd_ws_frame_t f = {0}; f.type = HTTPD_WS_TYPE_TEXT; if (httpd_ws_recv_frame(r, &f, 0) != ESP_OK) return ESP_FAIL;
    if (f.len == 0) return ESP_OK; uint8_t *p = malloc(f.len + 1); if (!p) return ESP_ERR_NO_MEM; f.payload = p;
    esp_err_t e = httpd_ws_recv_frame(r, &f, f.len); free(p); return e;
}

static void ws_work(void *arg)
{
    (void)arg; httpd_handle_t h = s_server; if (!h) return;
    cJSON *root = cJSON_CreateObject(), *m = cJSON_CreateObject(); if (!root || !m) { cJSON_Delete(root); cJSON_Delete(m); return; }
    extern float g_temp, g_humidity; cJSON_AddStringToObject(m, "version", LAB_VERSION); cJSON_AddStringToObject(m, "state", "READY"); cJSON_AddBoolToObject(m, "internet", wifi_lab_sta_connected()); cJSON_AddStringToObject(m, "sta_ip", wifi_lab_sta_ip()); cJSON_AddStringToObject(m, "ap_ip", "192.168.4.1"); cJSON_AddBoolToObject(m, "sd", storage_ready()); cJSON_AddBoolToObject(m, "usb_avr", usb_avr_ready()); cJSON_AddNumberToObject(m, "heap", esp_get_free_heap_size()); cJSON_AddNumberToObject(m, "heap_min", heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL)); cJSON_AddNumberToObject(m, "psram", heap_caps_get_free_size(MALLOC_CAP_SPIRAM)); cJSON_AddNumberToObject(m, "psram_total", heap_caps_get_total_size(MALLOC_CAP_SPIRAM)); cJSON_AddNumberToObject(m, "cpu_mhz", CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ); cJSON_AddNumberToObject(m, "temp", g_temp); cJSON_AddNumberToObject(m, "humidity", g_humidity); cJSON_AddItemToObject(root, "master", m);
    cJSON *a = cJSON_CreateArray(); for (size_t i = 0; i < worker_pool_count(); i++) { const worker_info_t *w = worker_pool_get(i); if (!w) continue; cJSON *x = cJSON_CreateObject(); cJSON_AddNumberToObject(x, "id", w->id); cJSON_AddStringToObject(x, "ip", w->ip); cJSON_AddStringToObject(x, "mac", w->mac); cJSON_AddStringToObject(x, "state", w->state); cJSON_AddStringToObject(x, "version", w->version); cJSON_AddStringToObject(x, "job", w->job); cJSON_AddNumberToObject(x, "progress", w->progress); cJSON_AddNumberToObject(x, "heap", w->heap); cJSON_AddItemToArray(a, x); } cJSON_AddItemToObject(root, "workers", a);
    char *s = cJSON_PrintUnformatted(root); cJSON_Delete(root); if (!s) return;
    size_t cap = WS_MAX_CLIENTS; int fds[WS_MAX_CLIENTS]; if (httpd_get_client_list(h, &cap, fds) != ESP_OK) { free(s); return; }
    httpd_ws_frame_t fr = {.type = HTTPD_WS_TYPE_TEXT, .payload = (uint8_t *)s, .len = strlen(s)};
    for (size_t i = 0; i < cap; i++) if (httpd_ws_get_fd_info(h, fds[i]) == HTTPD_WS_CLIENT_WEBSOCKET) httpd_ws_send_data(h, fds[i], &fr);
    free(s);
}

static void ws_task(void *arg)
{
    (void)arg; while (1) { if (s_server) httpd_queue_work(s_server, ws_work, NULL); vTaskDelay(pdMS_TO_TICKS(1500)); }
}

void web_server_start(void)
{
    httpd_config_t c = HTTPD_DEFAULT_CONFIG(); c.server_port = HTTP_PORT; c.max_open_sockets = WS_MAX_CLIENTS; c.uri_match_fn = httpd_uri_match_wildcard;
    ESP_ERROR_CHECK(httpd_start(&s_server, &c));
    httpd_uri_t u = {0}; u.uri = "/"; u.method = HTTP_GET; u.handler = index_get; ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &u));
#define REG(path,m,fn) do { httpd_uri_t z = {0}; z.uri = path; z.method = m; z.handler = fn; ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &z)); } while (0)
    REG("/api/state", HTTP_GET, state_get); REG("/api/jobs", HTTP_GET, jobs_get); REG("/api/logs", HTTP_GET, logs_get); REG("/api/job", HTTP_POST, job_post); REG("/api/job/cancel", HTTP_POST, job_cancel_post); REG("/api/jobs/cancel-all", HTTP_POST, job_cancel_all_post);
    REG("/api/sd/list", HTTP_GET, sd_list); REG("/api/sd/download", HTTP_GET, sd_download); REG("/api/sd/upload", HTTP_POST, sd_upload);
    REG("/api/worker/reboot", HTTP_POST, worker_reboot_post); REG("/api/worker/flash", HTTP_POST, worker_flash_post); REG("/api/avr/flash", HTTP_POST, avr_flash_post); REG("/api/update/check", HTTP_POST, update_check); REG("/api/update/approve", HTTP_POST, update_approve); REG("/api/notify/test", HTTP_POST, notify_test);
    REG("/api/admin/config", HTTP_GET, admin_config_get); REG("/api/admin/config", HTTP_POST, admin_config_post); REG("/api/agent/chat", HTTP_POST, agent_chat_post); REG("/api/fw/*", HTTP_GET, fw_local);
    REG(g_lab_cfg.control_path, HTTP_GET, control_get); REG(g_lab_cfg.control_path, HTTP_POST, control_post);
    httpd_uri_t w = {.uri = "/ws", .method = HTTP_GET, .handler = ws, .is_websocket = true}; ESP_ERROR_CHECK(httpd_register_uri_handler(s_server, &w));
    xTaskCreate(ws_task, "ws_push", 4096, NULL, 3, NULL);
#undef REG
    ESP_LOGI(TAG, "web server ready");
}
