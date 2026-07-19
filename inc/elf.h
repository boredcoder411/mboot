#pragma once

#include <stdint.h>

#define PT_LOAD 1
#define ELF_MAGIC 0x464C457F

#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_REL 9

typedef struct {
  uint32_t magic;
  uint8_t class;
  uint8_t data;
  uint8_t version;
  uint8_t os_abi;
  uint8_t abi_version;
  uint8_t pad[7];
  uint16_t type;
  uint16_t machine;
  uint32_t version2;
  uint32_t entry;
  uint32_t phoff;
  uint32_t shoff;
  uint32_t flags;
  uint16_t ehsize;
  uint16_t phentsize;
  uint16_t phnum;
  uint16_t shentsize;
  uint16_t shnum;
  uint16_t shstrndx;
} Elf32_Ehdr;

typedef struct {
  uint32_t type;
  uint32_t offset;
  uint32_t vaddr;
  uint32_t paddr;
  uint32_t filesz;
  uint32_t memsz;
  uint32_t flags;
  uint32_t align;
} Elf32_Phdr;

typedef struct {
  uint32_t sh_name;
  uint32_t sh_type;
  uint32_t sh_flags;
  uint32_t sh_addr;
  uint32_t sh_offset;
  uint32_t sh_size;
  uint32_t sh_link;
  uint32_t sh_info;
  uint32_t sh_addralign;
  uint32_t sh_entsize;
} __attribute__((packed)) Elf32_Shdr;

typedef void (*entry_point_t)(void);

int load_elf(void *file_data);
void jump_to_entry(void *elf_data);
void run_elf_with_args(void *elf_data, int argc, char **argv);
