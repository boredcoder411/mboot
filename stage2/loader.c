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

typedef struct {
  int x;
  int y;
} __attribute__((packed)) Vec2;

Vec2 square_pos = { SCREEN_WIDTH / 2 - 10, SCREEN_HEIGHT / 2 - 10 };
Vec2 square_speed = { 1, 1 };

void draw_square() {
  for (int x = 0; x < 20; x++) {
    for (int y = 0; y < 20; y++) {
      put_pixel(square_pos.x + x, square_pos.y + y, 15);
    }
  }

  for (int x = 0; x < 20; x++) {
    for (int y = 0; y < 20; y++) {
      put_pixel(square_pos.x + x, square_pos.y + y, 0);
    }
  }

  square_pos.x += square_speed.x;
  square_pos.y += square_speed.y;

  if (square_pos.x <= 0 || square_pos.x >= SCREEN_WIDTH - 20) {
    square_speed.x = -square_speed.x;
  }
  if (square_pos.y <= 0 || square_pos.y >= SCREEN_HEIGHT - 20) {
    square_speed.y = -square_speed.y;
  }

  for (int x = 0; x < 20; x++) {
    for (int y = 0; y < 20; y++) {
      put_pixel(square_pos.x + x, square_pos.y + y, 15);
    }
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
  draw_square();
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