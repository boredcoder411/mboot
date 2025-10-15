#include "fs.h"
#include "utils.h"

wad_header_t* init_wad(uint8_t* buffer) {
    if(!buffer) {
        return NULL;
    }

    wad_header_t* header = (wad_header_t*)buffer;

    if(strncmp(header->identifier, "IWAD", 4) != 0 && strncmp(header->identifier, "PWAD", 4) != 0) {
        return NULL;
    }

    return header;
}

lump_entry_t* init_lumps(wad_header_t* wad) {
    if(!wad) {
        return NULL;
    }

    return (lump_entry_t*)((uint8_t*)wad + wad->dir_offset);
}

// FIXME: for now ts function assumes the entire filesystem is loaded in mem
void *find_file(char name[8], wad_header_t *wad) {
  uint32_t found = 0;
  lump_entry_t *entries = (lump_entry_t *)((uint8_t *)wad + wad->dir_offset);

  for (uint32_t i = 0; i < wad->num_lumps; i++) {
    if (strncmp(entries[i].name, name, 8)) {
      found = i;
      break;
    }
  }

  if (found == wad->num_lumps) {
    return 0;
  }

  void *file = (void *)((uint8_t *)wad + entries[found].offset);
  return file;
}
