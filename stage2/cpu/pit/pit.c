#include "cpu/pit/pit.h"
#include "io.h"

void pit_init() {
  uint16_t divisor = 1193;

  outb(PIT_COMMAND, 0x36);

  outb(PIT_CHANNEL0, divisor & 0xFF);
  outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);
}
