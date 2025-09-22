#include "keyboard.h"
#include "dev/serial.h"
#include "utils.h"
#include "cpu/interrupts/irq.h"
#include "cpu/pic/pic.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"
void keyboard_handler(registers_t *r) {
  uint8_t scancode = inb(0x60);
  serial_print("keyboard: ");
  serial_print(itoa(scancode));
  serial_print("\n");
}

void install_keyboard() {
  install_irq(1, keyboard_handler);
  pic_clear_mask(1);
}