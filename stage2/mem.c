#include "mem.h"
#include "dev/serial.h"
#include "utils.h"
#include <stddef.h>
#include <stdint.h>

uintptr_t free_ram;
uintptr_t free_ram_end;

#ifdef ALLOC_DBG
extern int malloc_calls;
extern int free_calls;
#endif

inline uintptr_t align_up_uintptr(uintptr_t x, uintptr_t align) {
  return (x + (align - 1)) & ~(align - 1);
}

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
        WARN("MEM", "Overlap detected between entries %i and %i", i, j);
      }
    }
  }

  if (overlap_count == 0) {
    INFO("MEM", "No overlaps detected in memory map.");
  } else {
    WARN("MEM", "%i overlaps detected in memory map.", overlap_count);
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

block_header_t *free_list_head = NULL;

uintptr_t bump_next = 0;

void init_alloc(uint16_t count, e820_entry_t *entries) {
  dump_mmap(count, entries);
  check_overlaps(count, entries);
  uint64_t size = calculate_total_size(count, entries);
  INFO("MEM", "%f GB detected...", bytes_to_gb(size));

  uint16_t biggest_index = 0;

  for (uint16_t i = 0; i < count; i++) {
    if ((entries[i].type == 1) &&
        (entries[i].length > entries[biggest_index].length)) {
      biggest_index = i;
    }
  }

  free_ram = entries[biggest_index].base;
  free_ram_end = entries[biggest_index].base + entries[biggest_index].length;

  bump_next = free_ram;
}

void *memset(void *buf, uint8_t c, size_t n) {
  uint8_t *p = (uint8_t *)buf;
  while (n--)
    *p++ = c;
  return buf;
}

void *memcpy(void *dst, const void *src, size_t n) {
  uint8_t *d = (uint8_t *)dst;
  const uint8_t *s = (const uint8_t *)src;
  for (size_t i = 0; i < n; i++) {
    d[i] = s[i];
  }
  return dst;
}

block_header_t *find_free_block_and_remove(uint32_t size) {
  block_header_t *prev = NULL;
  block_header_t *cur = free_list_head;

  while (cur) {
    if (cur->free && cur->size >= size) {
      if (prev) {
        prev->next = cur->next;
      } else {
        free_list_head = cur->next;
      }
      cur->next = NULL;
      cur->free = 0;
      return cur;
    }
    prev = cur;
    cur = cur->next;
  }
  return NULL;
}

void try_split_block(block_header_t *block, uint32_t want_size) {
  uintptr_t hdr_size = (uintptr_t)sizeof(block_header_t);
  if (block->size <= want_size + hdr_size + 16) {
    return;
  }

  uintptr_t block_addr = (uintptr_t)block;
  uintptr_t payload_addr = block_addr + hdr_size;
  uintptr_t new_free_hdr_addr = payload_addr + want_size;

  block_header_t *new_hdr = (block_header_t *)new_free_hdr_addr;
  new_hdr->size = block->size - want_size - hdr_size;
  new_hdr->free = 1;
  new_hdr->next = NULL;

  block->size = want_size;

  block_header_t *prev = NULL;
  block_header_t *cur = free_list_head;
  while (cur && (uintptr_t)cur < (uintptr_t)new_hdr) {
    prev = cur;
    cur = cur->next;
  }
  new_hdr->next = cur;
  if (prev)
    prev->next = new_hdr;
  else
    free_list_head = new_hdr;
}

void *kmalloc(uint32_t bytes) {
#ifdef ALLOC_DBG
  malloc_calls++;
#endif
  const uintptr_t ALIGN = 8;
  if (bytes == 0)
    return NULL;
  uintptr_t want_size = (uintptr_t)bytes;
  want_size = align_up_uintptr(want_size, ALIGN);

  if (bump_next == 0) {
    bump_next = free_ram;
  }

  block_header_t *found = find_free_block_and_remove((uint32_t)want_size);
  if (found) {
    try_split_block(found, (uint32_t)want_size);
    void *payload = (void *)((uintptr_t)found + sizeof(block_header_t));
    memset(payload, 0, want_size);
    return payload;
  }

  uintptr_t hdr_addr =
      align_up_uintptr(bump_next, ALIGN); /* align header for safety */
  uintptr_t payload_addr = hdr_addr + sizeof(block_header_t);
  uintptr_t new_bump = payload_addr + want_size;

  if (new_bump > free_ram_end) {
    ERROR("MEM", "Out of memory :(");
    HALT();
  }

  block_header_t *hdr = (block_header_t *)hdr_addr;
  hdr->size = (uint32_t)want_size;
  hdr->free = 0;
  hdr->next = NULL;

  bump_next = new_bump;

  void *payload = (void *)payload_addr;
  memset(payload, 0, want_size);
  return payload;
}

int try_coalesce_adjacent(block_header_t *a, block_header_t *b) {
  uintptr_t a_end = (uintptr_t)a + sizeof(block_header_t) + a->size;
  if (a_end == (uintptr_t)b) {
    a->size = a->size + sizeof(block_header_t) + b->size;
    a->next = b->next;
    return 1;
  }
  return 0;
}

void kfree(void *loc) {
#ifdef ALLOC_DBG
  free_calls++;
#endif
  if (loc == NULL)
    return;

  block_header_t *block =
      (block_header_t *)((uintptr_t)loc - sizeof(block_header_t));

  uintptr_t block_addr = (uintptr_t)block;
  if ((block_addr < free_ram) || (block_addr >= free_ram_end)) {
    WARN("MEM", "Attempt to free pointer outside managed area: %p", loc);
    return;
  }

  block->free = 1;
  block->next = NULL;

  block_header_t *prev = NULL;
  block_header_t *cur = free_list_head;
  while (cur && (uintptr_t)cur < (uintptr_t)block) {
    prev = cur;
    cur = cur->next;
  }

  block->next = cur;
  if (prev)
    prev->next = block;
  else
    free_list_head = block;

  if (prev) {
    block_header_t *scan_prev = NULL;
    block_header_t *scan = free_list_head;
    while (scan && (uintptr_t)scan < (uintptr_t)block) {
      scan_prev = scan;
      scan = scan->next;
    }
    if (scan_prev && try_coalesce_adjacent(scan_prev, block)) {
      block = scan_prev;
    }
  }

  if (block->next) {
    try_coalesce_adjacent(block, block->next);
  }
}
