#pragma once

#include <stdbool.h>
#include <stdint.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC
#define PCI_COMMAND 0x04
#define PCI_BAR0 0x10
#define PCI_BAR1 0x14
#define PCI_BAR2 0x18
#define PCI_BAR3 0x1c
#define PCI_BAR4 0x20
#define PCI_BAR5 0x24
#define PCI_INTERRUPT_LINE 0x3C

typedef struct {
  uint16_t vendor_id;
  uint16_t device_id;
  const char *name;
} pci_device_info_t;

typedef struct {
  pci_device_info_t dev_info;
  uint8_t bus;
  uint8_t device;
  uint8_t function;
  uint32_t bar[6];
  uint32_t io_base;
  uint8_t irq;
  bool enabled;
} pci_device_desc_t;

uint32_t pci_config_read(const pci_device_desc_t *dev, uint8_t offset);
uint16_t pci_config_read_word(const pci_device_desc_t *dev, uint8_t offset);
void pci_config_write_word(const pci_device_desc_t *dev, uint8_t offset,
                           uint16_t value);

const char *pci_lookup_device(uint16_t vendor_id, uint16_t device_id);
void pci_enumerate();
void pci_handle_device(pci_device_desc_t *desc);
