#pragma once

#include <stdbool.h>
#include <stdint.h>

#define PAGE_PRESENT 0x001
#define PAGE_WRITABLE 0x002
#define PAGE_USER 0x004
#define PAGE_SIZE_4MB 0x080

void paging_init(void);
bool paging_is_enabled(void);
uint32_t paging_get_directory(void);
uint32_t paging_get_kernel_directory(void);
uint32_t paging_create_directory(void);
void paging_map_page(uint32_t pd, uint32_t vaddr, uint32_t paddr,
                     uint32_t flags);
void paging_switch(uint32_t pd);
