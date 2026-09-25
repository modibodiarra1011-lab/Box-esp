#include "agent.h"
#include "lab_config.h"
#include "storage.h"
#include "worker_pool.h"
#include "job_engine.h"
#include "cJSON.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "agent";

static void mem_add(const char *role, const char *text)
{
    if (!text) return;
    cJSON *s = cJSON_CreateString(text);
    if (!s) return;
    char *escaped = cJSON_PrintUnformatted(s);
    cJSON_Delete(s);
    if (!escaped) return;

    char b[1500];
    snprintf(b, sizeof(b), "{\"role\":\"%s\",\"text\":%s}\n", role ? role : "unknown", escaped);
    free(escaped);
    storage_append_text("/sd/AI/memory.jsonl", b);
}

static void urlenc(const char *in, char *out, size_t cap)
{
    const char *hex = "0123456789ABCDEF"; size_t n = 0;
    if (!in || !out || cap == 0) return;
    while (*in && n + 4 < cap) {
        unsigned char c = (unsigned char)*in++;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c=='-' || c=='_' || c=='.' || c=='~') out[n++] = (char)c;
        else { out[n++]='%'; out[n++]=hex[c>>4]; out[n++]=hex[c&15]; }
    }
    out[n]=0;
}

static void safe_actions(const char *q)
{
    if (!q) return;
    char lower[240]; strlcpy(lower, q, sizeof(lower));
    for (char *p = lower; *p; ++p) if (*p >= 'A' && *p <= 'Z') *p = (char)(*p - 'A' + 'a');
    if (strstr(lower, "check-up") || strstr(lower, "checkup") || strstr(lower, "teste tous") || strstr(lower, "test tous")) {
        int id = job_create("SYSTEM_TEST", 7);
        ESP_LOGI(TAG, "Agent safe action: SYSTEM_TEST job=%d", id);
    } else if (strstr(lower, "ping")) {
        int id = job_create("PING", 6);
        ESP_LOGI(TAG, "Agent safe action: PING job=%d", id);
    }
}

static bool search_online(const char *q, char *out, size_t cap)
{
    if (!q || !*q || !g_lab_cfg.search_endpoint[0] || !out || cap < 16) return false;
    char enc[360], url[620], body[1800] = {0};
    urlenc(q, enc, sizeof(enc));
    const char *sep = strchr(g_lab_cfg.search_endpoint, '?') ? "&" : "?";
    snprintf(url, sizeof(url), "%s%sq=%s", g_lab_cfg.search_endpoint, sep, enc);
    esp_http_client_config_t cfg = { .url = url, .timeout_ms = 12000, .crt_bundle_attach = esp_crt_bundle_attach };
    esp_http_client_handle_t h = esp_http_client_init(&cfg); if (!h) return false;
    esp_err_t r = esp_http_client_perform(h);
    int n = (r == ESP_OK) ? esp_http_client_read_response(h, body, sizeof(body)-1) : -1;
    int code = esp_http_client_get_status_code(h); esp_http_client_cleanup(h);
    if (r != ESP_OK || code < 200 || code >= 300 || n <= 0) return false;
    body[n] = 0;
    cJSON *j = cJSON_Parse(body); if (!j) return false;
    cJSON *abs = cJSON_GetObjectItem(j, "AbstractText");
    cJSON *heading = cJSON_GetObjectItem(j, "Heading");
    if (cJSON_IsString(abs) && abs->valuestring[0]) {
        snprintf(out, cap, "Recherche: %s — %s", cJSON_IsString(heading)?heading->valuestring:"", abs->valuestring);
        cJSON_Delete(j); mem_add("research", out); return true;
    }
    cJSON_Delete(j); return false;
}

static void local_answer(const char *q, char *out, size_t cap)
{
    (void)q;
    snprintf(out, cap,
        "{\"answer\":\"Je peux orchestrer les workers, lancer un CHECK-UP, consulter la SD et preparer un plan. Pour une reponse IA en ligne, configure une API compatible dans le panneau prive.\",\"mode\":\"local\"}");
}

