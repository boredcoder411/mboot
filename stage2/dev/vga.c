// VGA text-mode output and hardware cursor operations used by the terminal layer.
#include "dev/vga.h"
#include "io.h"
#include "mem.h"

static uint16_t *vga_buffer_ptr(void) { return (uint16_t *)VGA_BUF; }

// https://wiki.osdev.org/Text_Mode_Cursor#Moving_the_Cursor_2
/* The VGA cursor is a linear cell index written through the CRT ports. */
void vga_move_cursor(int x, int y) {
  uint16_t pos = y * VGA_WIDTH + x;

  outb(VGA_REG_DEST, 0x0F);
  outb(VGA_REG_DATA, (uint8_t)(pos & 0xFF));
  outb(VGA_REG_DEST, 0x0E);
  outb(VGA_REG_DATA, (uint8_t)((pos >> 8) & 0xFF));
}

void vga_put_char(int x, int y, char c) {
  /* VGA text memory is a row-major array of 16-bit cells. The low byte is
     the character; the upper byte selects foreground/background attributes. */
  vga_buffer_ptr()[y * VGA_WIDTH + x] = (uint16_t)c | VGA_WHITE_ON_BLACK;
}

void vga_clear_char(int x, int y) {
  vga_buffer_ptr()[y * VGA_WIDTH + x] = (uint16_t)' ' | VGA_WHITE_ON_BLACK;
}

void vga_clear_row(int row) {
  uint16_t blank = (uint16_t)' ' | VGA_WHITE_ON_BLACK;
  uint16_t *buf = vga_buffer_ptr();
  for (int x = 0; x < VGA_WIDTH; x++)
    buf[row * VGA_WIDTH + x] = blank;
}

void vga_clear_screen(void) {
  uint16_t blank = (uint16_t)' ' | VGA_WHITE_ON_BLACK;
  uint16_t *buf = vga_buffer_ptr();
  for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
    buf[i] = blank;
}

/* Each text cell contains a character byte and an attribute byte. */
void vga_draw_buffer(const uint16_t *cells) {
  memcpy(vga_buffer_ptr(), cells, VGA_WIDTH * VGA_HEIGHT * sizeof(uint16_t));
}

char vga_get_char(int x, int y) {
  return (char)(vga_buffer_ptr()[y * VGA_WIDTH + x] & 0xFF);
}

uint16_t vga_get_entry(int x, int y) {
  return vga_buffer_ptr()[y * VGA_WIDTH + x];
}
