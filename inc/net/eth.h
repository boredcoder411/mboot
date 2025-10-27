#pragma once

#include <stdint.h>

typedef struct __attribute__((packed)) {
  uint8_t dst[6];
  uint8_t src[6];
  uint16_t ethertype;
} eth_hdr;
