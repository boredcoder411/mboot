#pragma once

#include <stdint.h>

#define ELF32_R_SYM(i) ((i) >> 8)
#define ELF32_R_TYPE(i) ((uint8_t)(i))
#define R_386_32 1
#define R_386_PC32 2

typedef struct {
  uint32_t st_name;
  uint32_t st_value;
  uint32_t st_size;
  uint8_t st_info;
  uint8_t st_other;
  uint16_t st_shndx;
} __attribute__((packed)) elf32_sym_t;

typedef struct {
  uint32_t r_offset;
  uint32_t r_info;
} __attribute__((packed)) Elf32_Rel;

void elf_relocate(void *libc_image, void *app_image);
