#pragma once

#include <stdint.h>
#include <stdbool.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC
#define PCI_COMMAND 0x04
#define PCI_BAR0 0x10

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
    int irq;
    bool enabled;
    void *driver_data;
} pci_device_desc_t;

uint32_t pci_config_read(uint8_t bus, uint8_t slot, uint8_t func,
                         uint8_t offset);
uint16_t pci_config_read_word(uint8_t bus, uint8_t device, uint8_t func,
                              uint8_t offset);
const char *pci_lookup_device(uint16_t vendor_id, uint16_t device_id);
void pci_enumerate();

void pci_handle_device(uint8_t bus, uint8_t device, uint8_t func,
                       uint16_t vendor, uint16_t device_id);
