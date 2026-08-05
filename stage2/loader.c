#include "cpu/gdt.h"
#include "cpu/interrupts/idt.h"
#include "cpu/interrupts/irq.h"
#include "cpu/interrupts/isr.h"
#include "cpu/pic/pic.h"
#include "cpu/pit/pit.h"
#include "dev/e1k.h"
#include "dev/keyboard.h"
#include "dev/pci.h"
#include "dev/serial.h"
#include "elf.h"
#include "fat16.h"
#include "mem.h"
#include "net/arp.h"
#include "net/icmp.h"
#include "net/ipv4.h"
#include "net/udp.h"
#include "paging.h"
#include "scheduler.h"
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

  e820_entry_t *mem_map = E820_TABLE_ADDR;
  uint16_t entry_count = *E820_ENTRY_COUNT_ADDR;
  init_alloc(entry_count, mem_map);
  paging_init();

  pci_enumerate();
  uint8_t src_ip[4] = {10, 0, 2, 15};
  uint8_t gateway_ip[4] = {10, 0, 2, 2};
  uint8_t ping_ip[4] = {142, 251, 142, 14};
  uint8_t gateway_mac[6];
  ipv4_set_address(src_ip);
  udp_init();
  INFO("MAIN", "Network ready: guest IPv4 %d.%d.%d.%d, UDP echo port %u",
       src_ip[0], src_ip[1], src_ip[2], src_ip[3], 7);
  gdt_init();
  scheduler_init();
  pic_clear_mask(0);
  install_irq(0, NULL);
  uint32_t esp;
  asm("mov %%esp, %0" : "=r"(esp));
  tss_set_stack(esp, GDT_DATA_SEG);
  arp_send_request(src_ip, gateway_ip);

  int resolved = 0;
  for (int i = 0; i < 12000000; ++i) {
    e1k_drain_rx();
    if (arp_try_get_mac(gateway_ip, gateway_mac)) {
      resolved = 1;
      break;
    }
    asm volatile("pause");
  }

  if (resolved) {
    icmp_send_echo(src_ip, ping_ip, gateway_mac, 0xB007, 1);
  } else {
    INFO("MAIN", "failed to resolve gateway ARP, skipping ICMP echo");
  }

  fat16_init();

  char *argv[] = {"lua", NULL};
  int argc = 1;

  INFO("MAIN", "Starting Lua init...");
  if (spawn_elf("/lua.elf", argc, argv) < 0) {
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
