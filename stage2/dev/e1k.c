#include "dev/e1k.h"
#include "cpu/interrupts/irq.h"
#include "cpu/pic/pic.h"
#include "dev/nic.h"
#include "dev/pci.h"
#include "dev/serial.h"
#include "io.h"
#include "mem.h"
#include "net/arp.h"
#include "net/eth.h"
#include "utils.h"

nic_descriptor nic_e1k;

static const uint32_t e1k_irq_mask =
    E1K_ICR_TXDW | E1K_ICR_LSC | E1K_ICR_RXO | E1K_ICR_RXT0;

static e1k_tx_desc_t *tx_ring = NULL;
static uint8_t (*tx_bufs)[TX_BUF_SIZE] = NULL;
static uint32_t tx_tail = 0;

static e1k_rx_desc_t *rx_ring = NULL;
static uint8_t (*rx_bufs)[RX_BUF_SIZE] = NULL;
static uint32_t rx_tail = 0;

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

static void e1k_program_mac(void) {
  uint32_t ral = ((uint32_t)nic_e1k.mac[0]) |
                 ((uint32_t)nic_e1k.mac[1] << 8) |
                 ((uint32_t)nic_e1k.mac[2] << 16) |
                 ((uint32_t)nic_e1k.mac[3] << 24);
  uint32_t rah = ((uint32_t)nic_e1k.mac[4]) |
                 ((uint32_t)nic_e1k.mac[5] << 8) |
                 E1K_RAH_AV;

  e1k_write(E1K_RAL0, ral);
  e1k_write(E1K_RAH0, rah);

  INFO("E1K", "RAR0 programmed: RAL=0x%08x, RAH=0x%08x", ral, rah);
}

static inline void e1k_mmio_post(void) {
  (void)e1k_read(E1K_TDT);
  asm volatile("" ::: "memory");
}

