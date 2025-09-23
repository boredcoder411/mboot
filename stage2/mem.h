#pragma once

#include <stdint.h>

typedef struct __attribute__((packed)) {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi_extended_attributes;
} e820_entry_t;

void check_overlaps(uint16_t count, e820_entry_t* entries);
void dump_mmap(uint16_t count, e820_entry_t* entries);
uint64_t calculate_total_size(uint16_t count, e820_entry_t* entries);