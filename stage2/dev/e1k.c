#include "dev/e1k.h"
#include "dev/nic.h"
#include "dev/pci.h"
#include "dev/serial.h"
#include "io.h"
#include "mem.h"
#include "net/arp.h"
#include "net/eth.h"
#include "utils.h"

nic_descriptor nic_e1k;

static e1k_tx_desc_t tx_ring[NUM_TX_DESC] __attribute__((aligned(16)));
static uint8_t tx_bufs[NUM_TX_DESC][TX_BUF_SIZE] __attribute__((aligned(16)));
static uint32_t tx_tail = 0;

static inline int e1k_is_mmio(void) { return !(nic_e1k.desc.bar[0] & 0x1); }

inline void e1k_write(uint32_t reg, uint32_t val) {
  if (e1k_is_mmio()) {
    *(volatile uint32_t *)((uintptr_t)nic_e1k.desc.io_base + reg) = val;
  } else {
    uint16_t port = (uint16_t)((uintptr_t)nic_e1k.desc.io_base + reg);
    outl(port, val);
  }
}

inline uint32_t e1k_read(uint32_t reg) {
  if (e1k_is_mmio()) {
    return *(volatile uint32_t *)((uintptr_t)nic_e1k.desc.io_base + reg);
  } else {
    uint16_t port = (uint16_t)((uintptr_t)nic_e1k.desc.io_base + reg);
    return inl(port);
  }
}

int e1k_detect_eeprom(void) {
  uint32_t val;

  for (int i = 0; i < 1000; i++) {
    val = e1k_read(E1K_REG_EECD);
    if (val & E1K_EECD_EE_PRES)
      return 1;
  }

  return 0;
}

uint16_t e1k_read_eeprom(uint16_t addr) {
  e1k_write(E1K_REG_EERD, (addr << 8) | E1K_EERD_START);
  int timeout = 0;
  while (!(e1k_read(E1K_REG_EERD) & E1K_EERD_DONE)) {
    if (++timeout > 100000) {
      INFO("E1K", "EEPROM read timeout at addr %u", addr);
      return 0xFFFF;
    }
  }

  return (uint16_t)(e1k_read(E1K_REG_EERD) >> 16);
}

void e1k_read_mac() {
  for (uint16_t i = 0; i < 3; i++) {
    uint16_t word = e1k_read_eeprom(i);

    if (word == 0xFFFF) {
      continue;
    }

    nic_e1k.mac[i * 2] = word & 0xFF;
    nic_e1k.mac[i * 2 + 1] = (word >> 8) & 0xFF;
  }
}

static inline void e1k_mmio_post(void) {
  (void)e1k_read(E1K_TDT);
  asm volatile("" ::: "memory");
}

void e1k_tx_init(void) {
  memset(tx_ring, 0, sizeof(tx_ring));

  for (int i = 0; i < NUM_TX_DESC; i++) {
    memset(tx_bufs[i], 0, TX_BUF_SIZE);
    tx_ring[i].addr = (uint64_t)&tx_bufs[i][0];
    tx_ring[i].status = STATUS_DD;
  }

  uint32_t tdbal = (uint32_t)((uint64_t)tx_ring & 0xFFFFFFFF);
  uint32_t tdbah = (uint32_t)((uint64_t)tx_ring >> 32);
  e1k_write(E1K_TDBAL, tdbal);
  e1k_write(E1K_TDBAH, tdbah);
  e1k_write(E1K_TDLEN, NUM_TX_DESC * sizeof(e1k_tx_desc_t));

  e1k_write(E1K_TDH, 0);
  e1k_write(E1K_TDT, 0);
  e1k_mmio_post();

  tx_tail = 0;

  uint32_t tctl = E1K_TCTL_EN | E1K_TCTL_PSP | (0x10 << E1K_TCTL_CT_SHIFT) |
                  (0x40 << E1K_TCTL_COLD_SHIFT);
  e1k_write(E1K_TCTL, tctl);

  e1k_write(E1K_TIPG, 0x0060200A);
}

int e1k_send(void *frame, size_t len) {
  if (len > TX_BUF_SIZE) {
    return -1;
  }

  uint32_t hw_tdt = e1k_read(E1K_TDT);
  uint32_t cur = hw_tdt & (NUM_TX_DESC - 1);

  e1k_tx_desc_t *desc = &tx_ring[cur];

  int wait = 0;
  while (!(desc->status & STATUS_DD)) {
    if (++wait > 2000000) {
      INFO("E1K", "timeout waiting for free TX descriptor (idx=%u)", cur);
      return -2;
    }
    asm volatile("pause");
  }

  memcpy(tx_bufs[cur], frame, len);

  desc->length = (uint16_t)len;
  asm volatile("" ::: "memory");
  desc->cmd = CMD_EOP | CMD_IFCS | CMD_RS;
  desc->status = 0;

  uint32_t next = (cur + 1) % NUM_TX_DESC;
  e1k_write(E1K_TDT, next);
  e1k_mmio_post();

  wait = 0;
  while (!(desc->status & STATUS_DD)) {
    if (++wait > 10000000) {
      INFO("E1K", "timeout waiting for TX completion (idx=%u)", cur);
      return -3;
    }
  }

  tx_tail = next;

  return 0;
}

