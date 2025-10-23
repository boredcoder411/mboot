#ifdef TIME_DEMO
#include "dev/rtc.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"
void pit_handler(registers_t *r) {
  clear_screen();
  char *time = fetch_rtc();
  display_string(time, VGA_WHITE);
}
#endif
