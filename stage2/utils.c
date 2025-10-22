#include "utils.h"
#include "dev/serial.h"

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

uint8_t bcd_to_bin(uint8_t val) { return (val & 0x0F) + ((val / 16) * 10); }
