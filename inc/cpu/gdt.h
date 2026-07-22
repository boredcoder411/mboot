#pragma once

#include <stdint.h>

#define GDT_CODE_SEG 0x08
#define GDT_DATA_SEG 0x10
#define GDT_TSS_SEG 0x18

typedef struct {
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t base_mid;
  uint8_t access;
  uint8_t granularity;
  uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed)) gdt_ptr_t;

typedef struct {
  uint16_t prev, res0;
  uint32_t esp0;
  uint16_t ss0, res1;
  uint32_t esp1;
  uint16_t ss1, res2;
  uint32_t esp2;
  uint16_t ss2, res3;
  uint32_t cr3, eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
  uint32_t es, cs, ss, ds, fs, gs, ldt;
  uint16_t trap, iomap_base;
} __attribute__((packed)) tss_entry_t;

void gdt_init();
void tss_set_stack(uint32_t esp0, uint16_t ss0);