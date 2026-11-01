// Network interface shared by hardware drivers and the protocol stack.
#pragma once

#include "dev/pci.h"

typedef struct {
  pci_device_desc_t desc;
  uint8_t mac[8];
} nic_descriptor;
