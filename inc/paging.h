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
