#include "dev/ne2k.h"
#include "cpu/interrupts/irq.h"
#include "cpu/pic/pic.h"
#include "dev/pci.h"
#include "dev/serial.h"
#include "io.h"
#include "mem.h"
#include "net/arp.h"
#include "net/eth.h"
#include "utils.h"
#include <stdint.h>

uint32_t io_base;
uint8_t mac[6];

#pragma GCC diagnostic ignored "-Wunused-parameter"
void ne2k_handler(registers_t *r) {
  uint8_t isr = inb(io_base + 0x07);

  if (isr & 0x01) {
  }
  if (isr & 0x02) {
  }

  outb(io_base + 0x07, isr);
}

static int ne2k_read_mac(uint16_t io_base) {
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
    ERROR("NE2k", "BAR0 is not I/O type (0x%08X)", bar0);
    return;
  }

  io_base = bar0 & ~0x3U;
  INFO("NE2k", "IO base: 0x%04X", io_base);

  if (ne2k_read_mac(io_base) == 0) {
    INFO("NE2k", "MAC: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2],
         mac[3], mac[4], mac[5]);
  } else {
    WARN("NE2k", "Failed to read MAC address.");
  }

  uint8_t irq = pci_config_read(bus, device, func, 0x3C) & 0xFF;
  INFO("NE2k", "PCI IRQ: %d", irq);

  pic_clear_mask(irq);

  install_irq(irq, ne2k_handler);

  outb(io_base + NE2K_IMR, 0x1F);

  outb(io_base + NE2K_CMD, CMD_STA);
}

void ne2k_send(uint16_t io_base, const uint8_t *frame, uint16_t len) {
  if (len < 60)
    len = 60;

  const uint8_t tx_page_start = 0x40;

  outb(io_base + NE2K_CR, CMD_STA | CMD_RD0);
  outb(io_base + NE2K_RSAR0, 0x00);
  outb(io_base + NE2K_RSAR1, tx_page_start);
  outb(io_base + NE2K_RBCR0, len & 0xFF);
  outb(io_base + NE2K_RBCR1, len >> 8);

  for (uint16_t i = 0; i < len; i++)
    outb(io_base + NE2K_RDMAPORT, frame[i]);

  while (!(inb(io_base + NE2K_ISR) & (1 << 6)))
    ;
  outb(io_base + NE2K_ISR, (1 << 6));

  outb(io_base + NE2K_TPSR, tx_page_start);
  outb(io_base + NE2K_TBCR0, len & 0xFF);
  outb(io_base + NE2K_TBCR1, len >> 8);

  outb(io_base + NE2K_CR, CMD_STA | CMD_TXP);

  INFO("NE2K", "Sent %u bytes", len);
}

void ne2k_send_arp_request() {
  uint8_t frame[60];
  struct eth_hdr *eth = (struct eth_hdr *)frame;
  struct arp_pkt *arp = (struct arp_pkt *)(frame + sizeof(struct eth_hdr));

  memset(eth->dst, 0xFF, 6);
  memcpy(eth->src, (void *)mac, 6);
  eth->ethertype = htons(0x0806);

  arp->htype = htons(1);
  arp->ptype = htons(0x0800);
  arp->hlen = 6;
  arp->plen = 4;
  arp->opcode = htons(1);

  memcpy(arp->sender_mac, (void *)mac, 6);
  uint8_t my_ip[4] = {10, 0, 2, 15};
  memcpy(arp->sender_ip, my_ip, 4);

  memset(arp->target_mac, 0x00, 6);
  uint8_t target_ip[4] = {10, 0, 2, 2};
  memcpy(arp->target_ip, target_ip, 4);

  memset(frame + sizeof(struct eth_hdr) + sizeof(struct arp_pkt), 0,
         60 - (sizeof(struct eth_hdr) + sizeof(struct arp_pkt)));

  ne2k_send(io_base, frame, 60);

  INFO("NE2K", "ARP request sent for 10.0.2.2");
}
