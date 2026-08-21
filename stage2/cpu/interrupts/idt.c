#include "cpu/interrupts/idt.h"
#include "dev/keyboard.h"
#include "dev/serial.h"
#include "dev/vga.h"
#include "elf.h"
#include "io.h"
#include "net/socket.h"
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
  case 2: { // sys_fork
    int pid = fork_current(r);
    r->eax = (pid >= 0) ? (uint32_t)pid : 0xFFFFFFFFU;
    INFO("SYSCALL", "fork() = %d", pid);
    break;
  }
  case 3: { // sys_read
    uint32_t fd = r->ebx;
    INFO("SYSCALL", "read(fd=%u, count=%u)", fd, r->edx);
    r->eax = vfs_read(fd, r->edx, (void *)r->ecx);
    INFO("SYSCALL", "read -> %d", r->eax);
    break;
  }
  case 4: { // sys_write
    uint32_t fd = r->ebx;
    INFO("SYSCALL", "write(fd=%u, len=%u)", fd, r->edx);
    r->eax = vfs_write(fd, (const void *)r->ecx, r->edx);
    if ((int)r->eax > 0 && r->ecx) {
      const char *buf = (const char *)r->ecx;
      for (uint32_t i = 0; i < r->eax; i++)
        write_serial(buf[i]);
    }
    INFO("SYSCALL", "write -> %d", r->eax);
    break;
  }
  case 5: { // sys_open
    const char *pathname = (const char *)r->ebx;
    int fd = vfs_open(pathname);
    if (fd >= 0) {
      INFO("SYSCALL", "open(\"%s\") = %d", pathname, fd);
    }
    r->eax = fd;
    break;
  }
  case 6: { // sys_close
    int fd = r->ebx;
    if (fd > 2) {
      int result = vfs_close(fd);
      INFO("SYSCALL", "close(%d) = %d", fd, result);
      r->eax = result;
    } else {
      r->eax = 0;
    }
    break;
  }
  case 11: { // sys_exec
    const char *pathname = (const char *)r->ebx;
    char **argv = (char **)r->ecx;
    int argc = (int)r->edx;
    int tty = (int)r->esi;
    char *default_argv[] = {(char *)pathname, NULL};
    if (!argv || argc <= 0) {
      argv = default_argv;
      argc = 1;
    }
    int pid = spawn_elf(pathname, argc, argv, tty);
    INFO("SYSCALL", "exec(\"%s\", argc=%d, tty=%d) = %d", pathname, argc, tty,
         pid);
    r->eax = pid;
    break;
  }
  case 63: { // sys_dup2
    int oldfd = (int)r->ebx;
    int newfd = (int)r->ecx;
    r->eax = vfs_dup2(oldfd, newfd);
    INFO("SYSCALL", "dup2(%d, %d) = %d", oldfd, newfd, r->eax);
    break;
  }
  case 300: { // sys_socket
    int handle = socket_create((int)r->ebx, (int)r->ecx, (int)r->edx);
    r->eax = socket_handle_valid(handle) ? vfs_attach_handle(handle) : -1;
    if ((int)r->eax < 0 && socket_handle_valid(handle))
      socket_close(handle);
    INFO("SYSCALL", "socket(%d, %d, %d) = %d", r->ebx, r->ecx, r->edx, r->eax);
    break;
  }
  case 301: { // sys_bind
    int fd = (int)r->ebx;
    int handle = vfs_fd_handle(fd);
    r->eax =
        socket_bind(handle, (const struct sockaddr *)r->ecx, (socklen_t)r->edx);
    INFO("SYSCALL", "bind(%d) = %d", fd, r->eax);
    break;
  }
  case 302: { // sys_sendto
    int fd = (int)r->ebx;
    int handle = vfs_fd_handle(fd);
    r->eax = socket_sendto(handle, (const void *)r->ecx, r->edx, 0,
                            (const struct sockaddr *)r->esi, (socklen_t)r->edi);
    INFO("SYSCALL", "sendto(%d, len=%u) = %d", fd, r->edx, r->eax);
    break;
  }
  case 303: { // sys_recvfrom
    int fd = (int)r->ebx;
    int handle = vfs_fd_handle(fd);
    r->eax = socket_recvfrom(handle, (void *)r->ecx, r->edx, 0,
                             (struct sockaddr *)r->esi, (socklen_t *)r->edi);
    INFO("SYSCALL", "recvfrom(%d, len=%u) = %d", fd, r->edx, r->eax);
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