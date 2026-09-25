#pragma once
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"
#include "sdmmc_cmd.h"
void storage_init(void);
bool storage_ready(void);
const char *storage_root(void);
esp_err_t storage_prepare_tree(void);
esp_err_t storage_sha256_file(const char *path, char hex[65]);
esp_err_t storage_write_text(const char *path, const char *text);
esp_err_t storage_append_text(const char *path, const char *text);
