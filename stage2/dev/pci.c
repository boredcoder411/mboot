#include "dev/pci.h"
#include "dev/serial.h"
#include "io.h"
#include <stddef.h>
#include <stdint.h>

pci_device_info_t pci_device_table[] = {
    {0x10EC, 0x8029, "NE2000 - NIC"},
    {0x8086, 0x100E, "E1000 - NIC"},
    {0x8086, 0x1237, "Intel 82440FX - Host Bridge"},
    {0x8086, 0x7000, "Intel 82371SB - ISA Bridge"},
};

#define PCI_DEVICE_TABLE_SIZE                                                  \
  (sizeof(pci_device_table) / sizeof(pci_device_table[0]))

uint32_t pci_config_read(uint8_t bus, uint8_t slot, uint8_t func,
                         uint8_t offset) {
  uint32_t address = (1U << 31) | ((uint32_t)bus << 16) |
                     ((uint32_t)slot << 11) | ((uint32_t)func << 8) |
                     (offset & 0xFC);
  outl(PCI_CONFIG_ADDRESS, address);
  return inl(PCI_CONFIG_DATA);
}

uint16_t pci_config_read_word(uint8_t bus, uint8_t device, uint8_t func,
                              uint8_t offset) {
  uint32_t data = pci_config_read(bus, device, func, offset & 0xFC);
  return (uint16_t)((data >> ((offset & 2) * 8)) & 0xFFFF);
}

void pci_config_write_word(uint8_t bus, uint8_t device, uint8_t func,
                           uint8_t offset, uint16_t value) {
  uint32_t aligned_offset = offset & 0xFC;

  uint32_t address = (1U << 31) | ((uint32_t)bus << 16) |
                     ((uint32_t)device << 11) | ((uint32_t)func << 8) |
                     aligned_offset;

  outl(PCI_CONFIG_ADDRESS, address);
  uint32_t data = inl(PCI_CONFIG_DATA);

  if ((offset & 2) == 0)
    data = (data & 0xFFFF0000) | value;
  else
    data = (data & 0x0000FFFF) | ((uint32_t)value << 16);

  outl(PCI_CONFIG_ADDRESS, address);
  outl(PCI_CONFIG_DATA, data);
}

void pci_enable_busmaster(uint8_t bus, uint8_t dev, uint8_t func) {
  uint16_t cmd = pci_config_read_word(bus, dev, func, PCI_COMMAND);
  cmd |= (1 << 2);
  uint32_t address = (1U << 31) | ((uint32_t)bus << 16) |
                     ((uint32_t)dev << 11) | ((uint32_t)func << 8) |
                     (PCI_COMMAND & 0xFC);
  outl(0xCF8, address);
  outl(0xCFC, (cmd | ((uint32_t)cmd << 16)));
}

inline uint32_t pci_read_bar0(uint8_t bus, uint8_t dev, uint8_t func) {
  return pci_config_read(bus, dev, func, PCI_BAR0);
}

const char *pci_lookup_device(uint16_t vendor_id, uint16_t device_id) {
  for (size_t i = 0; i < PCI_DEVICE_TABLE_SIZE; i++) {
    if (pci_device_table[i].vendor_id == vendor_id &&
        pci_device_table[i].device_id == device_id) {
      return pci_device_table[i].name;
    }
  }
  return NULL;
}

void pci_enumerate() {
  for (uint8_t bus = 0; bus < 255; bus++) {
    for (uint8_t device = 0; device < 32; device++) {
      for (uint8_t func = 0; func < 8; func++) {

        uint16_t vendor = pci_config_read_word(bus, device, func, 0x00);
        if (vendor == 0xFFFF)
          continue;

        uint16_t device_id = pci_config_read_word(bus, device, func, 0x02);
        uint8_t class_code =
            (pci_config_read(bus, device, func, 0x08) >> 24) & 0xFF;
        uint8_t subclass =
            (pci_config_read(bus, device, func, 0x08) >> 16) & 0xFF;

        serial_printf("PCI Device found: bus=%u, dev=%u, func=%u\n", bus,
                      device, func);
        serial_printf("  Vendor ID: 0x%04X, Device ID: 0x%04X\n", vendor,
                      device_id);
        serial_printf("  Class: 0x%02X, Subclass: 0x%02X\n", class_code,
                      subclass);

        const char *name = pci_lookup_device(vendor, device_id);
        if (name)
          serial_printf("  -> Device: %s\n", name);

        pci_handle_device(bus, device, func, vendor, device_id);
      }
    }
  }
}
