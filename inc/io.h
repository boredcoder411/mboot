#pragma once

#include <stdint.h>

#define SERIAL_PORT 0x3F8

typedef struct {
  uint32_t edi, esi, ebp, edx, ecx, ebx, eax;
  uint32_t int_no, err_code;
  uint32_t eip, cs, eflags, esp, ss;
} registers_t;

void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);
void insw(uint16_t port, void *addr, uint32_t count);
