#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

char* itoa(int val);
char* hextoa(int val);
bool strncmp(const char* a, const char* b, size_t n);
void hexdump(void* data, size_t size);
static inline void memset(void *dst, uint8_t value, size_t n) {
    uint8_t *d = dst;

    while (n-- > 0) {
        *d++ = value;
    }
}
