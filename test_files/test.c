#include <stdint.h>

void outb(uint16_t port, uint8_t value) {
  asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

void _start() {
  outb(0x3f8, 'a');
  while (1) {
  }
}
