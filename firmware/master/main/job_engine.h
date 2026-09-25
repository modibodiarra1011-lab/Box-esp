#pragma once
#include <stdbool.h>
#include <stddef.h>
void job_engine_start(void);
int job_create(const char *type, int priority);
bool job_cancel(int id);
size_t job_cancel_all(void);
size_t job_json(char *out, size_t cap);
