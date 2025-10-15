#include "mbr.h"
#include "utils.h"

mbr_t* init_mbr(uint8_t* buffer) {
  mbr_t* mbr = (mbr_t*)buffer;
  
  if (mbr->boot_signature != 0xAA55) {
    return NULL;
  }

  return mbr;
}