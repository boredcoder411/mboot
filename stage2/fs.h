#pragma once

#include <stdint.h>

typedef struct {
  char identifier[4];
  uint32_t num_lumps;
  uint32_t dir_offset;
} __attribute__((packed)) wad_header_t;

typedef struct {
  uint32_t offset;
  uint32_t size;
  char name[8];
} __attribute__((packed)) lump_entry_t;

wad_header_t* init_wad(uint8_t* buffer);
lump_entry_t* init_lumps(wad_header_t* wad);