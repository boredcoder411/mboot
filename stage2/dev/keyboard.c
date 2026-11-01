// Decode keyboard input and route characters to the active terminal; Alt+F1 through Alt+F4 select terminals.
#include "dev/keyboard.h"
#include "cpu/interrupts/irq.h"
#include "cpu/pic/pic.h"
#include "dev/tty.h"
#include <stdbool.h>

bool shifted = false;
bool alt_down = false;
uint8_t keyboard_buffer[KEYBOARD_BUFFER_SIZE];
uint16_t keyboard_buffer_head = 0;
uint16_t keyboard_buffer_tail = 0;

/* ChatGPT started here */
const char scancode_map[] = {
    0,   27,   '1',  '2', '3',  '4', '5', '6', '7', '8', '9', '0', '\'',
    '^', '\b', '\t', 'q', 'w',  'e', 'r', 't', 'z', 'u', 'i', 'o', 'p',
    'u', 'e',  '\n', 0,   'a',  's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
    'o', 'a',  '$',  0,   '<',  'y', 'x', 'c', 'v', 'b', 'n', 'm', ',',
    '.', '-',  0,    '*', 0,    ' ', 0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   '-', 0,   0,   0,
    '+', 0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,
};

const char shift_scancode_map[] = {
    0,   27,   '+',  '"', '*',  '$', '%', '&', '/', '(', ')', '=', '?',
    '`', '\b', '\t', 'Q', 'W', 'E', 'R', 'T', 'Z', 'U', 'I', 'O', 'P',
    'E', '!',  '\n', 0,   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
    'O', 'A',  '^',  0,   '>',  'Y', 'X', 'C', 'V', 'B', 'N', 'M', ';',
    ':', '_',  0,    '*', 0,   ' ', 0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   '-', 0,   0,   0,
    '+', 0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,
};
/* ChatGPT ended here */

int keyboard_has_key() { return keyboard_buffer_head != keyboard_buffer_tail; }

uint8_t keyboard_read_key() {
  while (!keyboard_has_key()) {
    asm volatile("pause");
  }
  uint8_t key = keyboard_buffer[keyboard_buffer_tail];
  keyboard_buffer_tail = (keyboard_buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
  return key;
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
/* Decode make/break scancodes and modifier state before passing printable
   characters and terminal-switch keys to the active TTY. */
void keyboard_handler(registers_t *r) {
  /* Port 0x60 contains the next PS/2 scancode. Set-1 break codes have their
     high bit set; modifier releases must be handled before filtering them. */
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

  case 0x38:
    alt_down = true;
    return;

  case 0xB8:
    alt_down = false;
    return;
  }

  if (scancode & 0x80) {
    return;
  }

  if (alt_down && scancode >= 0x3B && scancode <= 0x3E) {
    tty_handle_key(scancode, 0);
    return;
  }

  /* The lookup tables encode this project's keyboard layout. They provide
     simple Shift translation rather than a full layout or compose engine. */
  const char *map = shifted ? shift_scancode_map : scancode_map;
  char ascii_char = map[scancode];

  if (ascii_char) {
    tty_handle_key(scancode, ascii_char);

    /* Keep a second circular buffer for the legacy keyboard_read_key API;
       the TTY has its own independent input queue. */
    uint16_t next_head = (keyboard_buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
    if (next_head != keyboard_buffer_tail) { // Buffer not full
      keyboard_buffer[keyboard_buffer_head] = ascii_char;
      keyboard_buffer_head = next_head;
    }
  }
}

void install_keyboard() {
  install_irq(1, keyboard_handler);
  pic_clear_mask(1);
}
