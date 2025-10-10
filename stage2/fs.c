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
