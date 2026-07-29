#include "libc.h"

static char buffer[256];

void main() {
  char buf[64];
  snprintf(buf, sizeof(buf), "%.7g", 1.5);
  snprintf(buf, sizeof(buf), "%.7g", 3.14);
  write(1, "OK\n", 3);
  exit(0);
}
