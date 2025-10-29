#pragma once

#include <stdint.h>

#define MBR_SIG 0xAA55

typedef struct {
  uint8_t status;
  uint8_t first_chs[3];
  uint8_t type;
  uint8_t last_chs[3];
  uint32_t first_lba;
  uint32_t sector_count;
} __attribute__((packed)) partition_entry_t;

typedef struct {
  uint8_t bootstrap_code[446];
  partition_entry_t partitions[4];
  uint16_t boot_signature;
} __attribute__((packed)) mbr_t;

mbr_t *init_mbr(uint8_t *buffer);