void e1k_init(nic_descriptor nic_desc) {
  nic_e1k = nic_desc;

  INFO("E1K", "init on %02x:%02x.%x (%04x:%04x)", nic_e1k.desc.bus,
       nic_e1k.desc.device, nic_e1k.desc.function,
       nic_e1k.desc.dev_info.vendor_id, nic_e1k.desc.dev_info.device_id);

  uint16_t cmd = pci_config_read_word(&nic_e1k.desc, 0x04);
  cmd |= (1 << 2);

  pci_config_write_word(&nic_e1k.desc, 0x04, cmd);

  uint16_t cmd_check = pci_config_read_word(&nic_e1k.desc, 0x04);
  if (!(cmd_check & (1 << 2))) {
    INFO("E1K", "Warning: failed to enable bus mastering (cmd=0x%04x)",
         cmd_check);
  }

  uint32_t bar0 = nic_e1k.desc.bar[0];
  uint64_t mmio_base_64 = 0;
  if (bar0 & 0x1) {
    uint32_t io_base = bar0 & ~0x3U;
    nic_e1k.desc.io_base = (uintptr_t)io_base;
    INFO("E1K", "IO BAR detected: 0x%08x", io_base);
  } else {
    uint32_t bar1 = nic_e1k.desc.bar[1];
    mmio_base_64 = (((uint64_t)bar1) << 32) | (bar0 & ~0xFULL);

    uint32_t mmio_base = (uint32_t)mmio_base_64;
    nic_e1k.desc.io_base = (uintptr_t)mmio_base;
    INFO("E1K", "MMIO base = 0x%08x", mmio_base);
  }

  e1k_write(E1K_REG_CTRL, E1K_CTRL_RST);
  int timeout = 0;
  while (e1k_read(E1K_REG_CTRL) & E1K_CTRL_RST) {
    if (++timeout > 1000000) {
      INFO("E1K", "device reset timeout");
      break;
    }
  }

  uint32_t status = e1k_read(E1K_REG_STATUS);
  INFO("E1K", "STATUS = 0x%08x", status);

  int has_eeprom = e1k_detect_eeprom();
  INFO("E1K", "EEPROM %sdetected", has_eeprom ? "" : "not ");

  if (has_eeprom) {
    e1k_read_mac();
  } else {
    uint32_t ral = e1k_read(0x5400);
    uint32_t rah = e1k_read(0x5404);
    nic_e1k.mac[0] = ral & 0xFF;
    nic_e1k.mac[1] = (ral >> 8) & 0xFF;
    nic_e1k.mac[2] = (ral >> 16) & 0xFF;
    nic_e1k.mac[3] = (ral >> 24) & 0xFF;
    nic_e1k.mac[4] = rah & 0xFF;
    nic_e1k.mac[5] = (rah >> 8) & 0xFF;
  }

  INFO("E1K", "MAC %02x:%02x:%02x:%02x:%02x:%02x", nic_e1k.mac[0],
       nic_e1k.mac[1], nic_e1k.mac[2], nic_e1k.mac[3], nic_e1k.mac[4],
       nic_e1k.mac[5]);

  e1k_tx_init();
}

void e1k_send_arp_request(uint8_t src_ip[4], uint8_t target_ip[4]) {
  size_t frame_len = sizeof(eth_hdr) + sizeof(arp_pkt);
  if (frame_len < 60)
    frame_len = 60;

  uint8_t *frame = kmalloc(frame_len);
  memset(frame, 0, frame_len);

  eth_hdr *eth = (eth_hdr *)frame;
  memset(eth->dst, 0xFF, 6);
  memcpy(eth->src, nic_e1k.mac, 6);
  eth->ethertype = htons(0x0806);

  arp_pkt *arp = (arp_pkt *)(frame + sizeof(eth_hdr));
  arp->htype = htons(1);
  arp->ptype = htons(0x0800);
  arp->hlen = 6;
  arp->plen = 4;
  arp->opcode = htons(1);

  memcpy(arp->sender_mac, nic_e1k.mac, 6);
  memcpy(arp->sender_ip, src_ip, 4);
  memset(arp->target_mac, 0x00, 6);
  memcpy(arp->target_ip, target_ip, 4);

  INFO("E1K", "Sending ARP request: who has %d.%d.%d.%d ? tell %d.%d.%d.%d",
       target_ip[0], target_ip[1], target_ip[2], target_ip[3], src_ip[0],
       src_ip[1], src_ip[2], src_ip[3]);

  int r = e1k_send(frame, frame_len);
  if (r != 0) {
    INFO("E1K", "e1k_send failed with %d", r);
  }

  kfree(frame);
}
