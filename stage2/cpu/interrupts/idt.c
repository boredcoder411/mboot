#include "cpu/interrupts/idt.h"
#include "dev/serial.h"
#include "elf.h"
#include "io.h"
#include "scheduler.h"
#include "vfs.h"

extern void syscall_handler();

void syscall_dispatch(registers_t *r) {
  switch (r->eax) {
  case 1: { // sys_exit
    uint32_t exit_code = r->ebx;
    INFO("SYSCALL", "exit(%d)", exit_code);
    scheduler_exit_current(exit_code);
    break;
  }
  case 3: {            // sys_read
    if (r->ebx == 0) { // stdin — line-buffered serial with echo
      static char line_buf[256];
      static int line_fill = 0;
      static int line_pos = 0;

      // Drain already-buffered line
      if (line_pos < line_fill) {
        uint32_t take = r->edx;
        if ((uint32_t)(line_fill - line_pos) < take)
          take = line_fill - line_pos;
        uint8_t *dst = (uint8_t *)r->ecx;
        for (uint32_t i = 0; i < take; i++)
          dst[i] = line_buf[line_pos++];
        r->eax = take;
        break;
      }

      // Read a fresh line from serial with echo
      line_fill = 0;
      line_pos = 0;
      for (;;) {
        uint8_t c = read_serial();
        if (c == '\r')
          c = '\n';
        if (c == '\b' || c == 0x7F) {
          if (line_fill > 0) {
            line_fill--;
            write_serial('\b');
            write_serial(' ');
            write_serial('\b');
          }
        } else if (c == '\n') {
          if (line_fill < (int)sizeof(line_buf))
            line_buf[line_fill++] = '\n';
          write_serial('\r');
          write_serial('\n');
          break;
        } else if (c >= ' ' && c < 0x7F) {
          if (line_fill < (int)sizeof(line_buf) - 1) {
            line_buf[line_fill++] = c;
            write_serial(c);
          }
        }
      }

      // Return first byte(s)
      if (line_fill > 0) {
        uint32_t take = r->edx;
        if ((uint32_t)line_fill < take)
          take = line_fill;
        uint8_t *dst = (uint8_t *)r->ecx;
        for (uint32_t i = 0; i < take; i++)
          dst[i] = line_buf[line_pos++];
        r->eax = take;
      } else {
        r->eax = 0;
      }
    } else if (r->ebx > 2) {
      uint32_t fd = r->ebx - 3;
      INFO("SYSCALL", "read(fd=%u, count=%u)", fd, r->edx);
      r->eax = read_file(fd, r->edx, (void *)r->ecx);
      INFO("SYSCALL", "read -> %d", r->eax);
    } else {
      r->eax = -1;
    }
    break;
  }
  case 4: { // sys_write
    if (r->ebx == 1 || r->ebx == 2) {
      if (r->ecx && r->edx > 0) {
        INFO("SYSCALL", "write(fd=%u, len=%u)", r->ebx, r->edx);
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
  case 11: { // sys_exec
    const char *pathname = (const char *)r->ebx;
    char *argv[] = {(char *)pathname, NULL};
    int pid = spawn_elf(pathname, 1, argv);
    INFO("SYSCALL", "exec(\"%s\") = %d", pathname, pid);
    r->eax = pid;
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

  idt_set_gate(0x80, (uint32_t)syscall_handler, 0x08, 0xEF);

  idt_load((uint32_t)&idt_ptr);
}
