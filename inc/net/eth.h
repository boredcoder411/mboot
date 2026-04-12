#pragma once

#include <stdint.h>

typedef struct {
  uint8_t dst[6];
  uint8_t src[6];
  uint16_t ethertype;
} __attribute__((packed)) eth_hdr;

void eth_process_packet(uint8_t *data, uint16_t len);
