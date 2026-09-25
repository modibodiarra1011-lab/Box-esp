#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#define WSTATE_MAX 16
typedef struct {
    bool seen;
    uint8_t id;
    char mac[18];
    char ip[16];
    char state[WSTATE_MAX];
    char version[32];
    char job[48];
    uint32_t progress;
    uint32_t heap;
    uint32_t heap_min;
    uint32_t uptime_ms;
    uint32_t flash_size;
    uint32_t psram_size;
    int32_t rssi;
    uint16_t cpu_mhz;
    uint8_t cores;
    int64_t last_seen_ms;
    int64_t last_assign_ms;
    bool resume_available;
    uint32_t checkpoint_progress;
} worker_info_t;
void worker_pool_start(void);
size_t worker_pool_count(void);
const worker_info_t *worker_pool_get(size_t i);
const worker_info_t *worker_pool_get_by_id(uint8_t id);
esp_err_t worker_send_job(uint8_t id, const char *type, int priority);
esp_err_t worker_cancel_job(uint8_t id);
esp_err_t worker_flash(uint8_t id, const char *sd_path);
esp_err_t worker_reboot(uint8_t id);
void worker_pool_push_ap_config(void);
