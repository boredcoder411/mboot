#include "elf.h"
#include "dev/serial.h"
#include "mem.h"

int load_elf(void *file_data) {
  INFO("ELF", "Loading elf file");
  Elf32_Ehdr *eh = (Elf32_Ehdr *)file_data;

  if (eh->magic != ELF_MAGIC)
    return -1;

  Elf32_Phdr *ph = (Elf32_Phdr *)((uint8_t *)file_data + eh->phoff);
  for (int i = 0; i < eh->phnum; i++, ph++) {
    if (ph->type != PT_LOAD)
      continue;

    void *src = (uint8_t *)file_data + ph->offset;
    void *dst = (void *)ph->vaddr;

    memcpy(dst, src, ph->filesz);

    if (ph->memsz > ph->filesz)
      memset((uint8_t *)dst + ph->filesz, 0, ph->memsz - ph->filesz);
  }

  INFO("ELF", "Running elf file");
  entry_point_t entry = (entry_point_t)(eh->entry);
  entry();

  return 0;
}
