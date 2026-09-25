#pragma once
#include <stddef.h>
void ota_manager_start(void);
void ota_register_local_file(const char *token,const char *path);
const char *ota_local_path_for_token(const char *token);
void ota_check_now(char *out,size_t cap);
void ota_approve(char *out,size_t cap);
