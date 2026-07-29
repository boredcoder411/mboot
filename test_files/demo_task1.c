#include "libc.h"

void main() {
  while (1) {
    write(1, "Hello from userspace demo task 1\n", 33);
    for (volatile int i = 0; i < 10000000; i++)
      ;
  }
}
