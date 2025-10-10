#include "cpu/interrupts/idt.h"
#include "cpu/interrupts/isr.h"
#include "cpu/pic/pic.h"
#include "cpu/pit/pit.h"
#include "dev/keyboard.h"
#include "dev/vga.h"
#include "dev/disk.h"
#include "dev/serial.h"
#include "mem.h"
#include "mbr.h"
#include "fs.h"
#include "utils.h"
#include <stdbool.h>
#include <stdint.h>

extern void enable_fpu();

void loader_start() {
  for (int i = 0; i < IRQs; i++) {
    pic_set_mask(i);
  }

  pic_remap();
  idt_init();
  install_exception_isrs();
  pit_init();
  enable_fpu();
  install_keyboard();

  memset(VIDEO_MEMORY, 0, 320 * 200);

  e820_entry_t *mem_map = (e820_entry_t *)0x9000;
  uint16_t entry_count = (*(uint16_t *)0x8E00);

  init_alloc(entry_count, mem_map);

  uint8_t *buf = alloc_page();
  mbr_t *mbr = init_mbr(buf);
  if (!mbr) {
    serial_print("Failed to find valid mbr\n");
    asm("cli;hlt");
  }

  for (int i = 0; i < 4; i++) {
    serial_print("Partition: ");
    serial_print(int_to_str(i));
  }

  asm("sti");

  while (1) {
  }
}
