#include "lab_config.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_random.h"
#include "nvs.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "config";
lab_config_t g_lab_cfg;

static void random_secret(char *out, size_t cap, const char *prefix)
{
    static const char alphabet[] = "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789";
    size_t o = 0;
    if (prefix) while (*prefix && o + 1 < cap) out[o++] = *prefix++;
    uint32_t seed = esp_random();
    while (o + 1 < cap) {
        seed = seed * 1664525UL + 1013904223UL;
        out[o++] = alphabet[seed % (sizeof(alphabet) - 1)];
        if (o >= 20 && cap > 22) break;
    }
    out[o] = 0;
}

static void defaults(lab_config_t *c)
{
    memset(c, 0, sizeof(*c));
    strlcpy(c->ap_ssid, DEFAULT_AP_SSID, sizeof(c->ap_ssid));
    strlcpy(c->ai_model, DEFAULT_AI_MODEL, sizeof(c->ai_model));
    c->auto_updates = false;
    c->espnow_enabled = false;
    c->rgb_gpio = RGB_GPIO_V11;
    strlcpy(c->board_variant, "DEVKITC1_V1_1", sizeof(c->board_variant));
}

static void getstr(nvs_handle_t h, const char *key, char *out, size_t n)
{
    size_t len = n;
    if (nvs_get_str(h, key, out, &len) != ESP_OK) out[0] = 0;
}

void lab_config_load(lab_config_t *cfg)
{
    defaults(cfg);
    bool generated = false;
    nvs_handle_t h;
    if (nvs_open("labcfg", NVS_READONLY, &h) == ESP_OK) {
        getstr(h, "ap_ssid", cfg->ap_ssid, sizeof(cfg->ap_ssid));
        getstr(h, "ap_pass", cfg->ap_pass, sizeof(cfg->ap_pass));
        getstr(h, "sta_ssid", cfg->sta_ssid, sizeof(cfg->sta_ssid));
        getstr(h, "sta_pass", cfg->sta_pass, sizeof(cfg->sta_pass));
        getstr(h, "admin_pass", cfg->admin_pass, sizeof(cfg->admin_pass));
        getstr(h, "control", cfg->control_path, sizeof(cfg->control_path));
        getstr(h, "wa_phone", cfg->whatsapp_phone, sizeof(cfg->whatsapp_phone));
        getstr(h, "wa_api", cfg->whatsapp_api, sizeof(cfg->whatsapp_api));
        getstr(h, "ai_ep", cfg->ai_endpoint, sizeof(cfg->ai_endpoint));
        getstr(h, "ai_key", cfg->ai_key, sizeof(cfg->ai_key));
        getstr(h, "ai_model", cfg->ai_model, sizeof(cfg->ai_model));
        getstr(h, "search_ep", cfg->search_endpoint, sizeof(cfg->search_endpoint));
        getstr(h, "manifest", cfg->update_manifest, sizeof(cfg->update_manifest));
        getstr(h, "variant", cfg->board_variant, sizeof(cfg->board_variant));
        uint8_t b;
        if (nvs_get_u8(h, "auto", &b) == ESP_OK) cfg->auto_updates = b != 0;
        if (nvs_get_u8(h, "espnow", &b) == ESP_OK) cfg->espnow_enabled = b != 0;
        int32_t gpio;
        if (nvs_get_i32(h, "rgb_gpio", &gpio) == ESP_OK) cfg->rgb_gpio = gpio;
        nvs_close(h);
    }

    if (!cfg->ap_pass[0]) {
        if (DEFAULT_AP_PASSWORD[0]) {
            strlcpy(cfg->ap_pass, DEFAULT_AP_PASSWORD, sizeof(cfg->ap_pass));
        } else {
            random_secret(cfg->ap_pass, sizeof(cfg->ap_pass), "LAB-");
        }
        generated = true;
    }
    if (!cfg->admin_pass[0]) { random_secret(cfg->admin_pass, sizeof(cfg->admin_pass), "ADM-"); generated = true; }
    if (!cfg->control_path[0]) {
        char token[20]; random_secret(token, sizeof(token), "");
        snprintf(cfg->control_path, sizeof(cfg->control_path), "/x-control-%s", token);
        generated = true;
    }
    if (!cfg->ai_model[0]) strlcpy(cfg->ai_model, DEFAULT_AI_MODEL, sizeof(cfg->ai_model));
    if (!cfg->board_variant[0]) strlcpy(cfg->board_variant, "DEVKITC1_V1_1", sizeof(cfg->board_variant));
    if (strcmp(cfg->board_variant, "DEVKITC1_V1_0") == 0) cfg->rgb_gpio = RGB_GPIO_V10;
    else if (strcmp(cfg->board_variant, "DEVKITC1_V1_1") == 0) cfg->rgb_gpio = RGB_GPIO_V11;
    g_lab_cfg = *cfg;

    if (generated) {
        if (lab_config_save(cfg) == ESP_OK) {
            ESP_LOGI(TAG, "FIRST BOOT PRIVATE SETUP: AP SSID=%s", cfg->ap_ssid);
            ESP_LOGI(TAG, "FIRST BOOT PRIVATE SETUP: AP PASSWORD=%s", cfg->ap_pass);
            ESP_LOGI(TAG, "FIRST BOOT PRIVATE SETUP: CONTROL PATH=%s", cfg->control_path);
            ESP_LOGI(TAG, "FIRST BOOT PRIVATE SETUP: ADMIN PASSWORD=%s", cfg->admin_pass);
            ESP_LOGW(TAG, "Save these credentials; they are not exposed in the public dashboard.");
        }
    }
    ESP_LOGI(TAG, "configuration loaded: AP=%s variant=%s RGB=%d", cfg->ap_ssid, cfg->board_variant, cfg->rgb_gpio);
}

