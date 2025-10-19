#include "cpu/pit/pit.h"
#include "io.h"

void pit_init() {
  uint16_t divisor = 19091;

  outb(0x36, PIT_COMMAND);

  outb(divisor & 0xFF, PIT_CHANNEL0);
  outb((divisor >> 8) & 0xFF, PIT_CHANNEL0);
}
