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

void dump_mmap(uint16_t count, e820_entry_t *entries) {
  for (uint16_t i = 0; i < count; i++) {
    e820_entry_t *entry = &entries[i];
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
  print_float(bytes_to_gb(size));
  serial_print(" GB detected...\n");

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

void* alloc_page() {
    static uint32_t next_paddr = 0;

    if (next_paddr == 0) {
        // initialize only once
        next_paddr = free_ram;
    }

    uint32_t paddr = next_paddr;
    next_paddr += PAGE_SIZE;

    if (next_paddr > free_ram_end) {
        serial_print("Out of memory :(\n");
        asm("cli");
        asm("hlt");
    }

    memset((void *)paddr, 0, PAGE_SIZE);
    return (void *)paddr;
}

static block_header_t *free_list = NULL;

#define ALIGN4(x) (((((x) - 1) >> 2) << 2) + 4)
#define BLOCK_SIZE sizeof(block_header_t)

void *kmalloc(size_t size) {
  size = ALIGN4(size);
  block_header_t *curr = free_list;
  block_header_t *prev = NULL;

  // First-fit search
  while (curr) {
    if (curr->free && curr->size >= size) {
      curr->free = 0;
      return (void *)(curr + 1);
    }
    prev = curr;
    curr = curr->next;
  }

  // No suitable block found → expand heap
  uintptr_t addr = free_ram;
  uintptr_t new_free_ram = free_ram + BLOCK_SIZE + size;

  if (new_free_ram > free_ram_end) {
    serial_print("kmalloc: Out of memory!\n");
    asm("cli; hlt");
  }

  block_header_t *block = (block_header_t *)addr;
  block->size = size;
  block->free = 0;
  block->next = NULL;

  free_ram = new_free_ram;

  if (prev) {
    prev->next = block;
  } else {
    free_list = block;
  }

  return (void *)(block + 1);
}

void kfree(void *ptr) {
  if (!ptr)
    return;

  block_header_t *block = (block_header_t *)ptr - 1;
  block->free = 1;

  // Coalescing (optional simple version)
  block_header_t *curr = free_list;
  while (curr) {
    if (curr->free) {
      while (curr->next && curr->next->free) {
        curr->size += BLOCK_SIZE + curr->next->size;
        curr->next = curr->next->next;
      }
    }
    curr = curr->next;
  }
}
