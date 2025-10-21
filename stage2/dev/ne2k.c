#include "dev/ne2k.h"
#include "dev/pci.h"
#include "dev/serial.h"
#include "io.h"
#include <stdint.h>

#define NE2K_CMD 0x00
#define NE2K_RSAR0 0x08
#define NE2K_RSAR1 0x09
#define NE2K_RBCR0 0x0A
#define NE2K_RBCR1 0x0B
#define NE2K_RDMAPORT 0x10

#define CMD_STP 0x01
#define CMD_STA 0x02
#define CMD_RD0 0x08

static int ne2k_read_mac(uint16_t io_base, uint8_t mac[6]) {
  uint8_t buf[32];

  outb(io_base + NE2K_CMD, CMD_STP);

  outb(io_base + NE2K_RSAR0, 0x00);
  outb(io_base + NE2K_RSAR1, 0x00);
  outb(io_base + NE2K_RBCR0, 32);
  outb(io_base + NE2K_RBCR1, 0x00);

  outb(io_base + NE2K_CMD, CMD_RD0 | CMD_STA);

  for (int i = 0; i < 32; i++)
    buf[i] = inb(io_base + NE2K_RDMAPORT);

  outb(io_base + NE2K_CMD, CMD_STP);

  int valid = 0;
  for (int i = 0; i < 6; i++)
    if (buf[i] != 0xFF)
      valid = 1;

  if (!valid)
    return -1;

  for (int i = 0; i < 6; i++)
    mac[i] = buf[i];

  return 0;
}

void ne2k_init(uint8_t bus, uint8_t device, uint8_t func, uint16_t vendor,
               uint16_t device_id) {
  if (vendor != 0x10EC || device_id != 0x8029)
    return;

  uint32_t bar0 = pci_config_read(bus, device, func, 0x10);
  if (!(bar0 & 0x1)) {
    serial_printf("  [NE2K] BAR0 is not I/O type (0x%08X)\n", bar0);
    return;
  }

  uint16_t io_base = bar0 & ~0x3U;
  serial_printf("  [NE2K] IO base: 0x%04X\n", io_base);

  uint8_t mac[6];
  if (ne2k_read_mac(io_base, mac) == 0) {
    serial_printf("  [NE2K] MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0],
                  mac[1], mac[2], mac[3], mac[4], mac[5]);
  } else {
    serial_printf("  [NE2K] Failed to read MAC address.\n");
  }
}
