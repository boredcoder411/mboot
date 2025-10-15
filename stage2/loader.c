#include "cpu/interrupts/idt.h"
#include "cpu/interrupts/irq.h"
#include "cpu/interrupts/isr.h"
#include "cpu/pic/pic.h"
#include "cpu/pit/pit.h"
#include "dev/disk.h"
#include "dev/keyboard.h"
#include "dev/serial.h"
#include "dev/vga.h"
#include "fs.h"
#include "imf.h"
#include "mbr.h"
#include "mem.h"
#include "psf.h"
#include "utils.h"
#include <stdbool.h>
#include <stdint.h>

extern void enable_fpu();

int x = 0;
int y = 0;

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
  remap_vga_dac();

  memset(VIDEO_MEMORY, 0, 320 * 200);

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
    asm("cli;hlt");
  }

  wad_header_t *wad = alloc_page();
  ata_lba_read(mbr->partitions[found].first_lba, 4, wad, 0);

  psf_header_t *psf = find_file("font.psf", wad);
  if (psf->magic != PSF1_FONT_MAGIC) {
    serial_print("invalid psf file\n");
    asm("cli;hlt");
  }
  uint8_t *glyphs = (uint8_t *)(psf + 1);

  uint8_t *welcome = find_file("test.txt", wad);
  for (uint32_t i = 0; welcome[i + 1] != '\0'; i++) {
    display_glyph(glyphs, welcome[i], x, y, VGA_WHITE);
    x += 8;
  }

  x = 0;
  y = 8;
  imf_t *imf_file = find_file("icon.imf", wad);
  display_imf(imf_file, x, y);

  asm("sti");

  while (1) {
  }
}
