#include "dev/ne2k.h"
#include "dev/pci.h"

void pci_handle_device(uint8_t bus, uint8_t device, uint8_t func,
                       uint16_t vendor, uint16_t device_id) {
  if (vendor == 0x10EC && device_id == 0x8029)
    ne2k_init(bus, device, func, vendor, device_id);
}
