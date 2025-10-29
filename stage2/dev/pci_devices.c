#include "dev/e1k.h"
#include "dev/nic.h"
#include "dev/pci.h"

void pci_handle_device(pci_device_desc_t *desc) {
  if (desc->dev_info.vendor_id == 0x8086 &&
      desc->dev_info.device_id == 0x100E) {
    desc->enabled = true;
    nic_descriptor nic = {
        .desc = *desc,
    };

    e1k_init(nic);
  }
}
