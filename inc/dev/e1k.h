#pragma once

#include <stdint.h>

void e1k_init(uint8_t bus, uint8_t device, uint8_t func, uint16_t vendor,
              uint16_t device_id);
void e1k_send(uint16_t io_base, const uint8_t *frame, uint16_t len);
