#include "dev/serial.h"
#include "io.h"
#include <stdarg.h>
#include <stdbool.h>

int init_serial(void) {
  outb(SERIAL_PORT + 1, 0x00); // Disable interrupts
  outb(SERIAL_PORT + 3, 0x80); // Enable DLAB
  outb(SERIAL_PORT + 0, 0x03); // Divisor low byte (38400 baud)
  outb(SERIAL_PORT + 1, 0x00); // Divisor high byte
  outb(SERIAL_PORT + 3, 0x03); // 8 bits, no parity, one stop
  outb(SERIAL_PORT + 2, 0xC7); // Enable FIFO, clear, 14-byte threshold
  outb(SERIAL_PORT + 4, 0x0B); // IRQs enabled, RTS/DSR set
  outb(SERIAL_PORT + 4, 0x1E); // Loopback test mode
  outb(SERIAL_PORT + 0, 0xAE); // Test byte

  // Check if serial chip echoes correctly
  if (inb(SERIAL_PORT + 0) != 0xAE) {
    return 1; // Error
  }

  // Normal operation mode
  outb(SERIAL_PORT + 4, 0x0F);
  return 0;
}

static inline int is_transmit_empty(void) {
  return inb(SERIAL_PORT + 5) & 0x20;
}

void write_serial(char a) {
  while (!is_transmit_empty());
  outb(SERIAL_PORT, a);
}

void serial_print(const char *str) {
  while (*str)
    write_serial(*str++);
}

static void serial_print_str(const char *s) {
  while (*s)
    write_serial(*s++);
}

static void serial_print_uint(unsigned int value, int base, int width,
                              bool pad_zero, bool uppercase) {
  char buf[32];
  const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
  int i = 0;

  if (value == 0) {
    buf[i++] = '0';
  } else {
    while (value > 0 && i < (int)sizeof(buf)) {
      buf[i++] = digits[value % base];
      value /= base;
    }
  }

  while (i < width) {
    buf[i++] = pad_zero ? '0' : ' ';
  }

  for (int j = i - 1; j >= 0; j--) {
    write_serial(buf[j]);
  }
}

static void serial_print_int(int value, int base, int width,
                             bool pad_zero, bool uppercase) {
  if (base == 10 && value < 0) {
    write_serial('-');
    serial_print_uint((unsigned int)(-value), base, width, pad_zero, uppercase);
  } else {
    serial_print_uint((unsigned int)value, base, width, pad_zero, uppercase);
  }
}

void serial_printf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  for (const char *p = fmt; *p; p++) {
    if (*p != '%') {
      write_serial(*p);
      continue;
    }

    p++;

    bool pad_zero = false;
    int width = 0;

    if (*p == '0') {
      pad_zero = true;
      p++;
    }

    while (*p >= '0' && *p <= '9') {
      width = width * 10 + (*p - '0');
      p++;
    }

    switch (*p) {
      case 'c': {
        char c = (char)va_arg(args, int);
        write_serial(c);
        break;
      }
      case 's': {
        const char *s = va_arg(args, const char *);
        serial_print_str(s ? s : "(null)");
        break;
      }
      case 'd':
      case 'i':
        serial_print_int(va_arg(args, int), 10, width, pad_zero, false);
        break;
      case 'u':
        serial_print_uint(va_arg(args, unsigned int), 10, width, pad_zero, false);
        break;
      case 'x':
        serial_print_uint(va_arg(args, unsigned int), 16, width, pad_zero, false);
        break;
      case 'X':
        serial_print_uint(va_arg(args, unsigned int), 16, width, pad_zero, true);
        break;
      case '%':
        write_serial('%');
        break;
      default:
        write_serial('%');
        write_serial(*p);
        break;
    }
  }

  va_end(args);
}