void agent_chat(const char *q, char *out, size_t cap)
{
    if (!q || !*q) { local_answer("", out, cap); return; }
    mem_add("user", q);
    safe_actions(q);
    char research[1800] = {0};
    bool have_research = search_online(q, research, sizeof(research));

    if (!g_lab_cfg.ai_endpoint[0]) {
        if (have_research) {
            cJSON *ro = cJSON_CreateObject();
            if (ro) {
                cJSON_AddStringToObject(ro, "answer", research);
                cJSON_AddStringToObject(ro, "mode", "research");
                char *rs = cJSON_PrintUnformatted(ro);
                if (rs) { strlcpy(out, rs, cap); free(rs); cJSON_Delete(ro); return; }
                cJSON_Delete(ro);
            }
        }
        local_answer(q, out, cap);
        return;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON *msgs = cJSON_CreateArray();
    cJSON *sys = cJSON_CreateObject();
    cJSON *u = cJSON_CreateObject();
    if (!root || !msgs || !sys || !u) {
        cJSON_Delete(root); cJSON_Delete(msgs); cJSON_Delete(sys); cJSON_Delete(u);
        local_answer(q, out, cap); return;
    }

    cJSON_AddStringToObject(root, "model", g_lab_cfg.ai_model[0] ? g_lab_cfg.ai_model : DEFAULT_AI_MODEL);
    cJSON_AddStringToObject(sys, "role", "system");
    cJSON_AddStringToObject(sys, "content",
        "Tu es l agent technique du laboratoire ESP32 LAB. Reponds en francais. "
        "Propose des plans, diagnostics et tests, mais ne presente jamais une action materielle comme executee sans resultat.");
    cJSON_AddItemToArray(msgs, sys);
    cJSON_AddStringToObject(u, "role", "user");
    if (have_research) { char prompt[2300]; snprintf(prompt, sizeof(prompt), "%s\n\nContexte de recherche externe:\n%s", q, research); cJSON_AddStringToObject(u, "content", prompt); } else cJSON_AddStringToObject(u, "content", q);
    cJSON_AddItemToArray(msgs, u);
    cJSON_AddItemToObject(root, "messages", msgs);

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!payload) { local_answer(q, out, cap); return; }

    esp_http_client_config_t cfg = {
        .url = g_lab_cfg.ai_endpoint,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 30000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    esp_http_client_handle_t h = esp_http_client_init(&cfg);
    if (!h) { free(payload); local_answer(q, out, cap); return; }
    esp_http_client_set_header(h, "Content-Type", "application/json");
    if (g_lab_cfg.ai_key[0]) {
        char auth[220];
        snprintf(auth, sizeof(auth), "Bearer %s", g_lab_cfg.ai_key);
        esp_http_client_set_header(h, "Authorization", auth);
    }
    esp_http_client_set_post_field(h, payload, strlen(payload));
    esp_err_t r = esp_http_client_perform(h);
    char resp[3000] = {0};
    int n = (r == ESP_OK) ? esp_http_client_read_response(h, resp, sizeof(resp) - 1) : -1;
    esp_http_client_cleanup(h);
    free(payload);

    if (r != ESP_OK || n <= 0) { local_answer(q, out, cap); return; }
    cJSON *j = cJSON_Parse(resp);
    if (!j) { local_answer(q, out, cap); return; }
    cJSON *choices = cJSON_GetObjectItem(j, "choices");
    cJSON *answer = NULL;
    if (cJSON_IsArray(choices) && cJSON_GetArraySize(choices) > 0) {
        cJSON *ch = cJSON_GetArrayItem(choices, 0);
        cJSON *msg = cJSON_GetObjectItem(ch, "message");
        if (msg) answer = cJSON_GetObjectItem(msg, "content");
    }
    if (answer && cJSON_IsString(answer)) {
        cJSON *o = cJSON_CreateObject();
        if (o) {
            cJSON_AddStringToObject(o, "answer", answer->valuestring);
            cJSON_AddStringToObject(o, "mode", "online");
            char *s = cJSON_PrintUnformatted(o);
            if (s) { strlcpy(out, s, cap); free(s); }
            cJSON_Delete(o);
            mem_add("assistant", answer->valuestring);
        } else local_answer(q, out, cap);
    } else local_answer(q, out, cap);
    cJSON_Delete(j);
}

void agent_start(void)
{
    storage_append_text("/sd/AI/memory.jsonl", "{\"event\":\"agent_start\"}\n");
    ESP_LOGI(TAG, "agent ready");
}
