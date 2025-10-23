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
#include "mbr.h"
#include "mem.h"
#include "utils.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef CUBE_DEMO
#include "cube.c"
#endif

#ifdef TIME_DEMO
#include "psf.h"
#include "time.c"
#endif

#ifdef NE2K_DEMO
#include "dev/pci.h"
void ne2k_send_arp_request(void);
#endif

#ifdef PSF_DEMO
#include "psf.h"
#endif

#ifdef E1K_DEMO
#include "dev/pci.h"
#endif

extern void enable_fpu(void);

#define E820_TABLE_ADDR ((e820_entry_t *)0x9000)
#define E820_ENTRY_COUNT_ADDR ((uint16_t *)0x8E00)
#define MBR_ADDR ((mbr_t *)0x7C00)

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

#if defined(CUBE_DEMO) || defined(TIME_DEMO)
  install_irq(0, pit_handler);
  pic_clear_mask(0);
#endif

  e820_entry_t *mem_map = E820_TABLE_ADDR;
  uint16_t entry_count = *E820_ENTRY_COUNT_ADDR;
  init_alloc(entry_count, mem_map);

  mbr_t *mbr = MBR_ADDR;
  int boot_index;
  for (int i = 0; i < 4; ++i) {
    if (mbr->partitions[i].type == 0xEF) {
      boot_index = i;
    }
  }
  if (boot_index < 0) {
    ERROR("MAIN", "couldn't find EFI partition (type 0xEF)");
    HALT();
  }

  wad_header_t *wad = kmalloc(4096);
  ata_lba_read(mbr->partitions[boot_index].first_lba, 4, wad, 0);

#if defined(PSF_DEMO) || defined(TIME_DEMO)
  psf_header_t *psf = find_file("font.psf", wad);
  if (!psf || psf->magic != PSF1_FONT_MAGIC) {
    ERROR("MAIN", "invalid or missing PSF file");
    HALT();
  }
  uint8_t *glyphs = (uint8_t *)(psf + 1);
  glyph_init(glyphs);
#endif

#ifdef PSF_DEMO
  display_string("Hello, World!", VGA_WHITE);
#endif

#ifdef IMF_DEMO
  imf_t *imf_file = find_file("icon.imf", wad);
  if (imf_file) {
    display_imf(imf_file, 0, 0);
  } else {
    ERROR("MAIN", "missing icon.imf");
  }
#endif

#ifdef NE2K_DEMO
  pci_enumerate();
  ne2k_send_arp_request();
#endif

#ifdef E1K_DEMO
  pci_enumerate();
#endif

  STI()

  while (1) {
  }
}
