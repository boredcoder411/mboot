#pragma once

#include <stdint.h>

void ne2k_init(uint8_t bus, uint8_t device, uint8_t func,
               uint16_t vendor, uint16_t device_id);

