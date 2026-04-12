#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PAGE_SIZE 4096

typedef struct {
  uint64_t base;
  uint64_t length;
  uint32_t type;
  uint32_t acpi_extended_attributes;
} __attribute__((packed)) e820_entry_t;

typedef struct block_header {
  size_t size;
  bool free;
  struct block_header *next;
} block_header_t;

void init_alloc(uint16_t count, e820_entry_t *entries);
void *memset(void *buf, uint8_t c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
void *kmalloc(uint32_t bytes);
void kfree(void *loc);
uintptr_t align_up_uintptr(uintptr_t x, uintptr_t align);
