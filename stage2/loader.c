#include "cpu/interrupts/idt.h"
#include "cpu/interrupts/irq.h"
#include "cpu/interrupts/isr.h"
#include "cpu/pic/pic.h"
#include "cpu/pit/pit.h"
#include "dev/disk.h"
#include "dev/keyboard.h"
#include "dev/rtc.h"
#include "dev/serial.h"
#include "dev/vga.h"
#include "fs.h"
#include "mbr.h"
#include "mem.h"
#include "psf.h"
#include "utils.h"
#include <stdbool.h>
#include <stdint.h>

extern void enable_fpu();

#pragma GCC diagnostic ignored "-Wunused-parameter"
void pit_handler(registers_t *r) {
  clear_screen();
  char *time = fetch_rtc();
  display_string(time, VGA_WHITE);
}

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
  install_irq(0, pit_handler);
  pic_clear_mask(0);

  e820_entry_t *mem_map = (e820_entry_t *)0x9000;
  uint16_t entry_count = (*(uint16_t *)0x8E00);

  init_alloc(entry_count, mem_map);

  mbr_t *mbr = (mbr_t *)0x7C00;
  uint32_t found = 0;
  for (uint32_t i = 0; i < 4; i++) {
    if (mbr->partitions[i].type == 0xef) {
      found = i;
      break;
    }
  }

  if (found == 4) {
    serial_print("couldn't find second partition");
    HALT()
  }

  wad_header_t *wad = kmalloc(4096);
  ata_lba_read(mbr->partitions[found].first_lba, 4, wad, 0);

  psf_header_t *psf = find_file("font.psf", wad);
  if (psf->magic != PSF1_FONT_MAGIC) {
    serial_print("invalid psf file\n");
    HALT()
  }
  uint8_t *glyphs = (uint8_t *)(psf + 1);
  vga_init(glyphs);

  uint8_t *welcome = find_file("test.txt", wad);
  display_string((char *)welcome, VGA_WHITE);

  imf_t *imf_file = find_file("icon.imf", wad);
  display_imf(imf_file, 0, 16);

  STI()

  while (1) {
  }
}
