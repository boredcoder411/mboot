#include "dev/pci.h"
#include "dev/serial.h"
#include "io.h"

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

        if (vendor == 0x10EC && device_id == 0x8029) {
          serial_printf("  -> Found NE2000 (Realtek RTL-8029) network card!\n");
        }
      }
    }
  }
}
