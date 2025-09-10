#pragma once

#include <stdint.h>

#define SERIAL_PORT 0x3F8

void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);
void insw(uint16_t port, void *addr, uint32_t count);