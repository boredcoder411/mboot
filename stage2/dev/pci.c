#include "dev/pci.h"
#include "dev/serial.h"
#include "io.h"
#include "mem.h"
#include <stddef.h>
#include <stdint.h>

pci_device_info_t pci_device_table[] = {
    {0x8086, 0x100E, "E1000 - NIC"},
    {0x8086, 0x1237, "Intel 82440FX - Host Bridge"},
    {0x8086, 0x7000, "Intel 82371SB - ISA Bridge"},
};

#define PCI_DEVICE_TABLE_SIZE                                                  \
  (sizeof(pci_device_table) / sizeof(pci_device_table[0]))

uint32_t pci_config_read_raw(uint8_t bus, uint8_t slot, uint8_t func,
                             uint8_t offset) {
  uint32_t address = (1U << 31) | ((uint32_t)bus << 16) |
                     ((uint32_t)slot << 11) | ((uint32_t)func << 8) |
                     (offset & 0xFC);
  outl(PCI_CONFIG_ADDRESS, address);
  return inl(PCI_CONFIG_DATA);
}

uint16_t pci_config_read_word_raw(uint8_t bus, uint8_t device, uint8_t func,
                                  uint8_t offset) {
  uint32_t data = pci_config_read_raw(bus, device, func, offset & 0xFC);
  return (uint16_t)((data >> ((offset & 2) * 8)) & 0xFFFF);
}

void pci_config_write_word_raw(uint8_t bus, uint8_t device, uint8_t func,
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

uint32_t pci_config_read(const pci_device_desc_t *dev, uint8_t offset) {
  return pci_config_read_raw(dev->bus, dev->device, dev->function, offset);
}

uint16_t pci_config_read_word(const pci_device_desc_t *dev, uint8_t offset) {
  return pci_config_read_word_raw(dev->bus, dev->device, dev->function, offset);
}

void pci_config_write_word(const pci_device_desc_t *dev, uint8_t offset,
                           uint16_t value) {
  return pci_config_write_word_raw(dev->bus, dev->device, dev->function, offset,
                                   value);
}

void pci_enable_busmaster(uint8_t bus, uint8_t dev, uint8_t func) {
  uint16_t cmd = pci_config_read_word_raw(bus, dev, func, PCI_COMMAND);
  cmd |= (1 << 2);
  uint32_t address = (1U << 31) | ((uint32_t)bus << 16) |
                     ((uint32_t)dev << 11) | ((uint32_t)func << 8) |
                     (PCI_COMMAND & 0xFC);
  outl(0xCF8, address);
  outl(0xCFC, (cmd | ((uint32_t)cmd << 16)));
}

void pci_read_bars(pci_device_desc_t *desc, uint32_t bars[6]) {
  bars[0] =
      pci_config_read_raw(desc->bus, desc->device, desc->function, PCI_BAR0);
  bars[1] =
      pci_config_read_raw(desc->bus, desc->device, desc->function, PCI_BAR1);
  bars[2] =
      pci_config_read_raw(desc->bus, desc->device, desc->function, PCI_BAR2);
  bars[3] =
      pci_config_read_raw(desc->bus, desc->device, desc->function, PCI_BAR3);
  bars[4] =
      pci_config_read_raw(desc->bus, desc->device, desc->function, PCI_BAR4);
  bars[5] =
      pci_config_read_raw(desc->bus, desc->device, desc->function, PCI_BAR5);
}

uint32_t pci_detect_iobase(pci_device_desc_t *desc) {
  for (int i = 0; i < 6; i++) {
    uint32_t bar = desc->bar[i];
    if (bar == 0)
      continue;

    if (bar & 0x1) {
      uint32_t iobase = bar & ~0x3;
      desc->io_base = iobase;
      serial_printf("  -> IOBase detected at 0x%X (BAR%d)\n", iobase, i);
      return iobase;
    }
  }

  serial_printf("  -> No IOBase found (device uses MMIO only)\n");
  return 0;
}

uint8_t pci_detect_irq(pci_device_desc_t *desc) {
  uint32_t val = pci_config_read_word_raw(desc->bus, desc->device,
                                          desc->function, PCI_INTERRUPT_LINE);
  return val & 0xFF;
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

        uint16_t vendor = pci_config_read_word_raw(bus, device, func, 0x00);
        if (vendor == 0xFFFF)
          continue;

        uint16_t device_id = pci_config_read_word_raw(bus, device, func, 0x02);
        uint8_t class_code =
            (pci_config_read_raw(bus, device, func, 0x08) >> 24) & 0xFF;
        uint8_t subclass =
            (pci_config_read_raw(bus, device, func, 0x08) >> 16) & 0xFF;

        serial_printf("PCI Device found: bus=%u, dev=%u, func=%u\n", bus,
                      device, func);
        serial_printf("  Vendor ID: 0x%04X, Device ID: 0x%04X\n", vendor,
                      device_id);
        serial_printf("  Class: 0x%02X, Subclass: 0x%02X\n", class_code,
                      subclass);

        const char *name = pci_lookup_device(vendor, device_id);
        if (name)
          serial_printf("  -> Device: %s\n", name);

        uint32_t bars[6];

        pci_device_desc_t desc = {
            .dev_info =
                {
                    .vendor_id = vendor,
                    .device_id = device_id,
                    .name = name,
                },
            .bus = bus,
            .device = device,
            .function = func,
            .bar = {0},
            .io_base = 0,
            .enabled = false,
        };

        pci_read_bars(&desc, bars);
        memcpy(desc.bar, bars, sizeof(uint32_t) * 6);

        desc.io_base = pci_detect_iobase(&desc);
        desc.irq = pci_detect_irq(&desc);

        pci_handle_device(&desc);
      }
    }
  }
}
