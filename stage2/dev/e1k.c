#include "dev/e1k.h"
#include "dev/pci.h"
#include "dev/serial.h"
#include "dev/nic.h"
#include "io.h"

nic_descriptor nic_e1k;

inline void e1k_write(uint32_t reg, uint32_t val) {
  *(volatile uint32_t *)((uintptr_t)nic_e1k.desc.io_base + reg) = val;
}

inline uint32_t e1k_read(uint32_t reg) {
  return *(volatile uint32_t *)((uintptr_t)nic_e1k.desc.io_base + reg);
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
  while (!(e1k_read(E1K_REG_EERD) & E1K_EERD_DONE))
    ;
  return (uint16_t)(e1k_read(E1K_REG_EERD) >> 16);
}

void e1k_read_mac() {
  for (uint16_t i = 0; i < 3; i++) {
    uint16_t word = e1k_read_eeprom(i);
    nic_e1k.mac[i * 2] = word & 0xFF;
    nic_e1k.mac[i * 2 + 1] = (word >> 8) & 0xFF;
  }
}

void e1k_init(uint8_t bus, uint8_t device, uint8_t func, uint16_t vendor,
              uint16_t device_id) {
  INFO("E1K", "init on %02x:%02x.%x (%04x:%04x)", bus, device, func, vendor,
       device_id);

  uint16_t cmd = pci_config_read_word(bus, device, func, 0x04);
  cmd |= (1 << 2);
  uint32_t address = (1U << 31) | ((uint32_t)bus << 16) |
                     ((uint32_t)device << 11) | ((uint32_t)func << 8) |
                     (0x04 & 0xFC);
  outl(0xCF8, address);
  outl(0xCFC, (cmd | ((uint32_t)cmd << 16)));

  uint32_t bar0 = pci_config_read(bus, device, func, 0x10);
  uint32_t mmio_base = bar0 & ~0xF;
  nic_e1k.desc.io_base = mmio_base;
  INFO("E1K", "MMIO base = 0x%08x", mmio_base);

  e1k_write(E1K_REG_CTRL, E1K_CTRL_RST);
  while (e1k_read(E1K_REG_CTRL) & E1K_CTRL_RST) {
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

  INFO("E1K", "MAC %02x:%02x:%02x:%02x:%02x:%02x", nic_e1k.mac[0], nic_e1k.mac[1], nic_e1k.mac[2],
       nic_e1k.mac[3], nic_e1k.mac[4], nic_e1k.mac[5]);
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void e1k_send(uint16_t io_base, const uint8_t *frame, uint16_t len) {}
