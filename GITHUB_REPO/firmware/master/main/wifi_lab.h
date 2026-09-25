#pragma once
#include <stdbool.h>
#include "esp_err.h"
void wifi_lab_start(void);
bool wifi_lab_sta_connected(void);
const char *wifi_lab_sta_ip(void);
