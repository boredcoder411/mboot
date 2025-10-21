#pragma once

#include <stdint.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

typedef union {
    struct {
        uint32_t reg_offset;
        uint32_t func;
        uint32_t device;
        uint32_t bus;
        uint32_t reserved;
        uint32_t enable;
    } __attribute__((packed, aligned(4)));
    uint32_t value;
} pci_config_address_t;

typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    const char *name;
} pci_device_info_t;

uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t func,
                         uint8_t offset);
void pci_enumerate();
