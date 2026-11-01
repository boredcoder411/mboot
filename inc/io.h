// Port I/O declarations and the register frame shared with interrupt assembly.
#pragma once

#include <stdint.h>

#define SERIAL_PORT 0x3F8

/* Assembly pushes these registers in reverse order, leaving EDI at the
   lowest address. ESP and SS are only pushed by the CPU on a privilege
   change; ordinary ring-0 interrupts do not have valid values for them. */
typedef struct {
  uint32_t edi, esi, ebp, edx, ecx, ebx, eax;
  uint32_t int_no, err_code;
  uint32_t eip, cs, eflags, esp, ss;
} registers_t;

void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);
void outl(uint16_t port, uint32_t val);
uint32_t inl(uint16_t port);
void insw(uint16_t port, void *addr, uint32_t count);
