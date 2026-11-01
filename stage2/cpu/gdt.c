// Install flat kernel segments and a TSS. Current tasks remain in ring 0, so privilege-transition stack switching is not exercised.
#include "cpu/gdt.h"
#include "dev/serial.h"
#include "mem.h"

static gdt_entry_t gdt_entries[4];
static gdt_ptr_t gdt_ptr;
static tss_entry_t tss;

static void gdt_set_entry(int num, uint32_t base, uint32_t limit,
                          uint8_t access, uint8_t gran) {
  /* x86 descriptors split the base and limit across non-contiguous fields.
     The high limit nibble shares a byte with the granularity flags. */
  gdt_entries[num].limit_low = limit & 0xFFFF;
  gdt_entries[num].base_low = base & 0xFFFF;
  gdt_entries[num].base_mid = (base >> 16) & 0xFF;
  gdt_entries[num].access = access;
  gdt_entries[num].granularity = gran | ((limit >> 16) & 0x0F);
  gdt_entries[num].base_high = (base >> 24) & 0xFF;
}

/* Install flat code/data descriptors plus a TSS descriptor, then reload the
   segment registers and task register for the new table. */
void gdt_init() {
  memset(gdt_entries, 0, sizeof(gdt_entries));
  memset(&tss, 0, sizeof(tss));

  gdt_ptr.limit = sizeof(gdt_entries) - 1;
  gdt_ptr.base = (uint32_t)&gdt_entries;

  /* Entry zero is null. Entries 1 and 2 use 4 KiB granularity to span the
     32-bit address space; entry 3 describes the in-memory task-state segment. */
  gdt_set_entry(0, 0, 0, 0, 0);
  gdt_set_entry(1, 0, 0xFFFFF, 0x9A, 0xCF);
  gdt_set_entry(2, 0, 0xFFFFF, 0x92, 0xCF);

  uint32_t tss_base = (uint32_t)&tss;
  uint32_t tss_limit = sizeof(tss_entry_t) - 1;
  gdt_set_entry(3, tss_base, tss_limit, 0x89, 0x00);

  tss.ss0 = GDT_DATA_SEG;
  tss.iomap_base = sizeof(tss_entry_t);

  uint32_t gdt_addr = (uint32_t)&gdt_ptr;
  asm volatile("lgdt (%0)\n"
               "mov %1, %%ax\n"
               "mov %%ax, %%ds\n"
               "mov %%ax, %%es\n"
               "mov %%ax, %%fs\n"
               "mov %%ax, %%gs\n"
               "mov %%ax, %%ss\n"
               "ljmp %2, $1f\n"
               "1:\n"
               :
               : "r"(gdt_addr), "i"(GDT_DATA_SEG), "i"(GDT_CODE_SEG)
               : "eax", "memory");

  /* Reload CS with the far jump above, then make the TSS descriptor active. */
  asm volatile("ltr %0\n" : : "r"((uint16_t)GDT_TSS_SEG) : "memory");

  INFO("GDT", "GDT and TSS initialized (selector 0x%X)", GDT_TSS_SEG);
}

/* The TSS records a potential privilege-transition stack, although current
   applications run at ring 0 and do not make that transition. */
void tss_set_stack(uint32_t esp0, uint16_t ss0) {
  tss.esp0 = esp0;
  tss.ss0 = ss0;
}
