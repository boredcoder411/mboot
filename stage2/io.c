// Low-level x86 port I/O primitives used by device drivers.
#include "io.h"

/* x86 port I/O is separate from ordinary memory loads and stores. */
void outb(uint16_t port, uint8_t value) {
  asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

uint8_t inb(uint16_t port) {
  uint8_t ret;
  asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

void outl(uint16_t port, uint32_t val) {
  __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

uint32_t inl(uint16_t port) {
  uint32_t ret;
  __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

void insw(uint16_t port, void *addr, uint32_t count) {
  /* REP INSW reads count 16-bit words from DX into the buffer at EDI. The
     memory clobber tells the compiler that the destination is modified. */
  asm volatile("rep insw" : "+D"(addr), "+c"(count) : "d"(port) : "memory");
}
