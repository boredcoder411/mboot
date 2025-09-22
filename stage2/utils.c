#include "utils.h"
#include "dev/serial.h"

char* itoa(int val) {
  static char buf[32] = {0};
  int i = 30;

  if (val == 0) {
    buf[i] = '0';
    return &buf[i];
  }

  for(; val && i ; --i, val /= 10) {
    buf[i] = "0123456789"[val % 10];
  }

  return &buf[i+1];
}

char* hextoa(int val) {
	static char buf[32] = {0};
	int i = 30;
	for(; val && i ; --i, val /= 16)
		buf[i] = "0123456789ABCDEF"[val % 16];
	return &buf[i+1];
}

bool strncmp(const char* a, const char* b, size_t n) {
  for (size_t i = 0; i < n; i++) {
    if (a[i] != b[i]) {
      return false;
    }
  }
  return true;
}

void hexdump(void* data, size_t size) {
  uint8_t* ptr = (uint8_t*)data;
  for (size_t i = 0; i < size; i++) {
    serial_print(hextoa(ptr[i]));
    serial_print(" ");
  }
  serial_print("\n");
}
