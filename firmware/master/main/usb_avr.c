#include "usb_avr.h"
#include "lab_config.h"
#include "usb/cdc_acm_host.h"
#include "usb/usb_host.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_intr_alloc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "usb_avr";
static cdc_acm_dev_hdl_t s_dev = NULL;
static bool s_ready = false;
static uint8_t s_rx[4096];
static size_t s_rx_len = 0;
static SemaphoreHandle_t s_lock = NULL;

static bool rx_cb(const uint8_t *data, size_t len, void *arg)
{
    (void)arg;
    if (!s_lock || xSemaphoreTake(s_lock, 0) != pdTRUE) return true;
    size_t room = sizeof(s_rx) - s_rx_len;
    if (len > room) len = room;
    if (len) { memcpy(s_rx + s_rx_len, data, len); s_rx_len += len; }
    xSemaphoreGive(s_lock);
    return true;
}

static void event_cb(const cdc_acm_host_dev_event_data_t *event, void *arg)
{
    (void)arg;
    if (!event) return;
    if (event->type == CDC_ACM_HOST_DEVICE_DISCONNECTED) {
        s_ready = false;
        s_dev = NULL;
        ESP_LOGW(TAG, "USB CDC device disconnected");
    } else if (event->type == CDC_ACM_HOST_ERROR) {
        ESP_LOGE(TAG, "USB CDC host error %d", event->data.error);
    }
}

static void usb_host_events_task(void *arg)
{
    (void)arg;
    while (1) {
        uint32_t flags = 0;
        esp_err_t err = usb_host_lib_handle_events(pdMS_TO_TICKS(100), &flags);
        if (err != ESP_OK) ESP_LOGW(TAG, "USB host event error: %s", esp_err_to_name(err));
        if (flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) usb_host_device_free_all();
    }
}

