#include "mem.h"
#include "dev/serial.h"
#include "utils.h"
#include <stddef.h>
#include <stdint.h>

uintptr_t free_ram;
uintptr_t free_ram_end;

void check_overlaps(uint16_t count, e820_entry_t *entries) {
  int overlap_count = 0;
  for (uint16_t i = 0; i < count; i++) {
    e820_entry_t *a = &entries[i];
    uint64_t a_start = a->base;
    uint64_t a_end = a->base + a->length;

    for (uint16_t j = i + 1; j < count; j++) {
      e820_entry_t *b = &entries[j];
      uint64_t b_start = b->base;
      uint64_t b_end = b->base + b->length;

      if ((a_start < b_end) && (b_start < a_end)) {
        overlap_count++;
        serial_printf("Overlap detected between entries %i and %i\n", i, j);
      }
    }
  }

  if (overlap_count == 0) {
    serial_printf("No overlaps detected in memory map.\n");
  } else {
    serial_printf("%i overlaps detected in memory map.\n", overlap_count);
  }
}

void dump_mmap(uint16_t count, e820_entry_t *entries) {
  for (uint16_t i = 0; i < count; i++) {
    e820_entry_t *entry = &entries[i];

    serial_printf("Base: 0x%08X%08X, Length: 0x%08X%08X, Type: %u\n",
                  (uint32_t)(entry->base >> 32),
                  (uint32_t)(entry->base & 0xFFFFFFFF),
                  (uint32_t)(entry->length >> 32),
                  (uint32_t)(entry->length & 0xFFFFFFFF), entry->type);
  }
}

uint64_t calculate_total_size(uint16_t count, e820_entry_t *entries) {
  uint64_t total = 0;

  for (uint16_t i = 0; i < count; i++) {
    if (entries[i].type == 1) {
      total += entries[i].length;
    }
  }

  return total;
}

void init_alloc(uint16_t count, e820_entry_t *entries) {
  dump_mmap(count, entries);
  check_overlaps(count, entries);
  uint64_t size = calculate_total_size(count, entries);
  serial_printf("%f GB detected...\n", bytes_to_gb(size));

  uint16_t biggest_index = 0;

  for (uint16_t i = 0; i < count; i++) {
    if ((entries[i].type == 1) &&
        (entries[i].length > entries[biggest_index].length)) {
      biggest_index = i;
    }
  }

  free_ram = entries[biggest_index].base;
  free_ram_end = entries[biggest_index].base + entries[biggest_index].length;
}

void *memset(void *buf, uint8_t c, size_t n) {
  uint8_t *p = (uint8_t *)buf;
  while (n--)
    *p++ = c;
  return buf;
}

void *memcpy(void *dst, void *src, size_t n) {
  uint8_t *d = (uint8_t *)dst;
  const uint8_t *s = (const uint8_t *)src;
  for (size_t i = 0; i < n; i++) {
    d[i] = s[i];
  }
  return dst;
}

void *kmalloc(uint32_t bytes) {
  static uint32_t next_paddr = 0;

  if (next_paddr == 0) {
    // initialize only once
    next_paddr = free_ram;
  }

  uint32_t paddr = next_paddr;
  next_paddr += bytes;

  if (next_paddr > free_ram_end) {
    serial_printf("Out of memory :(\n");
    HALT()
  }

  memset((void *)paddr, 0, bytes);
  return (void *)paddr;
}
