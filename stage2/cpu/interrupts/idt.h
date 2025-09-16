#pragma once

#include <stdint.h>

struct IDTEntry {
  uint16_t offset_low;
  uint16_t selector;
  uint8_t __ignored;
  uint8_t type;
  uint16_t offset_high;
} __attribute__((packed));

struct IDTPointer {
  uint16_t limit;
  uintptr_t base;
} __attribute__((packed));

static struct {
  struct IDTEntry entries[256];
  struct IDTPointer pointer;
} idt;

extern void idt_load();
extern void isr_common();

void idt_set(uint8_t index, uint16_t off_lo, uint16_t off_hi, uint16_t selector, uint8_t flags);
void idt_init();
