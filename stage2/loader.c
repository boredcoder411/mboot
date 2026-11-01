// Kernel initialization order from interrupt setup through scheduling /init.elf. Boot-time E820 data is supplied by stage1.
#include "cpu/cpuid.h"
#include "cpu/gdt.h"
#include "cpu/interrupts/idt.h"
#include "cpu/interrupts/irq.h"
#include "cpu/interrupts/isr.h"
#include "cpu/pic/pic.h"
#include "cpu/pit/pit.h"
#include "dev/keyboard.h"
#include "dev/pci.h"
#include "dev/serial.h"
#include "dev/tty.h"
#include "elf.h"
#include "fat16.h"
#include "mem.h"
#include "net/net.h"
#include "paging.h"
#include "scheduler.h"
#include "utils.h"
#include <stdbool.h>
#include <stdint.h>

extern void enable_fpu(void);

#define E820_TABLE_ADDR ((e820_entry_t *)0x71000)
#define E820_ENTRY_COUNT_ADDR ((uint16_t *)0x70FF0)

#ifdef ALLOC_DBG
int malloc_calls;
int free_calls;
#endif

void loader_start(void) {
  /* Keep device IRQs masked while installing handlers and initializing the
     subsystems they use; the timer is unmasked after scheduler setup. */
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
  cpuid_init();

  /* Stage1 leaves the count and table at these fixed physical addresses. */
  e820_entry_t *mem_map = E820_TABLE_ADDR;
  uint16_t entry_count = *E820_ENTRY_COUNT_ADDR;
  init_alloc(entry_count, mem_map);
  /* Heap initialization must precede TTY and page-table allocations. The
     initial page directory identity maps addresses used by the kernel. */
  tty_init();
  paging_init();

  pci_enumerate();
  net_init();
  gdt_init();
  scheduler_init();
  pic_clear_mask(0);
  install_irq(0, NULL);
  uint32_t esp;
  asm("mov %%esp, %0" : "=r"(esp));
  tss_set_stack(esp, GDT_DATA_SEG);

  /* Once devices and scheduling are ready, register FAT16 with the VFS so
     spawn_elf can read the first application from the disk image. */
  fat16_init();

  char *argv[] = {"init", NULL};
  int argc = 1;

  INFO("MAIN", "Starting init...");
  if (spawn_elf("/init.elf", argc, argv, -1) < 0) {
    while (1) {
      asm volatile("hlt");
    }
  }

  /* Enable hardware interrupts only after init has been placed on the task
     list. The idle context then halts until an interrupt makes work possible. */
  STI();
  INFO("MAIN", "Init scheduled. Entering idle loop.");
  while (1) {
    asm volatile("hlt");
  }
}
