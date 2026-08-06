#include "cpu/interrupts/idt.h"
#include "dev/keyboard.h"
#include "dev/serial.h"
#include "elf.h"
#include "io.h"
#include "net/arp.h"
#include "net/icmp.h"
#include "net/ipv4.h"
#include "net/udp.h"
#include "scheduler.h"
#include "vfs.h"
#include "dev/vga.h"

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
    if (r->ebx == 0) { // stdin — line-buffered keyboard with echo
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

      // Read a fresh line from the keyboard with echo
      line_fill = 0;
      line_pos = 0;
      for (;;) {
        uint8_t c = keyboard_read_key();
        if (c == '\r')
          c = '\n';
        if (c == '\b' || c == 0x7F) {
          if (line_fill > 0) {
            line_fill--;
            vga_write('\b');
            vga_write(' ');
            vga_write('\b');
          }
        } else if (c == '\n') {
          if (line_fill < (int)sizeof(line_buf))
            line_buf[line_fill++] = '\n';
          vga_write('\r');
          vga_write('\n');
          break;
        } else if (c >= ' ' && c < 0x7F) {
          if (line_fill < (int)sizeof(line_buf) - 1) {
            line_buf[line_fill++] = c;
            vga_write(c);
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
          vga_write(buf[i]);
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
  case 200: { // sys_net_init
    const uint8_t *src_ip = (const uint8_t *)r->ebx;
    ipv4_set_address(src_ip);
    udp_init();
    INFO("SYSCALL", "net_init(%d.%d.%d.%d)", src_ip[0], src_ip[1], src_ip[2],
         src_ip[3]);
    r->eax = 0;
    break;
  }
  case 201: { // sys_arp_request
    const uint8_t *src_ip = (const uint8_t *)r->ebx;
    const uint8_t *target_ip = (const uint8_t *)r->ecx;
    arp_send_request((uint8_t *)src_ip, (uint8_t *)target_ip);
    r->eax = 0;
    break;
  }
  case 202: { // sys_arp_lookup
    const uint8_t *ip = (const uint8_t *)r->ebx;
    uint8_t *mac_out = (uint8_t *)r->ecx;
    r->eax = arp_try_get_mac((uint8_t *)ip, mac_out) ? 0 : -1;
    break;
  }
  case 203: { // sys_icmp_echo
    const uint8_t *src_ip = (const uint8_t *)r->ebx;
    const uint8_t *dst_ip = (const uint8_t *)r->ecx;
    const uint8_t *next_hop_mac = (const uint8_t *)r->edx;
    uint16_t identifier = (uint16_t)r->esi;
    uint16_t sequence = (uint16_t)r->edi;
    r->eax = icmp_send_echo((uint8_t *)src_ip, (uint8_t *)dst_ip,
                            (uint8_t *)next_hop_mac, identifier, sequence);
    break;
  }
  case 204: { // sys_udp_server_bind
    r->eax = udp_server_bind((uint16_t)r->ebx);
    break;
  }
  case 205: { // sys_udp_server_recv
    uint16_t port = (uint16_t)r->ebx;
    uint8_t *src_ip_out = (uint8_t *)r->ecx;
    uint16_t *src_port_out = (uint16_t *)r->edx;
    uint8_t *buf = (uint8_t *)r->esi;
    uint16_t max_len = (uint16_t)r->edi;
    uint16_t payload_len = 0;
    int ret =
        udp_server_recv(port, src_ip_out, src_port_out, buf, max_len, &payload_len);
    r->eax = ret < 0 ? -1 : (int)payload_len;
    break;
  }
  case 206: { // sys_udp_send_local
    uint16_t src_port = (uint16_t)r->ebx;
    const uint8_t *dst_ip = (const uint8_t *)r->ecx;
    uint16_t dst_port = (uint16_t)r->edx;
    const uint8_t *payload = (const uint8_t *)r->esi;
    uint16_t payload_len = (uint16_t)r->edi;
    r->eax = udp_send_local(src_port, dst_ip, dst_port, payload, payload_len);
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
