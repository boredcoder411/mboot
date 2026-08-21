#include "cpu/cpuid.h"
#include "dev/serial.h"
#include "mem.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CPUINFO_BUF_SIZE 2048

static cpu_info_t cpu;
static char cpuinfo_buf[CPUINFO_BUF_SIZE];
static size_t cpuinfo_len;
static int initialized;

static int cpuid_supported(void) {
  uint32_t before;
  uint32_t after;

  asm volatile("pushfl\n"
               "popl %0\n"
               "movl %0, %1\n"
               "xorl $0x00200000, %1\n"
               "pushl %1\n"
               "popfl\n"
               "pushfl\n"
               "popl %1\n"
               "pushl %0\n"
               "popfl"
               : "=&r"(before), "=&r"(after)
               :
               : "cc");

  return ((before ^ after) & 0x00200000U) != 0;
}

static void cpuid_leaf(uint32_t leaf, uint32_t subleaf, uint32_t *a,
                       uint32_t *b, uint32_t *c, uint32_t *d) {
  asm volatile("cpuid"
               : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
               : "a"(leaf), "c"(subleaf));
}

static void append_char(char c) {
  if (cpuinfo_len + 1 >= CPUINFO_BUF_SIZE)
    return;
  cpuinfo_buf[cpuinfo_len++] = c;
  cpuinfo_buf[cpuinfo_len] = '\0';
}

static void append_str(const char *s) {
  if (!s)
    return;
  while (*s)
    append_char(*s++);
}

static void append_uint(uint32_t value) {
  char tmp[10];
  int i = 0;

  if (value == 0) {
    append_char('0');
    return;
  }

  while (value > 0 && i < (int)sizeof(tmp)) {
    tmp[i++] = (char)('0' + (value % 10));
    value /= 10;
  }

  while (i > 0)
    append_char(tmp[--i]);
}

static void append_field_str(const char *name, const char *value) {
  append_str(name);
  append_str("\t: ");
  append_str(value);
  append_char('\n');
}

static void append_field_uint(const char *name, uint32_t value) {
  append_str(name);
  append_str("\t: ");
  append_uint(value);
  append_char('\n');
}

static void append_flag(uint32_t bits, uint32_t mask, const char *name,
                        bool *first) {
  if ((bits & mask) == 0)
    return;
  if (!*first)
    append_char(' ');
  append_str(name);
  *first = false;
}

static void build_cpuinfo_text(void) {
  bool first = true;

  cpuinfo_len = 0;
  cpuinfo_buf[0] = '\0';

  append_field_uint("processor", 0);

  if (!cpu.supported) {
    append_field_str("vendor_id", "unknown");
    append_field_str("model name", "CPUID unsupported");
    return;
  }

  append_field_str("vendor_id", cpu.vendor);
  append_field_uint("cpu family", cpu.family);
  append_field_uint("model", cpu.model);
  append_field_str("model name", cpu.brand[0] ? cpu.brand : "unknown");
  append_field_uint("stepping", cpu.stepping);
  append_field_uint("cpu type", cpu.type);
  append_field_uint("clflush size", cpu.clflush_size);
  append_field_uint("cache_alignment", cpu.clflush_size);
  append_str("address sizes\t: ");
  append_uint(cpu.phys_addr_bits);
  append_str(" bits physical, ");
  append_uint(cpu.virt_addr_bits);
  append_str(" bits virtual\n");
  append_str("flags\t\t: ");

  append_flag(cpu.features_edx, 1U << 0, "fpu", &first);
  append_flag(cpu.features_edx, 1U << 1, "vme", &first);
  append_flag(cpu.features_edx, 1U << 2, "de", &first);
  append_flag(cpu.features_edx, 1U << 3, "pse", &first);
  append_flag(cpu.features_edx, 1U << 4, "tsc", &first);
  append_flag(cpu.features_edx, 1U << 5, "msr", &first);
  append_flag(cpu.features_edx, 1U << 6, "pae", &first);
  append_flag(cpu.features_edx, 1U << 7, "mce", &first);
  append_flag(cpu.features_edx, 1U << 8, "cx8", &first);
  append_flag(cpu.features_edx, 1U << 9, "apic", &first);
  append_flag(cpu.features_edx, 1U << 11, "sep", &first);
  append_flag(cpu.features_edx, 1U << 12, "mtrr", &first);
  append_flag(cpu.features_edx, 1U << 13, "pge", &first);
  append_flag(cpu.features_edx, 1U << 14, "mca", &first);
  append_flag(cpu.features_edx, 1U << 15, "cmov", &first);
  append_flag(cpu.features_edx, 1U << 16, "pat", &first);
  append_flag(cpu.features_edx, 1U << 17, "pse36", &first);
  append_flag(cpu.features_edx, 1U << 19, "clflush", &first);
  append_flag(cpu.features_edx, 1U << 23, "mmx", &first);
  append_flag(cpu.features_edx, 1U << 24, "fxsr", &first);
  append_flag(cpu.features_edx, 1U << 25, "sse", &first);
  append_flag(cpu.features_edx, 1U << 26, "sse2", &first);
  append_flag(cpu.features_ecx, 1U << 0, "sse3", &first);
  append_flag(cpu.features_ecx, 1U << 9, "ssse3", &first);
  append_flag(cpu.features_ecx, 1U << 19, "sse4_1", &first);
  append_flag(cpu.features_ecx, 1U << 20, "sse4_2", &first);
  append_flag(cpu.features_ecx, 1U << 22, "movbe", &first);
  append_flag(cpu.features_ecx, 1U << 23, "popcnt", &first);
  append_flag(cpu.features_ecx, 1U << 25, "aes", &first);
  append_flag(cpu.features_ecx, 1U << 26, "xsave", &first);
  append_flag(cpu.features_ecx, 1U << 28, "avx", &first);
  append_flag(cpu.ext_features_edx, 1U << 20, "nx", &first);
  append_flag(cpu.ext_features_edx, 1U << 29, "lm", &first);
  append_char('\n');
}

