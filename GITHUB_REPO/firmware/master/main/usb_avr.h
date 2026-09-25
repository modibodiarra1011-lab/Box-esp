#pragma once
#include <stddef.h>
#include "esp_err.h"
esp_err_t usb_avr_init(void);
esp_err_t usb_avr_flash_hex(const char *path,const char *profile,char *result,size_t cap);
bool usb_avr_ready(void);
