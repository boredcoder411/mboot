#pragma once

#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096

typedef struct __attribute__((packed)) {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi_extended_attributes;
} e820_entry_t;

typedef struct block_header {
  size_t size;
  int free;
  struct block_header *next;
} block_header_t;

void init_alloc(uint16_t count, e820_entry_t *entries);
void *memset(void *buf, uint8_t c, size_t n);
void *memcpy(void *dst, void *src, size_t n);
void *alloc_page();
void *kmalloc(size_t size);
void kfree(void *ptr);