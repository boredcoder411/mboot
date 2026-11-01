// Continuously prints a task-specific message to demonstrate scheduling.
#include "libc.h"

void main() {
  while (1) {
    write(1, "Hello from userspace demo task 1\n", 33);
    /* A volatile busy loop spaces output while allowing timer preemption. */
    for (volatile int i = 0; i < 10000000; i++)
      ;
  }
}
