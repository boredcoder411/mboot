#include "dev/vga.h"
#include "io.h"
#include "mem.h"

int cur_x = 0;
int cur_y = 0;

// https://wiki.osdev.org/Text_Mode_Cursor#Moving_the_Cursor_2
void update_cursor(int x, int y) {
  uint16_t pos = y * VGA_WIDTH + x;

  outb(0x3D4, 0x0F);
  outb(0x3D5, (uint8_t)(pos & 0xFF));
  outb(0x3D4, 0x0E);
  outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void vga_write(char c) {
  uint16_t pos = cur_y * VGA_WIDTH + cur_x;
  uint16_t *vga_buffer = (uint16_t *)VGA_BUF;

  switch (c) {
  case '\n':
    cur_x = 0;
    cur_y++;
    break;
  case '\r':
    cur_x = 0;
    break;
  case '\b':
    if (cur_x > 0) {
      cur_x--;
      vga_buffer[pos] = (uint16_t)' ' | VGA_WHITE_ON_BLACK;
    }
    break;
  case '\t':
    cur_x = (cur_x + 8) & ~(8 - 1);
    break;
  default:
    vga_buffer[pos] = (uint16_t)c | VGA_WHITE_ON_BLACK;
    cur_x++;
    break;
  }

  if (cur_x >= VGA_WIDTH) {
    cur_x = 0;
    cur_y++;
  }

  if (cur_y >= VGA_HEIGHT) {
    for (int y = 1; y < VGA_HEIGHT; y++) {
      memcpy(&vga_buffer[(y - 1) * VGA_WIDTH], &vga_buffer[y * VGA_WIDTH],
             VGA_WIDTH * sizeof(uint16_t));
    }

    uint16_t blank = (uint16_t)' ' | VGA_WHITE_ON_BLACK;
    for (int x = 0; x < VGA_WIDTH; x++) {
      vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = blank;
    }
    cur_y = VGA_HEIGHT - 1;
  }

  update_cursor(cur_x, cur_y);
}