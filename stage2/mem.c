#include "mem.h"
#include "utils.h"
#include "dev/serial.h"

void check_overlaps(uint16_t count, e820_entry_t* entries) {
  int overlap_count = 0;
  for (uint16_t i = 0; i < count; i++) {
    e820_entry_t* a = &entries[i];
    uint64_t a_start = a->base;
    uint64_t a_end = a->base + a->length;

    for (uint16_t j = i + 1; j < count; j++) {
      e820_entry_t* b = &entries[j];
      uint64_t b_start = b->base;
      uint64_t b_end = b->base + b->length;

      if ((a_start < b_end) && (b_start < a_end)) {
        overlap_count++;
        serial_print("Overlap detected between entries ");
        serial_print(int_to_str(i));
        serial_print(" and ");
        serial_print(int_to_str(j));
        serial_print("\n");
      }
    }
  }

  if (overlap_count == 0) {
    serial_print("No overlaps detected in memory map.\n");
  } else {
    serial_print(int_to_str(overlap_count));
    serial_print(" overlaps detected in memory map.\n");
  }
}

void dump_mmap(uint16_t count, e820_entry_t* entries) {
  for (uint16_t i = 0; i < count; i++) {
    e820_entry_t* entry = &entries[i];
    serial_print("Base: ");
    serial_print(hextoa((int)(entry->base >> 32)));
    serial_print(hextoa((int)(entry->base & 0xFFFFFFFF)));
    serial_print(", Length: ");
    serial_print(hextoa((int)(entry->length >> 32)));
    serial_print(hextoa((int)(entry->length & 0xFFFFFFFF)));
    serial_print(", Type: ");
    serial_print(int_to_str(entry->type));
    serial_print("\n");
  }
}

uint64_t calculate_total_size(uint16_t count, e820_entry_t* entries) {
    uint64_t total = 0;

    for (uint16_t i = 0; i < count; i++) {
        if (entries[i].type == 1) {
            total += entries[i].length;
        }
    }

    return total;
}