void e1k_tx_init(void) {
  uintptr_t ring_addr = (uintptr_t)kmalloc(sizeof(e1k_tx_desc_t) * NUM_TX_DESC + 128);
  uintptr_t aligned_ring = align_up_uintptr(ring_addr, 128);
  tx_ring = (e1k_tx_desc_t *)aligned_ring;

  tx_bufs = (uint8_t (*)[TX_BUF_SIZE])kmalloc(sizeof(uint8_t) * NUM_TX_DESC * TX_BUF_SIZE);
  
  INFO("E1K", "TX alloc: ring=0x%08x, bufs=0x%08x", (uint32_t)tx_ring, (uint32_t)tx_bufs);

  memset(tx_ring, 0, sizeof(e1k_tx_desc_t) * NUM_TX_DESC);

  for (int i = 0; i < NUM_TX_DESC; i++) {
    memset(tx_bufs[i], 0, TX_BUF_SIZE);
    tx_ring[i].addr = (uint64_t)(uintptr_t)&tx_bufs[i][0];
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

  INFO("E1K", "TX ring at 0x%08x (align=%d), buf[0] at 0x%08x", (uint32_t)tx_ring,
       (uint32_t)tx_ring % 128, (uint32_t)tx_bufs[0]);
}

void e1k_rx_init(void) {
  // Allocate RX ring with 128-byte alignment
  uintptr_t ring_addr = (uintptr_t)kmalloc(sizeof(e1k_rx_desc_t) * NUM_RX_DESC + 128);
  uintptr_t aligned_ring = align_up_uintptr(ring_addr, 128);
  rx_ring = (e1k_rx_desc_t *)aligned_ring;
  
  // Allocate RX buffers
  rx_bufs = (uint8_t (*)[RX_BUF_SIZE])kmalloc(sizeof(uint8_t) * NUM_RX_DESC * RX_BUF_SIZE);
  
  INFO("E1K", "RX alloc: ring=0x%08x, bufs=0x%08x", (uint32_t)rx_ring, (uint32_t)rx_bufs);

  memset(rx_ring, 0, sizeof(e1k_rx_desc_t) * NUM_RX_DESC);

  for (int i = 0; i < NUM_RX_DESC; i++) {
    memset(rx_bufs[i], 0, RX_BUF_SIZE);
    // Ensure address is within 32-bit DMA range for compatibility
    rx_ring[i].addr = (uint64_t)(uintptr_t)&rx_bufs[i][0];
    rx_ring[i].status = 0;
    rx_ring[i].length = 0;
  }

  uint32_t rdbal = (uint32_t)((uint64_t)rx_ring & 0xFFFFFFFF);
  uint32_t rdbah = (uint32_t)((uint64_t)rx_ring >> 32);
  e1k_write(E1K_RDBAL, rdbal);
  e1k_write(E1K_RDBAH, rdbah);
  e1k_write(E1K_RDLEN, NUM_RX_DESC * sizeof(e1k_rx_desc_t));

  // RCTL: Configure receiver but don't enable yet
  // SZ=0 (2KB buffers), BSEX=0 (legacy mode), SECRC=1 (strip CRC)
  // SBP=1 (store bad packets), BAM=1 (broadcast accept)
  uint32_t rctl = E1K_RCTL_SBP | E1K_RCTL_UPE | E1K_RCTL_MPE |
                  E1K_RCTL_LBM_NONE | E1K_RCTL_RDMTS_HALF | E1K_RCTL_MO_3 |
                  E1K_RCTL_BAM | E1K_RCTL_SECRC;
  e1k_write(E1K_RCTL, rctl);

  // Set head and tail pointers
  e1k_write(E1K_RDH, 0);
  e1k_write(E1K_RDT, NUM_RX_DESC - 1);  // All descriptors initially available

  // Configure RXDCTL for immediate descriptor writeback
  // Set WTHRESH=0 (immediate), HTHRESH=0, PTHRESH=0 for lowest latency
  e1k_write(E1K_RXDCTL, 0);

  rx_tail = 0;

  // Small delay to let RX engine stabilize
  for (volatile int i = 0; i < 1000; i++)
    asm volatile("nop");

  // Enable RXDCTL
  uint32_t rxdctl = e1k_read(E1K_RXDCTL);
  rxdctl |= E1K_RXDCTL_EN;
  e1k_write(E1K_RXDCTL, rxdctl);

  // Now enable the receiver
  rctl |= E1K_RCTL_EN;
  e1k_write(E1K_RCTL, rctl);

  // Disable interrupt throttling for immediate interrupts
  e1k_write(E1K_REG_ITR, 0);
  e1k_write(E1K_REG_RDTR, 0);
  e1k_write(E1K_REG_RADV, 0);

  // Post RX descriptors and ensure they're visible to hardware
  e1k_write(E1K_RDT, NUM_RX_DESC - 1);
  asm volatile("" ::: "memory");

  uint32_t status = e1k_read(E1K_REG_STATUS);
  uint32_t rdh_check = e1k_read(E1K_RDH);
  uint32_t rdt_check = e1k_read(E1K_RDT);
  uint32_t rctl_check = e1k_read(E1K_RCTL);
  uint32_t rdbal_check = e1k_read(E1K_RDBAL);
  uint32_t rdlen_check = e1k_read(E1K_RDLEN);

  INFO("E1K", "RX init: RDBAL=0x%08x, RDBAH=0x%08x, RDLEN=%u", rdbal, rdbah,
       NUM_RX_DESC * sizeof(e1k_rx_desc_t));
  INFO("E1K", "RX check: STATUS=0x%08x, RDH=%u, RDT=%u, RCTL=0x%08x, RDLEN_chk=%u, RDBAL_chk=0x%08x",
       status, rdh_check, rdt_check, rctl_check, rdlen_check, rdbal_check);
  INFO("E1K", "RX ring at 0x%08x (align=%d), buf[0] at 0x%08x", (uint32_t)rx_ring,
       (uint32_t)rx_ring % 128, (uint32_t)rx_bufs[0]);
  INFO("E1K", "RX desc[0].addr = 0x%08x%08x, desc size=%d", 
       (uint32_t)(rx_ring[0].addr >> 32),
       (uint32_t)(rx_ring[0].addr & 0xFFFFFFFF),
       (int)sizeof(e1k_rx_desc_t));
}

static void e1k_process_packet(uint8_t *data, uint16_t len) {
  eth_hdr *eth = (eth_hdr *)data;

  if (len < sizeof(eth_hdr))
    return;

  INFO("E1K", "RX packet: ethertype=0x%04x, len=%u", ntohs(eth->ethertype), len);

  if (ntohs(eth->ethertype) == 0x0806) {
    arp_pkt *arp = (arp_pkt *)(data + sizeof(eth_hdr));
    if (len < sizeof(eth_hdr) + sizeof(arp_pkt))
      return;

    INFO("E1K", "ARP opcode=%u, sender=%d.%d.%d.%d, target=%d.%d.%d.%d",
         ntohs(arp->opcode), arp->sender_ip[0], arp->sender_ip[1],
         arp->sender_ip[2], arp->sender_ip[3], arp->target_ip[0],
         arp->target_ip[1], arp->target_ip[2], arp->target_ip[3]);

    if (ntohs(arp->opcode) == 1) {
      INFO("E1K", "ARP request received - would send reply here");
    }
  }
}

static void e1k_drain_rx(void) {
  int processed = 0;
  while (1) {
    e1k_rx_desc_t *desc = &rx_ring[rx_tail];

    asm volatile("lfence" ::: "memory");
    uint8_t status = desc->status;

    if (!(status & E1K_RXD_STAT_DD)) {
      break;
    }

    if (desc->status & E1K_RXD_STAT_EOP) {
      if (!(desc->errors & E1K_RXD_ERR_FRAME_ERR_MASK) && desc->length > 0) {
        uint16_t pkt_len = desc->length;
        if (pkt_len > RX_BUF_SIZE)
          pkt_len = RX_BUF_SIZE;

        e1k_process_packet(rx_bufs[rx_tail], pkt_len);
        processed++;
      } else {
        INFO("E1K", "RX error: status=0x%02x, errors=0x%02x, len=%u",
             desc->status, desc->errors, desc->length);
      }
    }

    // Clear status and prepare for next use
    desc->status = 0;
    desc->errors = 0;
    desc->length = 0;
    rx_ring[rx_tail].addr = (uint64_t)(uintptr_t)&rx_bufs[rx_tail][0];

    uint32_t completed = rx_tail;
    rx_tail = (rx_tail + 1) % NUM_RX_DESC;

    // RDT points one past the last descriptor owned by hardware, so when we
    // return a consumed descriptor to the NIC we publish the descriptor we
    // just recycled, not the next one we plan to inspect.
    e1k_write(E1K_RDT, completed);
  }

  if (processed > 0) {
    INFO("E1K", "Processed %d packets", processed);
  }
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void handler(registers_t *r) {
  uint32_t icr = e1k_read(E1K_REG_ICR);
  INFO("E1K", "Interrupt ICR=0x%08x", icr);

  if (icr & E1K_ICR_RXT0) {
    INFO("E1K", "RX timer interrupt");
    e1k_drain_rx();
  }
  if (icr & E1K_ICR_TXDW) {
    INFO("E1K", "TX write-back interrupt");
  }
  if (icr & E1K_ICR_LSC) {
    uint32_t status = e1k_read(E1K_REG_STATUS);
    INFO("E1K", "Link status change: %s", (status & (1 << 1)) ? "UP" : "DOWN");
  }
  if (icr & E1K_ICR_RXO) {
    INFO("E1K", "RX overrun");
    e1k_drain_rx();
  }

  e1k_write(E1K_REG_IMS, e1k_irq_mask);
}

void e1k_irq_init(void) {
  install_irq(nic_e1k.desc.irq, handler);
  e1k_read(E1K_REG_ICR);
  e1k_write(E1K_REG_IMS, e1k_irq_mask);
  pic_clear_mask(nic_e1k.desc.irq);
  
  uint32_t status = e1k_read(E1K_REG_STATUS);
  INFO("E1K", "IRQ init: IRQ=%u, STATUS=0x%08x, Link=%s",
       nic_e1k.desc.irq, status, (status & (1 << 1)) ? "UP" : "DOWN");
}

int e1k_send(void *frame, size_t len) {
  if (len > TX_BUF_SIZE) {
    return -1;
  }

  uint32_t hw_tdt = e1k_read(E1K_TDT);
  uint32_t cur = hw_tdt & (NUM_TX_DESC - 1);

  e1k_tx_desc_t *desc = &tx_ring[cur];

  INFO("E1K", "TX send: len=%u, cur=%u, desc status=0x%02x", len, cur,
       desc->status);

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

  INFO("E1K", "TX submitted: idx=%u, next=%u", cur, next);

  wait = 0;
  while (!(desc->status & STATUS_DD)) {
    if (++wait > 10000000) {
      INFO("E1K", "timeout waiting for TX completion (idx=%u)", cur);
      return -3;
    }
  }

  INFO("E1K", "TX complete: idx=%u, status=0x%02x", cur, desc->status);
  tx_tail = next;

  return 0;
}

void e1k_init(nic_descriptor nic_desc) {
  nic_e1k = nic_desc;

  INFO("E1K", "init on %02x:%02x.%x (%04x:%04x)", nic_e1k.desc.bus,
       nic_e1k.desc.device, nic_e1k.desc.function,
       nic_e1k.desc.dev_info.vendor_id, nic_e1k.desc.dev_info.device_id);

  uint16_t cmd = pci_config_read_word(&nic_e1k.desc, 0x04);
  cmd |= (1 << 2);  // Bus mastering
  cmd |= (1 << 0);  // I/O space
  cmd |= (1 << 1);  // Memory space

  pci_config_write_word(&nic_e1k.desc, 0x04, cmd);

  uint16_t cmd_check = pci_config_read_word(&nic_e1k.desc, 0x04);
  if (!(cmd_check & (1 << 2))) {
    INFO("E1K", "Warning: failed to enable bus mastering (cmd=0x%04x)",
         cmd_check);
  }

  uint8_t irq_line = pci_config_read_word(&nic_e1k.desc, 0x3C) & 0xFF;
  uint8_t irq_pin = pci_config_read_word(&nic_e1k.desc, 0x3D) & 0xFF;
  INFO("E1K", "PCI IRQ: line=%u, pin=%u", irq_line, irq_pin);

  uint16_t msi_cap = pci_config_read_word(&nic_e1k.desc, 0x50);
  INFO("E1K", "MSI capability: 0x%04x", msi_cap);
  if (msi_cap & 0x1) {
    INFO("E1K", "MSI is enabled, disabling for legacy IRQ");
    uint16_t msi_ctrl = msi_cap & ~0x1U;
    pci_config_write_word(&nic_e1k.desc, 0x50, msi_ctrl);
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

  uint32_t ctrl = e1k_read(E1K_REG_CTRL);
  ctrl |= E1K_CTRL_SLU | E1K_CTRL_FRCSPD | E1K_CTRL_FRCDPX | E1K_CTRL_FD;
  e1k_write(E1K_REG_CTRL, ctrl);

  uint32_t status = e1k_read(E1K_REG_STATUS);
  INFO("E1K", "STATUS = 0x%08x, CTRL = 0x%08x", status, ctrl);

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

  e1k_program_mac();

  e1k_tx_init();
  e1k_rx_init();
  e1k_irq_init();
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
  } else {
    INFO("E1K", "ARP request sent successfully");
  }

  kfree(frame);
}
