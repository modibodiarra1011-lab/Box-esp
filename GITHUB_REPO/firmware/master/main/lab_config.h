#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#define LAB_VERSION "5.0.0"
#define LAB_NAME "ESP32 LAB"
#define DEFAULT_AP_SSID "ESP32-LAB"
#define DEFAULT_AP_PASSWORD "ESP32-LAB-Setup2026!"
#define DEFAULT_ADMIN_PASSWORD ""
#define DEFAULT_CONTROL_PATH ""
#define DEFAULT_AI_MODEL "gpt-4o-mini"
#define WORKER_MAX 8
#define JOB_MAX 24
#define DHT_GPIO 4
#define SD_CS_GPIO 10
#define SD_SCK_GPIO 12
#define SD_MISO_GPIO 13
#define SD_MOSI_GPIO 11
#define RGB_GPIO_V10 48
#define RGB_GPIO_V11 38
#define USB_HOST_VBUS_EN_GPIO -1
#define DISCOVERY_PORT 4211
#define LOG_PORT 4212
#define HTTP_PORT 80
#define WS_MAX_CLIENTS 8
#define UPDATE_INTERVAL_MS (6UL * 60UL * 60UL * 1000UL)
#define WORKER_HEARTBEAT_TIMEOUT_MS 10000UL
#define WEB_SESSION_MS (30UL * 60UL * 1000UL)
#define MAX_UPLOAD_BYTES (8UL * 1024UL * 1024UL)

typedef struct {
    char ap_ssid[33];
    char ap_pass[65];
    char sta_ssid[33];
    char sta_pass[65];
    char admin_pass[65];
    char control_path[65];
    char whatsapp_phone[32];
    char whatsapp_api[128];
    char ai_endpoint[192];
    char ai_key[160];
    char ai_model[64];
    char search_endpoint[192];
    char update_manifest[192];
    bool auto_updates;
    bool espnow_enabled;
    int rgb_gpio;
    char board_variant[16];
} lab_config_t;

void lab_config_load(lab_config_t *cfg);
esp_err_t lab_config_save(const lab_config_t *cfg);
extern lab_config_t g_lab_cfg;
