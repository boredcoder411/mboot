#include "dev/keyboard.h"
#include "cpu/interrupts/irq.h"
#include "cpu/pic/pic.h"
#include "dev/serial.h"
#include <stdbool.h>

bool shifted = false;
const char scancode_map[] = {
    0,   27,   '1',  '2', '3',  '4', '5', '6', '7', '8', '9', '0', '-',
    '=', '\b', '\t', 'q', 'w',  'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    '[', ']',  '\n', 0,   'a',  's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
    ';', '\'', '`',  0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',',
    '.', '/',  0,    '*', 0,    ' ', 0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   '-', 0,   0,   0,
    '+', 0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,
};

const char shift_scancode_map[] = {
    0,   27,   '!',  '@', '#', '$', '%', '^', '&', '*', '(', ')', '_',
    '+', '\b', '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
    '{', '}',  '\n', 0,   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
    ':', '"',  '~',  0,   '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<',
    '>', '?',  0,    '*', 0,   ' ', 0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   '-', 0,   0,   0,
    '+', 0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,
};

#pragma GCC diagnostic ignored "-Wunused-parameter"
void keyboard_handler(registers_t *r) {
  uint8_t scancode = inb(0x60);

  switch (scancode) {
  case 0x2A:
    shifted = true;
    return;

  case 0x36:
    shifted = true;
    return;

  case 0xAA:
    shifted = false;
    return;

  case 0xB6:
    shifted = false;
    return;
  }

  if (scancode & 0x80) {
    return;
  }

  const char *map = shifted ? shift_scancode_map : scancode_map;
  char ascii_char[3];
  ascii_char[0] = map[scancode];
  ascii_char[1] = '\n';
  ascii_char[2] = '\0';

  if (ascii_char[0]) {
    serial_print(ascii_char);
  }
}

void install_keyboard() {
  install_irq(1, keyboard_handler);
  pic_clear_mask(1);
}