esp_err_t lab_config_save(const lab_config_t *cfg)
{
    if (!cfg || !cfg->ap_ssid[0] || strlen(cfg->ap_pass) < 8 || strlen(cfg->ap_pass) > 63 ||
        strlen(cfg->ap_ssid) > 32 || !cfg->admin_pass[0] || !cfg->control_path[0]) return ESP_ERR_INVALID_ARG;
    if (cfg->rgb_gpio != RGB_GPIO_V10 && cfg->rgb_gpio != RGB_GPIO_V11) return ESP_ERR_INVALID_ARG;

    nvs_handle_t h;
    ESP_RETURN_ON_ERROR(nvs_open("labcfg", NVS_READWRITE, &h), TAG, "nvs_open");
#define PUTS(k,v) do { ESP_RETURN_ON_ERROR(nvs_set_str(h, k, v), TAG, "nvs_set_str %s", k); } while (0)
    PUTS("ap_ssid", cfg->ap_ssid); PUTS("ap_pass", cfg->ap_pass);
    PUTS("sta_ssid", cfg->sta_ssid); PUTS("sta_pass", cfg->sta_pass);
    PUTS("admin_pass", cfg->admin_pass); PUTS("control", cfg->control_path);
    PUTS("wa_phone", cfg->whatsapp_phone); PUTS("wa_api", cfg->whatsapp_api);
    PUTS("ai_ep", cfg->ai_endpoint); PUTS("ai_key", cfg->ai_key); PUTS("ai_model", cfg->ai_model);
    PUTS("search_ep", cfg->search_endpoint); PUTS("manifest", cfg->update_manifest); PUTS("variant", cfg->board_variant);
    ESP_RETURN_ON_ERROR(nvs_set_u8(h, "auto", cfg->auto_updates ? 1 : 0), TAG, "auto");
    ESP_RETURN_ON_ERROR(nvs_set_u8(h, "espnow", cfg->espnow_enabled ? 1 : 0), TAG, "espnow");
    ESP_RETURN_ON_ERROR(nvs_set_i32(h, "rgb_gpio", cfg->rgb_gpio), TAG, "rgb_gpio");
    esp_err_t ret = nvs_commit(h); nvs_close(h); if (ret == ESP_OK) g_lab_cfg = *cfg; return ret;
#undef PUTS
}
