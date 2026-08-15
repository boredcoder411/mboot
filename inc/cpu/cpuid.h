#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct {
  int supported;
  char vendor[13];
  char brand[49];
  uint32_t family;
  uint32_t model;
  uint32_t stepping;
  uint32_t type;
  uint32_t ext_family;
  uint32_t ext_model;
  uint32_t features_ecx;
  uint32_t features_edx;
  uint32_t ext_features_ecx;
  uint32_t ext_features_edx;
  uint32_t max_leaf;
  uint32_t max_ext_leaf;
  uint32_t clflush_size;
  uint32_t logical_cpus;
  uint32_t apic_id;
  uint32_t phys_addr_bits;
  uint32_t virt_addr_bits;
} cpu_info_t;

void cpuid_init(void);
const cpu_info_t *cpuid_info(void);
const char *cpuid_cpuinfo_text(void);
size_t cpuid_cpuinfo_size(void);
