#pragma once

#include <stdint.h>

typedef enum {
  WAD_IWAD,
  WAD_PWAD
} wad_type_t;

static const char wad_type_names[2][4] = {
    { 'I', 'W', 'A', 'D' },
    { 'P', 'W', 'A', 'D' }
};

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
void *find_file(char name[8], wad_header_t *wad);
