#pragma once

#include <stdint.h>

void ata_lba_read(uint32_t lba, uint8_t sector_count, void *buffer,
                  uint8_t drive);