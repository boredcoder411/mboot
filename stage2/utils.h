#pragma once

#include <stdint.h>
#include <stddef.h>

char* itoa(int val);
char* hextoa(int val);
bool strncmp(const char* a, const char* b, size_t n);
void hexdump(void* data, size_t size);