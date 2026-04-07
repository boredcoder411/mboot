#include "cpu/interrupts/idt.h"
#include "dev/keyboard.h"
#include "dev/serial.h"
#include "io.h"
#include "vfs.h"

extern void syscall_handler();

void syscall_dispatch(registers_t *r) {
  switch (r->eax) {
  case 1: { // sys_exit
    uint32_t exit_code = r->ebx;
    INFO("SYSCALL", "exit(%d)", exit_code);
    break;
  }
  case 3: { // sys_read
    if (r->ebx == 0) {
      if (keyboard_has_key()) {
        uint8_t key = keyboard_read_key();
        if (r->ecx && r->edx > 0) {
          *(uint8_t *)r->ecx = key;
          r->eax = 1;
        } else {
          r->eax = -1;
        }
      } else {
        r->eax = 0;
      }
    } else if (r->ebx > 2) {
      r->eax = read_file(r->ebx - 3, r->edx, (void *)r->ecx);
    } else {
      r->eax = -1;
    }
    break;
  }
  case 4: { // sys_write
    if (r->ebx == 1 || r->ebx == 2) {
      if (r->ecx && r->edx > 0) {
        const char *buf = (const char *)r->ecx;
        for (uint32_t i = 0; i < r->edx; i++) {
          write_serial(buf[i]);
        }
        r->eax = r->edx;
        break;
      }
    }
    r->eax = -1;
    break;
  }
  case 5: { // sys_open
    const char *pathname = (const char *)r->ebx;
    int fd = open_file(pathname);
    if (fd >= 0) {
      INFO("SYSCALL", "open(\"%s\") = %d", pathname, fd + 3);
    }
    r->eax = (fd >= 0) ? fd + 3 : fd;
    break;
  }
  case 6: { // sys_close
    int fd = r->ebx;
    if (fd > 2) {
      int result = close_file(fd - 3);
      INFO("SYSCALL", "close(%d) = %d", fd, result);
      r->eax = result;
    } else {
      r->eax = 0;
    }
    break;
  }
  default:
    WARN("SYSCALL", "unknown syscall %d", r->eax);
    r->eax = -1;
  }
}

idt_entry_t idt[IDT_ENTRIES];
idt_ptr_t idt_ptr;

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
  idt[num].base_low = base & 0xFFFF;
  idt[num].sel = sel;
  idt[num].always0 = 0;
  idt[num].flags = flags;
  idt[num].base_high = (base >> 16) & 0xFFFF;
}

void idt_init() {
  idt_ptr.limit = sizeof(idt_entry_t) * IDT_ENTRIES - 1;
  idt_ptr.base = (uint32_t)&idt;

  for (int i = 0; i < IDT_ENTRIES; i++) {
    idt_set_gate(i, 0, 0, 0);
  }

  idt_set_gate(0x80, (uint32_t)syscall_handler, 0x08, 0xEE);

  idt_load((uint32_t)&idt_ptr);
}
