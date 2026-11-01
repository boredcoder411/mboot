// VGA text display and cursor interface.
#pragma once

#include <stdint.h>

// we're in vga tty mode
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_BUF 0xB8000

// colors
#define VGA_WHITE_ON_BLACK (0x0F << 8)

#define VGA_REG_DEST 0x3D4
#define VGA_REG_DATA 0x3D5

void vga_move_cursor(int x, int y);
void vga_put_char(int x, int y, char c);
void vga_clear_char(int x, int y);
void vga_clear_row(int row);
void vga_clear_screen(void);
void vga_draw_buffer(const uint16_t *cells);
char vga_get_char(int x, int y);
uint16_t vga_get_entry(int x, int y);
