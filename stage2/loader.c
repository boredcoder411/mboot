#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "mbr.h"
#include "fs.h"
#include "utils.h"
#include "vga.h"
#include "serial.h"
#include "disk.h"
#include "cpu/interrupts/idt.h"

void loader_start() {
	init_serial();
  idt_init();
  asm("sti");

	uint32_t lba = 0;
	uint8_t sector_count = 1;
	uint8_t buffer[512];
	uint8_t drive = 0;

	//ata_lba_read(lba, sector_count, buffer, drive);

  //mbr_t* mbr = init_mbr(buffer);
  //if (!mbr) {
  //  serial_print("No MBR found!\n");
  //  while (1);
  //}

  //partition_entry_t* partition = mbr->partitions;
  //for (int i = 0; i < 4; i++) {
  //  if (partition[i].status != 0x80) {
  //    break;
  //  }
  //  partition++;
  //}

  //lba = partition->first_lba;
  //ata_lba_read(lba, sector_count, buffer, drive);

  //wad_header_t* wad = init_wad(buffer);
  //if (!wad) {
  //  serial_print("No WAD found!\n");
  //  while (1);
  //}

  //lump_entry_t* lumps = init_lumps(wad);
  //if (!lumps) {
  //  serial_print("No lumps found!\n");
  //  while (1);
  //}

  //for (int x = 0; x < SCREEN_WIDTH; x++) {
  //  for (int y = 0; y < SCREEN_HEIGHT; y++) {
  //    put_pixel(x, y, y % 256);
  //  }
  //}

	//return;
}
