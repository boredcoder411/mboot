#pragma once

#include <stdint.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

typedef struct {
  uint16_t vendor_id;
  uint16_t device_id;
  const char *name;
} pci_device_info_t;

uint32_t pci_config_read(uint8_t bus, uint8_t slot, uint8_t func,
                         uint8_t offset);
uint16_t pci_config_read_word(uint8_t bus, uint8_t device, uint8_t func,
                              uint8_t offset);
const char *pci_lookup_device(uint16_t vendor_id, uint16_t device_id);
void pci_enumerate();

void pci_handle_device(uint8_t bus, uint8_t device, uint8_t func,
                       uint16_t vendor, uint16_t device_id);
