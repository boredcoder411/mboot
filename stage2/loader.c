#include "cpu/gdt.h"
#include "cpu/interrupts/idt.h"
#include "cpu/interrupts/irq.h"
#include "cpu/interrupts/isr.h"
#include "cpu/pic/pic.h"
#include "cpu/pit/pit.h"
#include "dev/keyboard.h"
#include "dev/pci.h"
#include "dev/serial.h"
#include "elf.h"
#include "fat16.h"
#include "mem.h"
#include "paging.h"
#include "scheduler.h"
#include "utils.h"
#include <stdbool.h>
#include <stdint.h>

extern void enable_fpu(void);

#define E820_TABLE_ADDR ((e820_entry_t *)0x9000)
#define E820_ENTRY_COUNT_ADDR ((uint16_t *)0x8E00)

#ifdef ALLOC_DBG
int malloc_calls;
int free_calls;
#endif

void loader_start(void) {
  for (int i = 0; i < IRQs; ++i) {
    pic_set_mask(i);
  }

  pic_remap();
  idt_init();
  install_exception_isrs();
  pit_init();
  enable_fpu();
  install_keyboard();
  init_serial();

  e820_entry_t *mem_map = E820_TABLE_ADDR;
  uint16_t entry_count = *E820_ENTRY_COUNT_ADDR;
  init_alloc(entry_count, mem_map);
  paging_init();

  pci_enumerate();
  gdt_init();
  scheduler_init();
  pic_clear_mask(0);
  install_irq(0, NULL);
  uint32_t esp;
  asm("mov %%esp, %0" : "=r"(esp));
  tss_set_stack(esp, GDT_DATA_SEG);

  fat16_init();

  char *argv[] = {"init", NULL};
  int argc = 1;

  INFO("MAIN", "Starting init...");
  if (spawn_elf("/init.elf", argc, argv) < 0) {
    while (1) {
      asm volatile("hlt");
    }
  }

  STI();
  INFO("MAIN", "Init scheduled. Entering idle loop.");
  while (1) {
    asm volatile("hlt");
  }
}
