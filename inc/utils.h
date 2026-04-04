#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HALT() asm("cli;hlt");
#define CLI() asm("cli");
#define STI() asm("sti");
#define htons(x) ((((x) & 0xFF) << 8) | ((x) >> 8))
#define ntohs(x) htons(x)
#define htonl(x)                                                             \
  ((((uint32_t)(x) & 0x000000FFU) << 24) |                                   \
   (((uint32_t)(x) & 0x0000FF00U) << 8) |                                    \
   (((uint32_t)(x) & 0x00FF0000U) >> 8) |                                    \
   (((uint32_t)(x) & 0xFF000000U) >> 24))
#define ntohl(x) htonl(x)
#define strcmp(a, b) strncmp(a, b, strlen(a))

bool strncmp(const char *a, const char *b, size_t n);
int strlen(const char *s);
int tolower(int c);
int strcasecmp(const char *s1, const char *s2);
float bytes_to_gb(uint64_t bytes);
uint8_t bcd_to_bin(uint8_t val);
