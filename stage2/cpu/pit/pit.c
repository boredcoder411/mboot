// Configure the programmable interval timer that drives preemptive scheduling.
#include "cpu/pit/pit.h"
#include "io.h"

/* Channel 0 produces the periodic interrupt used to preempt runnable tasks. */
void pit_init() {
  /* The approximately 1.193 MHz PIT input divided by 1193 gives about a
     1 kHz scheduler tick. Command 0x36 selects channel 0, low/high bytes,
     mode 3 square wave, binary counting. */
  uint16_t divisor = 1193;

  outb(PIT_COMMAND, 0x36);

  outb(PIT_CHANNEL0, divisor & 0xFF);
  outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);
}
