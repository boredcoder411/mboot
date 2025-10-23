#pragma once

#include "imf.h"
#include <stdint.h>

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200
#define VIDEO_MEMORY (uint8_t *)0xA0000

#define abs(x) ((x) < 0 ? -(x) : (x))
#define VGA_INDEX(r, g, b) ((uint8_t)(((r) * 36) + ((g) * 6) + (b)))

#define VGA_BLACK VGA_INDEX(0, 0, 0)
#define VGA_BLUE VGA_INDEX(0, 0, 3)
#define VGA_GREEN VGA_INDEX(0, 3, 0)
#define VGA_CYAN VGA_INDEX(0, 3, 3)
#define VGA_RED VGA_INDEX(3, 0, 0)
#define VGA_MAGENTA VGA_INDEX(3, 0, 3)
#define VGA_BROWN VGA_INDEX(3, 2, 0)
#define VGA_LIGHT_GRAY VGA_INDEX(4, 4, 4)
#define VGA_DARK_GRAY VGA_INDEX(2, 2, 2)
#define VGA_LIGHT_RED VGA_INDEX(5, 1, 1)
#define VGA_LIGHT_GREEN VGA_INDEX(1, 5, 1)
#define VGA_LIGHT_BLUE VGA_INDEX(1, 1, 5)
#define VGA_LIGHT_CYAN VGA_INDEX(2, 5, 5)
#define VGA_LIGHT_MAGENTA VGA_INDEX(5, 1, 5)
#define VGA_YELLOW VGA_INDEX(5, 5, 0)
#define VGA_WHITE VGA_INDEX(5, 5, 5)

#define VGA_GRAY(n) ((uint8_t)(216 + ((n) & 0x0F)))

void glyph_init(uint8_t *glyphs);
void put_pixel(int x, int y, uint8_t color);
void draw_line(int x0, int y0, int x1, int y1, uint8_t color);
void remap_vga_dac();
void display_string(char *str, uint8_t color);
void display_imf(imf_t *imf, int pos_x, int pos_y);
void clear_screen();
