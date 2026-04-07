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
#include "elf.h"
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
  init_serial();
  remap_vga_dac();

  e820_entry_t *mem_map = E820_TABLE_ADDR;
  uint16_t entry_count = *E820_ENTRY_COUNT_ADDR;
  init_alloc(entry_count, mem_map);

  pci_enumerate();
  uint8_t src_ip[4] = {10, 0, 2, 15};
  uint8_t gateway_ip[4] = {10, 0, 2, 2};
  uint8_t ping_ip[4] = {142, 251, 142, 14};
  uint8_t gateway_mac[6];
  STI();
  e1k_send_arp_request(src_ip, gateway_ip);

  int resolved = 0;
  for (int i = 0; i < 12000000; ++i) {
    e1k_drain_rx();
    if (e1k_try_get_arp_mac(gateway_ip, gateway_mac)) {
      resolved = 1;
      break;
    }
    asm volatile("pause");
  }

  if (resolved) {
    e1k_send_icmp_echo(src_ip, ping_ip, gateway_mac, 0xB007, 1);
  } else {
    INFO("MAIN", "failed to resolve gateway ARP, skipping ICMP echo");
  }

  fat16_init();

  INFO("MAIN", "Loading test.elf...");
  int file = open_file("/test.elf");
  void *test_buf = NULL;
  int test_size = 0;

  if (file >= 0) {
    test_size = fat16_get_size(file);
    test_buf = kmalloc(test_size);
    read_file(file, test_size, test_buf);
    load_elf(test_buf);
    close_file(file);
    INFO("MAIN", "test.elf loaded successfully");
  } else {
    INFO("MAIN", "error: could not find test.elf");
    while (1) {
    }
  }

  Elf32_Ehdr *test_header = (Elf32_Ehdr *)test_buf;
  entry_point_t test_entry = (entry_point_t)test_header->entry;

  kfree(test_buf);

  INFO("MAIN", "Jumping to test program at 0x%x", (uint32_t)test_entry);
  test_entry();

  INFO("MAIN", "if you see this message, the elf returned somehow.");

#ifdef ALLOC_DBG
  INFO("MAIN", "malloc called %d times, free called %d times", malloc_calls,
       free_calls);
#endif

  while (1) {
  }

#ifdef ALLOC_DBG
  INFO("MAIN", "malloc called %d times, free called %d times", malloc_calls,
       free_calls);
#endif

  while (1) {
  }
}
