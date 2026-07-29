#include "paging.h"
#include "dev/serial.h"
#include "mem.h"

#define PAGE_DIRECTORY_ENTRIES 1024
#define FOUR_MIB 0x400000U

static uint32_t *page_directory;

static inline uint32_t read_cr0(void) {
  uint32_t value;
  asm volatile("mov %%cr0, %0" : "=r"(value));
  return value;
}

static inline uint32_t read_cr3(void) {
  uint32_t value;
  asm volatile("mov %%cr3, %0" : "=r"(value));
  return value;
}

static inline uint32_t read_cr4(void) {
  uint32_t value;
  asm volatile("mov %%cr4, %0" : "=r"(value));
  return value;
}

static inline void write_cr0(uint32_t value) {
  asm volatile("mov %0, %%cr0" : : "r"(value) : "memory");
}

static inline void write_cr3(uint32_t value) {
  asm volatile("mov %0, %%cr3" : : "r"(value) : "memory");
}

static inline void write_cr4(uint32_t value) {
  asm volatile("mov %0, %%cr4" : : "r"(value) : "memory");
}

void paging_init(void) {
  uintptr_t raw_page_directory = (uintptr_t)kmalloc(PAGE_SIZE * 2);
  page_directory =
      (uint32_t *)align_up_uintptr(raw_page_directory, PAGE_SIZE);

  for (uint32_t i = 0; i < PAGE_DIRECTORY_ENTRIES; ++i) {
    uint32_t phys = i * FOUR_MIB;
    page_directory[i] =
        (phys & 0xFFC00000U) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_SIZE_4MB;
  }

  write_cr4(read_cr4() | 0x00000010U);
  write_cr3((uint32_t)page_directory);
  write_cr0(read_cr0() | 0x80000000U);

  INFO("PAGING", "Enabled 4 MiB identity paging (CR3=0x%X)", read_cr3());
}

bool paging_is_enabled(void) { return (read_cr0() & 0x80000000U) != 0; }

uint32_t paging_get_directory(void) { return read_cr3(); }
