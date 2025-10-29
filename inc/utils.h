#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HALT() asm("cli;hlt");
#define CLI() asm("cli");
#define STI() asm("sti");
#define htons(x) ((((x) & 0xFF) << 8) | ((x) >> 8))

bool strncmp(const char *a, const char *b, size_t n);
int strlen(char *s);
float bytes_to_gb(uint64_t bytes);
uint8_t bcd_to_bin(uint8_t val);
