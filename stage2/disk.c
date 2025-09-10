#include "disk.h"
#include "io.h"

void ata_lba_read(uint32_t lba, uint8_t sector_count, void* buffer, uint8_t drive) {
    uint8_t drive_select = 0xE0;

    switch (drive) {
        case 0:
            drive_select = 0xE0;
            break;
        case 1:
            drive_select = 0xF0;
            break;
        case 2:
            drive_select = 0xE0;
            break;
        case 3:
            drive_select = 0xF0;
            break;
        default:
            drive_select = 0xE0;
            break;
    }

    outb(0x01F6, (drive_select | ((lba >> 24) & 0x0F)));

    outb(0x01F2, sector_count);

    outb(0x1F3, (uint8_t)lba);

    outb(0x1F4, (uint8_t)(lba >> 8));

    outb(0x1F5, (uint8_t)(lba >> 16));

    outb(0x1F7, 0x20);

    while (!(inb(0x1F7) & 0x08)) {
    }

    insw(0x1F0, buffer, sector_count * 256);
}
