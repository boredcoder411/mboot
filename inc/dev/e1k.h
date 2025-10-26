#pragma once

#include <stdint.h>

#define E1K_REG_CTRL 0x0000
#define E1K_REG_STATUS 0x0008
#define E1K_CTRL_RST (1 << 26)
#define E1K_REG_EERD 0x0014
#define E1K_EERD_START (1 << 0)
#define E1K_EERD_DONE (1 << 4)
#define E1K_REG_EECD 0x0010
#define E1K_EECD_EE_PRES (1 << 8)

void e1k_init(uint8_t bus, uint8_t device, uint8_t func, uint16_t vendor,
              uint16_t device_id);
void e1k_send(uint16_t io_base, const uint8_t *frame, uint16_t len);
