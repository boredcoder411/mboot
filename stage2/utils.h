#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HALT() asm("cli;hlt");
#define CLI() asm("cli");
#define STI() asm("sti");

char *int_to_str(int32_t val);
char *uint_to_str(uint32_t val);
char *uint_to_hex(uint64_t val);
char *hextoa(uint64_t val);
bool strncmp(const char *a, const char *b, size_t n);
void hexdump(void *data, size_t size);
float bytes_to_gb(uint64_t bytes);
void print_float(float f);
uint8_t bcd_to_bin(uint8_t val);
