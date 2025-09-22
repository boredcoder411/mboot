#include <stdint.h>
#include <stdbool.h>
#include "utils.h"
#include "vga.h"
#include "serial.h"
#include "cpu/interrupts/idt.h"
#include "cpu/interrupts/isr.h"
#include "cpu/interrupts/irq.h"
#include "cpu/pic/pic.h"
#include "cpu/pit/pit.h"

extern void enable_fpu();
extern void div0_fault();

// taylor of sin(x)
float sin(float x) {
  float x3 = x * x * x;
  float x5 = x3 * x * x;
  float x7 = x5 * x * x;
  float ret = x - (x3 / 6) + (x5 / 120) - (x7 / 5040);
  return ret;
}

void sin_demo() {
  for (int x = 0; x < SCREEN_WIDTH; x++) {
    int y = (SCREEN_HEIGHT / 2) + (int)(20 * sin((float)x / 10.0f));
    put_pixel(x, y, 15);
  }
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void keyboard_handler(registers_t *r) {
  uint8_t scancode = inb(0x60);
  serial_print("keyboard: ");
  serial_print(itoa(scancode));
  serial_print("\n");
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void pit_handler(registers_t *r) {
  sin_demo();
}

void loader_start() {
  // mask all IRQs on the pic because they aren't set yet
  for (int i = 0; i < IRQs; i++) {
    pic_set_mask(i);
  }

  pic_remap();
  idt_init();
  install_exception_isrs();
  pit_init();
  install_irq(0, pit_handler);
  pic_clear_mask(0);
  enable_fpu();
  install_irq(1, keyboard_handler);
  pic_clear_mask(1);
  asm("sti");

  for (int x = 0; x < SCREEN_WIDTH; x++) {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
      put_pixel(x, y, 0);
    }
  }

  while (1);
}