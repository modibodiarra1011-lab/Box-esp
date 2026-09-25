#include <stddef.h>
#pragma once
#include <stdbool.h>
void notifications_test(char *out,size_t cap);
bool notifications_send(const char *text);
