#pragma once

#include <stdint.h>

#define NE2K_CMD 0x00
#define NE2K_RSAR0 0x08
#define NE2K_RSAR1 0x09
#define NE2K_RBCR0 0x0A
#define NE2K_RBCR1 0x0B
#define NE2K_RDMAPORT 0x10
#define NE2K_IMR 0x0F
#define NE2K_CR 0x00
#define NE2K_TPSR 0x04
#define NE2K_TBCR0 0x05
#define NE2K_TBCR1 0x06
#define NE2K_ISR 0x07

#define CMD_STP 0x01
#define CMD_STA 0x02
#define CMD_RD0 0x08
#define CMD_TXP 0x04

void ne2k_init(uint8_t bus, uint8_t device, uint8_t func,
               uint16_t vendor, uint16_t device_id);
void ne2k_send(uint16_t io_base, const uint8_t *frame, uint16_t len);
