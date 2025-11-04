#include "cpu/interrupts/idt.h"
#include "io.h"
#include "dev/serial.h"

extern void syscall_handler();

void syscall_dispatch(registers_t *r) {
  switch (r->eax) {
    default:
      INFO("SYSCALL", "called");
  }
}

idt_entry_t idt[IDT_ENTRIES];
idt_ptr_t idt_ptr;

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
  idt[num].base_low = base & 0xFFFF;
  idt[num].sel = sel;
  idt[num].always0 = 0;
  idt[num].flags = flags;
  idt[num].base_high = (base >> 16) & 0xFFFF;
}

void idt_init() {
  idt_ptr.limit = sizeof(idt_entry_t) * IDT_ENTRIES - 1;
  idt_ptr.base = (uint32_t)&idt;

  for (int i = 0; i < IDT_ENTRIES; i++) {
    idt_set_gate(i, 0, 0, 0);
  }

  idt_set_gate(0x80, (uint32_t)syscall_handler, 0x08, 0x8E);

  idt_load((uint32_t)&idt_ptr);
}
