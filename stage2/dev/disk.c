#include "dev/disk.h"
#include "io.h"
#include "utils.h"

#define ATA_PRIMARY_IO 0x1F0
#define ATA_REG_STATUS (ATA_PRIMARY_IO + 7)
#define ATA_REG_ALTSTATUS 0x3F6

void ata_wait_busy_clear() {
  while (inb(ATA_REG_STATUS) & 0x80)
    ;
}

void ata_wait_drq_set() {
  while (!(inb(ATA_REG_STATUS) & 0x08))
    ;
}

void ata_lba_read(uint32_t lba, uint8_t sector_count, void *buffer,
                  uint8_t drive) {
  uint8_t drive_select = (drive & 1) ? 0xF0 : 0xE0;

  outb(0x1F6, drive_select | ((lba >> 24) & 0x0F));

  outb(0x1F2, sector_count);
  outb(0x1F3, (uint8_t)lba);
  outb(0x1F4, (uint8_t)(lba >> 8));
  outb(0x1F5, (uint8_t)(lba >> 16));

  outb(0x1F7, 0x20);

  for (uint8_t s = 0; s < sector_count; s++) {
    uint8_t status;

    do {
      status = inb(ATA_REG_STATUS);
      if (status & 0x01) {
        HALT()
        return;
      }
    } while ((status & 0x88) != 0x08);

    insw(ATA_PRIMARY_IO, (uint16_t *)buffer + (s * 256), 256);
  }
}
