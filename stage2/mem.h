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

typedef struct {
  uint32_t start;
  uint32_t end;
} __attribute__((packed)) mem_block_t;

void init_alloc(uint16_t count, e820_entry_t *entries);
void *memset(void *buf, char c, size_t n);
uint32_t alloc_pages(uint32_t n);