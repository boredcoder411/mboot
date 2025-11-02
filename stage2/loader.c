#include "cpu/interrupts/idt.h"
#include "cpu/interrupts/irq.h"
#include "cpu/interrupts/isr.h"
#include "cpu/pic/pic.h"
#include "cpu/pit/pit.h"
#include "dev/e1k.h"
#include "dev/keyboard.h"
#include "dev/pci.h"
#include "dev/serial.h"
#include "dev/vga.h"
#include "fat16.h"
#include "mem.h"
#include "utils.h"
#include "vfs.h"
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
  remap_vga_dac();

  e820_entry_t *mem_map = E820_TABLE_ADDR;
  uint16_t entry_count = *E820_ENTRY_COUNT_ADDR;
  init_alloc(entry_count, mem_map);

  pci_enumerate();
  uint8_t src_ip[4] = {10, 0, 2, 15};
  uint8_t target_ip[4] = {10, 0, 2, 2};
  STI();
  e1k_send_arp_request(src_ip, target_ip);

  fat16_init();

  int file = open_file("/hi.txt");
  int size = fat16_get_size(file);
  void *buf = kmalloc(size);
  read_file(file, size, buf);
  kfree(buf);
  close_file(file);

#ifdef ALLOC_DBG
  INFO("MAIN", "malloc called %d times, free called %d times", malloc_calls,
       free_calls);
#endif

  while (1) {
  }
}