void cpuid_init(void) {
  uint32_t a;
  uint32_t b;
  uint32_t c;
  uint32_t d;

  if (initialized)
    return;

  memset(&cpu, 0, sizeof(cpu));
  cpu.phys_addr_bits = 32;
  cpu.virt_addr_bits = 32;

  if (!cpuid_supported()) {
    build_cpuinfo_text();
    initialized = 1;
    WARN("CPUID", "CPUID instruction not supported");
    return;
  }

  cpu.supported = 1;

  cpuid_leaf(0, 0, &a, &b, &c, &d);
  cpu.max_leaf = a;
  memcpy(cpu.vendor, &b, 4);
  memcpy(cpu.vendor + 4, &d, 4);
  memcpy(cpu.vendor + 8, &c, 4);
  cpu.vendor[12] = '\0';

  if (cpu.max_leaf >= 1) {
    cpuid_leaf(1, 0, &a, &b, &c, &d);
    uint32_t base_family = (a >> 8) & 0xF;
    uint32_t base_model = (a >> 4) & 0xF;

    cpu.stepping = a & 0xF;
    cpu.type = (a >> 12) & 0x3;
    cpu.ext_model = (a >> 16) & 0xF;
    cpu.ext_family = (a >> 20) & 0xFF;
    cpu.family =
        base_family == 0xF ? base_family + cpu.ext_family : base_family;
    cpu.model = (base_family == 0x6 || base_family == 0xF)
                    ? base_model + (cpu.ext_model << 4)
                    : base_model;
    cpu.clflush_size = ((b >> 8) & 0xFF) * 8;
    cpu.logical_cpus = (b >> 16) & 0xFF;
    cpu.apic_id = (b >> 24) & 0xFF;
    cpu.features_ecx = c;
    cpu.features_edx = d;
  }

  cpuid_leaf(0x80000000U, 0, &a, &b, &c, &d);
  cpu.max_ext_leaf = a;

  if (cpu.max_ext_leaf >= 0x80000004U) {
    uint32_t *brand = (uint32_t *)cpu.brand;
    cpuid_leaf(0x80000002U, 0, &brand[0], &brand[1], &brand[2], &brand[3]);
    cpuid_leaf(0x80000003U, 0, &brand[4], &brand[5], &brand[6], &brand[7]);
    cpuid_leaf(0x80000004U, 0, &brand[8], &brand[9], &brand[10], &brand[11]);
    cpu.brand[48] = '\0';
  }

  if (cpu.max_ext_leaf >= 0x80000001U) {
    cpuid_leaf(0x80000001U, 0, &a, &b, &c, &d);
    cpu.ext_features_ecx = c;
    cpu.ext_features_edx = d;
  }

  if (cpu.max_ext_leaf >= 0x80000008U) {
    cpuid_leaf(0x80000008U, 0, &a, &b, &c, &d);
    cpu.phys_addr_bits = a & 0xFF;
    cpu.virt_addr_bits = (a >> 8) & 0xFF;
  }

  build_cpuinfo_text();
  initialized = 1;
  INFO("CPUID", "%s %s", cpu.vendor, cpu.brand[0] ? cpu.brand : "unknown CPU");
}

const cpu_info_t *cpuid_info(void) {
  if (!initialized)
    cpuid_init();
  return &cpu;
}

const char *cpuid_cpuinfo_text(void) {
  if (!initialized)
    cpuid_init();
  return cpuinfo_buf;
}

size_t cpuid_cpuinfo_size(void) {
  if (!initialized)
    cpuid_init();
  return cpuinfo_len;
}
