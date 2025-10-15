#include "cpu/interrupts/idt.h"
#include "cpu/interrupts/isr.h"
#include "cpu/interrupts/irq.h"
#include "cpu/pic/pic.h"
#include "cpu/pit/pit.h"
#include "dev/disk.h"
#include "dev/keyboard.h"
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

void display_glyph(uint8_t *glyphs, uint32_t glyph_index, int x, int y, uint8_t color) {
    for (uint32_t row = 0; row < 8; row++) {
        uint8_t row_byte = glyphs[glyph_index * 8 + row];
        for (int bit = 7; bit >= 0; bit--) {
            if (row_byte & (1 << bit)) {
                put_pixel(x + (7 - bit), y + row, color);
            }
        }
    }
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
  ata_lba_read(mbr->partitions[found].first_lba, 3, wad, 0);

  lump_entry_t *entries = (lump_entry_t *)((uint8_t *)wad + wad->dir_offset);
  for (uint32_t i = 0; i < wad->num_lumps; i++) {
    if (strncmp(entries[i].name, "font.psf", 8)) {
      found = i;
      break;
    }
  }

  if (found == wad->num_lumps) {
    serial_print("couldn't find font\n");
    asm("cli;hlt");
  }

  psf_header_t *psf = (psf_header_t *)((uint8_t *)wad + entries[found].offset);
  if (psf->magic != PSF1_FONT_MAGIC) {
    serial_print("invalid psf file\n");
    asm("cli;hlt");
  }

  uint8_t *glyphs = (uint8_t *)(psf + 1);
  int x = 0;
  for (int i = 65; i < (65 + 26); i++) {
    display_glyph(glyphs, i, x, 0, 15);
    x += 8;
  }

  asm("sti");

  while (1) {
  }
}
