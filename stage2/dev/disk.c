// ATA PIO sector reads. A scheduler-aware lock serializes command sequences; the active transfer polls the device.
#include "dev/disk.h"
#include "dev/serial.h"
#include "io.h"
#include "scheduler.h"
#include "utils.h"

static volatile int disk_mutex = 0;

/* Disable interrupts while checking and claiming the lock on this single
   CPU. A contender sleeps on the lock address until disk_unlock wakes it. */
static void disk_lock(void) {
  CLI();
  while (disk_mutex) {
    scheduler_block((void *)&disk_mutex);
    CLI();
  }
  disk_mutex = 1;
  STI();
}

static void disk_unlock(void) {
  CLI();
  disk_mutex = 0;
  scheduler_wake((void *)&disk_mutex);
  STI();
}

#define ATA_PRIMARY_IO 0x1F0
#define ATA_REG_STATUS (ATA_PRIMARY_IO + 7)
#define ATA_REG_ALTSTATUS 0x3F6

void ata_wait_busy_clear() {
  /* BSY must clear before programming a command. Unlike the transfer loop
     below, this preliminary wait has no timeout or error return. */
  while (inb(ATA_REG_STATUS) & 0x80)
    ;
}

void ata_wait_drq_set() {
  while (!(inb(ATA_REG_STATUS) & 0x08))
    ;
}

void ata_lba_read(uint32_t lba, uint8_t sector_count, void *buffer,
                  uint8_t drive) {
  disk_lock();
  uint8_t drive_select = (drive & 1) ? 0xF0 : 0xE0;

  ata_wait_busy_clear();

  /* Program the 28-bit LBA and sector count, then issue READ SECTORS. */
  outb(0x1F6, drive_select | ((lba >> 24) & 0x0F));
  outb(0x1F2, sector_count);
  outb(0x1F3, (uint8_t)lba);
  outb(0x1F4, (uint8_t)(lba >> 8));
  outb(0x1F5, (uint8_t)(lba >> 16));
  outb(0x1F7, 0x20);

  for (uint8_t s = 0; s < sector_count; s++) {
    uint8_t status;
    uint32_t timeout = 10000000;

    /* Wait for BSY=0 and DRQ=1, or stop on ERR/timeout. This polls instead
       of sleeping until a disk interrupt; the void API only logs failure. */
    do {
      status = inb(ATA_REG_STATUS);
      if (status & 0x01) {
        WARN("DISK", "ATA error on LBA %u", lba + s);
        disk_unlock();
        return;
      }
      if (--timeout == 0) {
        WARN("DISK", "ATA timeout on LBA %u (status=0x%02x)", lba + s, status);
        disk_unlock();
        return;
      }
    } while ((status & 0x88) != 0x08);

    /* One sector is 512 bytes: transfer 256 words from the ATA data port. */
    insw(ATA_PRIMARY_IO, (uint16_t *)buffer + (s * 256), 256);
  }
  disk_unlock();
}
