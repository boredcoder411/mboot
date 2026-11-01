// x86 page-directory setup and per-task mappings. Identity mappings and ring-0 tasks mean these tables are not a security boundary.
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
  /* Page directories must be 4 KiB aligned; keep enough heap space to
     align the returned address without depending on kmalloc's alignment. */
  uintptr_t raw_page_directory = (uintptr_t)kmalloc(PAGE_SIZE * 2);
  page_directory = (uint32_t *)align_up_uintptr(raw_page_directory, PAGE_SIZE);

  /* Map every 4 MiB virtual range to the same physical address. This is an
     address-space mapping, not a claim that all 4 GiB are backed by RAM. */
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

uint32_t paging_get_kernel_directory(void) { return (uint32_t)page_directory; }

static uint32_t alloc_aligned_page(void) {
  uintptr_t raw = (uintptr_t)kmalloc(PAGE_SIZE * 2);
  return (uint32_t)align_up_uintptr(raw, PAGE_SIZE);
}

uint32_t paging_create_directory(void) {
  uint32_t dir = alloc_aligned_page();
  memcpy((void *)dir, (void *)page_directory, PAGE_SIZE);
  return dir;
}

void paging_map_page(uint32_t pd, uint32_t vaddr, uint32_t paddr,
                     uint32_t flags) {
  /* A 32-bit virtual address is split into a 10-bit directory index, a
     10-bit table index and a 12-bit offset. Only the page-aligned physical
     address goes in the entry; the low bits hold permissions and present. */
  uint32_t dir_idx = vaddr >> 22;
  uint32_t table_idx = (vaddr >> 12) & 0x3FF;

  uint32_t *dir = (uint32_t *)pd;
  uint32_t entry = dir[dir_idx];

  /* Split the identity-mapped large page before replacing one 4 KiB slot;
     retain identity mappings for the other slots in this directory entry. */
  if (entry & PAGE_SIZE_4MB) {
    uint32_t table = alloc_aligned_page();
    uint32_t phys_base = entry & 0xFFC00000U;
    for (uint32_t i = 0; i < 1024; ++i) {
      ((uint32_t *)table)[i] =
          (phys_base + (i << 12)) | PAGE_PRESENT | PAGE_WRITABLE;
    }
    dir[dir_idx] = (table & 0xFFFFF000U) | PAGE_PRESENT | PAGE_WRITABLE;
    entry = dir[dir_idx];
  }

  uint32_t *table = (uint32_t *)(entry & 0xFFFFF000U);
  table[table_idx] = (paddr & 0xFFFFF000U) | flags | PAGE_PRESENT;
}

uint32_t paging_clone_directory(uint32_t parent_pd) {
  /* Copy marked application pages eagerly; shared large kernel mappings
     come from paging_create_directory. There is no copy-on-write. */
  uint32_t new_dir = paging_create_directory();
  uint32_t *parent = (uint32_t *)parent_pd;

  for (uint32_t dir_idx = 0; dir_idx < PAGE_DIRECTORY_ENTRIES; ++dir_idx) {
    uint32_t entry = parent[dir_idx];
    if (!(entry & PAGE_PRESENT) || (entry & PAGE_SIZE_4MB))
      continue;

    uint32_t *table = (uint32_t *)(entry & 0xFFFFF000U);
    for (uint32_t table_idx = 0; table_idx < 1024; ++table_idx) {
      uint32_t pte = table[table_idx];
      if (!(pte & PAGE_PRESENT) || !(pte & PAGE_USER))
        continue;

      /* Reconstruct the virtual address from its indices and copy the page
         contents to a new physical page. PAGE_USER is used here as the mark
         for pages to clone, not as an effective ring-0 protection boundary. */
      uint32_t vaddr = (dir_idx << 22) | (table_idx << 12);
      uint32_t new_page = alloc_aligned_page();
      memcpy((void *)new_page, (void *)(pte & 0xFFFFF000U), PAGE_SIZE);
      paging_map_page(new_dir, vaddr, new_page, pte & 0xFFF);
    }
  }

  return new_dir;
}

void paging_switch(uint32_t pd) { write_cr3(pd); }

void paging_free_directory(uint32_t pd) {
  /* This clears mappings but does not reclaim their backing allocations or
     the page tables themselves. Repeated fork/exec can therefore leak RAM. */
  if (!pd || pd == (uint32_t)page_directory)
    return;
  uint32_t *dir = (uint32_t *)pd;
  for (uint32_t dir_idx = 0; dir_idx < PAGE_DIRECTORY_ENTRIES; ++dir_idx) {
    uint32_t entry = dir[dir_idx];
    if (!(entry & PAGE_PRESENT) || (entry & PAGE_SIZE_4MB))
      continue;
    uint32_t *table = (uint32_t *)(entry & 0xFFFFF000U);
    for (uint32_t table_idx = 0; table_idx < 1024; ++table_idx) {
      uint32_t pte = table[table_idx];
      if (!(pte & PAGE_PRESENT) || !(pte & PAGE_USER))
        continue;
      table[table_idx] = 0;
    }
    dir[dir_idx] = page_directory[dir_idx];
  }
  (void)pd;
}