static void cdc_connect_task(void *arg)
{
    (void)arg;
    while (1) {
        if (!s_dev) {
            cdc_acm_host_device_config_t cfg = {
                .connection_timeout_ms = 1000,
                .out_buffer_size = 1024,
                .in_buffer_size = 1024,
                .event_cb = event_cb,
                .data_cb = rx_cb,
                .user_arg = NULL,
            };
            cdc_acm_dev_hdl_t h = NULL;
            if (cdc_acm_host_open(CDC_HOST_ANY_VID, CDC_HOST_ANY_PID, 0, &cfg, &h) == ESP_OK) {
                cdc_acm_line_coding_t lc = {
                    .dwDTERate = 115200,
                    .bCharFormat = 0,
                    .bParityType = 0,
                    .bDataBits = 8,
                };
                cdc_acm_host_line_coding_set(h, &lc);
                cdc_acm_host_set_control_line_state(h, true, true);
                s_dev = h;
                s_ready = true;
                ESP_LOGI(TAG, "USB CDC serial target ready");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void rx_clear(void)
{
    if (!s_lock) return;
    xSemaphoreTake(s_lock, portMAX_DELAY); s_rx_len = 0; xSemaphoreGive(s_lock);
}

static size_t rx_take(uint8_t *out, size_t cap, uint32_t timeout_ms)
{
    if (!out || !cap || !s_lock) return 0;
    TickType_t start = xTaskGetTickCount();
    TickType_t timeout = pdMS_TO_TICKS(timeout_ms);
    while ((xTaskGetTickCount() - start) < timeout) {
        xSemaphoreTake(s_lock, portMAX_DELAY);
        size_t n = s_rx_len < cap ? s_rx_len : cap;
        if (n) { memcpy(out, s_rx, n); memmove(s_rx, s_rx + n, s_rx_len - n); s_rx_len -= n; }
        xSemaphoreGive(s_lock);
        if (n) return n;
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    return 0;
}

static esp_err_t tx(const uint8_t *data, size_t len)
{
    if (!s_dev) return ESP_ERR_INVALID_STATE;
    return cdc_acm_host_data_tx_blocking(s_dev, data, len, 2000);
}

static bool response_ok(const uint8_t *data, size_t len)
{
    return data && len >= 2 && data[len - 2] == 0x14 && data[len - 1] == 0x10;
}

static esp_err_t sync_cmd(void)
{
    uint8_t c[] = {0x30, 0x20}, r[16]; rx_clear(); ESP_RETURN_ON_ERROR(tx(c, sizeof(c)), TAG, "sync tx");
    size_t n = rx_take(r, sizeof(r), 1500); return response_ok(r, n) ? ESP_OK : ESP_FAIL;
}

static esp_err_t cmd_simple(uint8_t cmd)
{
    uint8_t c[] = {cmd, 0x20}, r[16]; rx_clear(); ESP_RETURN_ON_ERROR(tx(c, sizeof(c)), TAG, "simple tx");
    size_t n = rx_take(r, sizeof(r), 1200); return response_ok(r, n) ? ESP_OK : ESP_FAIL;
}

static esp_err_t read_signature(uint8_t sig[3])
{
    uint8_t c[] = {0x75, 0x20}, r[16]; rx_clear(); ESP_RETURN_ON_ERROR(tx(c, sizeof(c)), TAG, "signature tx");
    size_t n = rx_take(r, sizeof(r), 1200); if (n < 5 || !response_ok(r, n)) return ESP_FAIL;
    memcpy(sig, r, 3); return ESP_OK;
}

static esp_err_t load_address(uint32_t word_addr)
{
    uint8_t c[] = {0x55, (uint8_t)(word_addr & 0xff), (uint8_t)((word_addr >> 8) & 0xff), 0x20};
    return tx(c, sizeof(c));
}

static esp_err_t program_page(const uint8_t *data, size_t len)
{
    if (!data || len == 0 || len > 256) return ESP_ERR_INVALID_ARG;
    uint8_t *frame = malloc(len + 5); if (!frame) return ESP_ERR_NO_MEM;
    frame[0] = 0x64; frame[1] = (uint8_t)(len >> 8); frame[2] = (uint8_t)len; frame[3] = 'F'; memcpy(frame + 4, data, len); frame[len + 4] = 0x20;
    rx_clear(); esp_err_t r = tx(frame, len + 5); free(frame); if (r != ESP_OK) return r;
    uint8_t response[8]; size_t n = rx_take(response, sizeof(response), 2500); return response_ok(response, n) ? ESP_OK : ESP_FAIL;
}

static esp_err_t read_page(uint8_t *data, size_t len)
{
    if (!data || len == 0 || len > 256) return ESP_ERR_INVALID_ARG;
    uint8_t c[] = {0x74, (uint8_t)(len >> 8), (uint8_t)len, 'F', 0x20}; rx_clear(); ESP_RETURN_ON_ERROR(tx(c, sizeof(c)), TAG, "read page tx");
    size_t got = 0; uint32_t remaining = 2500;
    while (got < len && remaining) { size_t n = rx_take(data + got, len - got, 50); if (n) got += n; else remaining -= 50; }
    uint8_t tail[4]; size_t n = rx_take(tail, sizeof(tail), 150); return (got == len && response_ok(tail, n)) ? ESP_OK : ESP_FAIL;
}

static int hexval(char c)
{
    if (c >= '0' && c <= '9') return c - '0'; if (c >= 'A' && c <= 'F') return c - 'A' + 10; if (c >= 'a' && c <= 'f') return c - 'a' + 10; return -1;
}

static bool hexbyte(const char *p, uint8_t *out)
{
    int a = hexval(p[0]), b = hexval(p[1]); if (a < 0 || b < 0) return false; *out = (uint8_t)((a << 4) | b); return true;
}

static bool parse_hex(const char *path, uint8_t *memory, size_t cap, size_t *used)
{
    FILE *f = fopen(path, "r"); if (!f) return false;
    memset(memory, 0xff, cap); char line[600]; uint32_t base_linear = 0, base_segment = 0; size_t max_addr = 0; bool saw_eof = false;
    while (fgets(line, sizeof(line), f)) {
        size_t len_line = strcspn(line, "\r\n"); line[len_line] = 0; if (!line[0]) continue;
        if (line[0] != ':' || len_line < 11) { fclose(f); return false; }
        uint8_t lenb, a_hi, a_lo, type; if (!hexbyte(line + 1, &lenb) || !hexbyte(line + 3, &a_hi) || !hexbyte(line + 5, &a_lo) || !hexbyte(line + 7, &type)) { fclose(f); return false; }
        size_t expected = 11 + (size_t)lenb * 2; if (len_line != expected) { fclose(f); return false; }
        uint8_t sum = 0; for (size_t i = 1; i < len_line; i += 2) { uint8_t v; if (!hexbyte(line + i, &v)) { fclose(f); return false; } sum = (uint8_t)(sum + v); }
        if (sum != 0) { fclose(f); return false; }
        uint16_t addr = ((uint16_t)a_hi << 8) | a_lo;
        if (type == 0x00) {
            uint32_t base = base_linear + base_segment + addr;
            for (size_t i = 0; i < lenb; i++) {
                uint8_t v; if (!hexbyte(line + 9 + i * 2, &v) || base + i >= cap) { fclose(f); return false; }
                memory[base + i] = v; if (base + i + 1 > max_addr) max_addr = base + i + 1;
            }
        } else if (type == 0x01) {
            if (lenb != 0) {
                fclose(f);
                return false;
            }
            saw_eof = true;
        } else if (type == 0x02) {
            if (lenb != 2) { fclose(f); return false; }
            uint8_t hi, lo; if (!hexbyte(line + 9, &hi) || !hexbyte(line + 11, &lo)) { fclose(f); return false; }
            base_segment = (((uint32_t)hi << 8) | lo) << 4; base_linear = 0;
        } else if (type == 0x04) {
            if (lenb != 2) { fclose(f); return false; }
            uint8_t hi, lo; if (!hexbyte(line + 9, &hi) || !hexbyte(line + 11, &lo)) { fclose(f); return false; }
            base_linear = ((uint32_t)hi << 24 | (uint32_t)lo << 16); base_segment = 0;
        }
    }
    fclose(f); *used = max_addr; return saw_eof && max_addr > 0;
}

static void reset_target(void)
{
    if (!s_dev) return;
    cdc_acm_host_set_control_line_state(s_dev, false, false);
    vTaskDelay(pdMS_TO_TICKS(120));
    cdc_acm_host_set_control_line_state(s_dev, true, true);
    vTaskDelay(pdMS_TO_TICKS(900));
}

esp_err_t usb_avr_init(void)
{
    s_lock = xSemaphoreCreateMutex(); if (!s_lock) return ESP_ERR_NO_MEM;
#if USB_HOST_VBUS_EN_GPIO >= 0
    gpio_set_direction(USB_HOST_VBUS_EN_GPIO, GPIO_MODE_OUTPUT); gpio_set_level(USB_HOST_VBUS_EN_GPIO, 1); vTaskDelay(pdMS_TO_TICKS(100));
#endif
    usb_host_config_t host_cfg = {.skip_phy_setup = false, .intr_flags = ESP_INTR_FLAG_LEVEL1};
    ESP_RETURN_ON_ERROR(usb_host_install(&host_cfg), TAG, "usb_host_install");
    xTaskCreate(usb_host_events_task, "usb_host_events", 4096, NULL, 10, NULL);
    cdc_acm_host_driver_config_t drv_cfg = {.driver_task_stack_size = 4096, .driver_task_priority = 11, .xCoreID = 0, .new_dev_cb = NULL};
    ESP_RETURN_ON_ERROR(cdc_acm_host_install(&drv_cfg), TAG, "cdc_acm_host_install");
    xTaskCreate(cdc_connect_task, "cdc_connect", 4096, NULL, 8, NULL);
    return ESP_OK;
}

bool usb_avr_ready(void) { return s_ready; }

esp_err_t usb_avr_flash_hex(const char *path, const char *profile, char *result, size_t cap)
{
    if (!result || cap == 0) return ESP_ERR_INVALID_ARG;
    if (!s_ready || !path || !profile) { snprintf(result, cap, "USB Host non disponible"); return ESP_ERR_INVALID_STATE; }
    size_t mem_size;
    uint32_t expected_sig;
    if (strcmp(profile, "ATmega328P_Optiboot") == 0) { mem_size = 32768; expected_sig = 0x1E950F; }
    else if (strcmp(profile, "ATmega168P_STK500") == 0) { mem_size = 16384; expected_sig = 0x1E9406; }
    else { snprintf(result, cap, "profil AVR non pris en charge"); return ESP_ERR_NOT_SUPPORTED; }

    uint8_t *memory = malloc(mem_size), *verify = malloc(256); if (!memory || !verify) { free(memory); free(verify); return ESP_ERR_NO_MEM; }
    size_t used = 0; if (!parse_hex(path, memory, mem_size, &used)) { free(memory); free(verify); snprintf(result, cap, "HEX invalide"); return ESP_FAIL; }
    reset_target();
    if (sync_cmd() != ESP_OK || cmd_simple(0x50) != ESP_OK) { free(memory); free(verify); snprintf(result, cap, "STK500: entree programmation echouee"); return ESP_FAIL; }
    uint8_t sig[3]; if (read_signature(sig) != ESP_OK) { cmd_simple(0x51); free(memory); free(verify); snprintf(result, cap, "AVR: signature illisible"); return ESP_FAIL; }
    uint32_t sig_value = ((uint32_t)sig[0] << 16) | ((uint32_t)sig[1] << 8) | sig[2];
    if (sig_value != expected_sig) { cmd_simple(0x51); free(memory); free(verify); snprintf(result, cap, "signature inattendue %06lX (attendu %06lX)", (unsigned long)sig_value, (unsigned long)expected_sig); return ESP_ERR_NOT_SUPPORTED; }

    const size_t page = 256;
    for (size_t addr = 0; addr < used; addr += page) {
        size_t n = (used - addr > page) ? page : used - addr; bool all_ff = true; for (size_t i = 0; i < n; i++) if (memory[addr + i] != 0xFF) { all_ff = false; break; }
        if (all_ff) continue;
        if (load_address((uint32_t)(addr / 2)) != ESP_OK || program_page(memory + addr, n) != ESP_OK) { cmd_simple(0x51); free(memory); free(verify); snprintf(result, cap, "ecriture echouee adresse %u", (unsigned)addr); return ESP_FAIL; }
    }
    for (size_t addr = 0; addr < used; addr += page) {
        size_t n = (used - addr > page) ? page : used - addr;
        if (load_address((uint32_t)(addr / 2)) != ESP_OK || read_page(verify, n) != ESP_OK || memcmp(verify, memory + addr, n) != 0) { cmd_simple(0x51); free(memory); free(verify); snprintf(result, cap, "verification echouee adresse %u", (unsigned)addr); return ESP_FAIL; }
    }
    cmd_simple(0x51); free(memory); free(verify); snprintf(result, cap, "AVR %06lX programme et verifie", (unsigned long)sig_value); return ESP_OK;
}
