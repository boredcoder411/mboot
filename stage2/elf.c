#include "elf.h"
#include "dev/serial.h"
#include "fat16.h"
#include "mem.h"
#include "scheduler.h"
#include "vfs.h"

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

  return 0;
}

int spawn_elf(const char *path, int argc, char **argv) {
  int file = open_file(path);
  if (file < 0) {
    INFO("ELF", "error: could not find %s", path);
    return -1;
  }

  int size = fat16_get_size(file);
  void *buf = kmalloc(size);
  read_file(file, size, buf);
  close_file(file);

  INFO("ELF", "%s loaded (%d bytes)", path, size);
  if (load_elf(buf) != 0) {
    INFO("ELF", "error: %s is not a valid ELF", path);
    return -1;
  }

  Elf32_Ehdr *eh = (Elf32_Ehdr *)buf;
  return create_user_task(eh->entry, argc, argv);
}

void jump_to_entry(void *elf_data) {
  Elf32_Ehdr *eh = (Elf32_Ehdr *)elf_data;
  entry_point_t entry = (entry_point_t)(eh->entry);
  INFO("ELF", "Jumping to entry point at 0x%x", eh->entry);
  entry();
}

extern void run_elf_with_args_asm(void *entry, int argc, char **argv);

void run_elf_with_args(void *elf_data, int argc, char **argv) {
  Elf32_Ehdr *eh = (Elf32_Ehdr *)elf_data;
  INFO("ELF", "Running entry point at 0x%x with argc=%d", eh->entry, argc);
  run_elf_with_args_asm((void *)(uintptr_t)eh->entry, argc, argv);
}
