#include "vga.h"
#include "io.h"

void put_pixel(int x, int y, uint8_t color) {
    uint8_t* video_memory = (uint8_t*)VIDEO_MEMORY;
    video_memory[y * SCREEN_WIDTH + x] = color;
}

void draw_line(int x0, int y0, int x1, int y1, uint8_t color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        put_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int err2 = err * 2;
        if (err2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (err2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void set_vga_dac_entry(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    outb(0x3C8, index);
    outb(0x3C9, r);
    outb(0x3C9, g);
    outb(0x3C9, b);
}

void remap_vga_dac() {
    uint8_t r, g, b;
    int idx = 0;

    for (r = 0; r < 6; r++) {
        for (g = 0; g < 6; g++) {
            for (b = 0; b < 6; b++) {
                set_vga_dac_entry(idx++, r * 10, g * 10, b * 10);
            }
        }
    }

    for (; idx < 256; idx++) {
        uint8_t v = (idx - 216) * 4;
        set_vga_dac_entry(idx, v, v, v);
    }
}

void display_glyph(uint8_t *glyphs, uint32_t glyph_index, int x, int y, uint8_t color) {
    for (uint32_t row = 0; row < 8; row++) {
        uint8_t row_byte = glyphs[glyph_index * 8 + row];
        for (int bit = 7; bit >= 0; bit--) {
            if (row_byte & (1 << bit)) {
                put_pixel(x + (7 - bit), y + row, color);
            }
        }
    }
}

void display_imf(imf_t *imf, int pos_x, int pos_y) {
  for (uint8_t j = 0; j < imf->y; j++) {
    for (uint8_t i = 0; i < imf->x; i++) {
      put_pixel(pos_x + i, pos_y + j, imf->colors[j * imf->x + i]);
    }
  }
}
