#include "cpu/interrupts/idt.h"
#include "utils.h"
#include "serial.h"

void idt_set(uint8_t index, uint16_t off_lo, uint16_t off_hi, uint16_t selector, uint8_t flags) {
  idt.entries[index] = (struct IDTEntry) {
    .offset_low = off_lo,
    .offset_high = off_hi,
    .selector = selector,
    .type = flags | 0x60,
    .__ignored = 0
  };
}

void idt_init() {
  idt.pointer.limit = sizeof(idt.entries) - 1;
  idt.pointer.base = (uintptr_t) &idt.entries[0];
  memset(&idt.entries[0], 0, sizeof(idt.entries));

  uintptr_t handler = (uintptr_t) isr_common;
  uint16_t handler_lo = handler & 0xFFFF;
  uint16_t handler_hi = handler >> 16;

  for (int i = 0; i < 256; i++) {
    idt_set(i, handler_lo, handler_hi, 0x08, 0x8E);
  }

  idt_load((uintptr_t) &idt.pointer);
}

void isr_handler() {
  serial_print("interrupt triggered\n");
  return;
}
