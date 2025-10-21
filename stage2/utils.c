#include "utils.h"
#include "dev/serial.h"

char *int_to_str(int32_t val) {
  static char buf[32];
  int i = 30;
  bool negative = false;
  buf[31] = '\0';

  if (val == 0)
    return "0";

  if (val < 0) {
    negative = true;
    val = -val;
  }

  while (val && i) {
    buf[i--] = "0123456789"[val % 10];
    val /= 10;
  }

  if (negative)
    buf[i--] = '-';
  return &buf[i + 1];
}

char *uint_to_str(uint32_t val) {
  static char buf[32];
  int i = 30;
  buf[31] = '\0';

  if (val == 0)
    return "0";

  while (val && i) {
    buf[i--] = "0123456789"[val % 10];
    val /= 10;
  }

  return &buf[i + 1];
}

char *uint_to_hex(uint64_t val) {
  static char buf[32];
  int i = 30;
  buf[31] = '\0';

  if (val == 0)
    return "0";

  while (val && i) {
    buf[i--] = "0123456789ABCDEF"[val % 16];
    val /= 16;
  }

  return &buf[i + 1];
}

char *hextoa(uint64_t val) {
  static char buf[32] = {0};
  int i = 30;
  if (val == 0)
    return "0";
  for (; val && i; --i, val /= 16)
    buf[i] = "0123456789ABCDEF"[val % 16];
  return &buf[i + 1];
}

bool strncmp(const char *a, const char *b, size_t n) {
  for (size_t i = 0; i < n; i++) {
    if (a[i] != b[i]) {
      return false;
    }
  }
  return true;
}

void hexdump(void *data, size_t size) {
  uint8_t *ptr = (uint8_t *)data;
  for (size_t i = 0; i < size; i++) {
    serial_printf("%u ", ptr[i]);
  }
  serial_printf("\n");
}

float bytes_to_gb(uint64_t bytes) {
  const double bytes_per_gb = 1024.0 * 1024.0 * 1024.0;
  return (float)(bytes / bytes_per_gb);
}

void print_float(float f) {
  int whole = (int)f;
  int decimal = (int)((f - whole) * 1000);
  if (decimal < 0)
    decimal = -decimal;

  serial_printf(int_to_str(whole));
  serial_printf(".");
  serial_printf(uint_to_str(decimal));
}

uint8_t bcd_to_bin(uint8_t val) { return (val & 0x0F) + ((val / 16) * 10); }
