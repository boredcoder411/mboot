#include "dev/vga.h"
#include "io.h"

int cur_x = 0;
int cur_y = 0;

// https://wiki.osdev.org/Text_Mode_Cursor#Moving_the_Cursor_2
void update_cursor(int x, int y) {
	uint16_t pos = y * VGA_WIDTH + x;

	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
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
                vga_buffer[pos] = (uint16_t)' ' | (0x0F << 8);
            }
            break;
        default:
            vga_buffer[pos] = (uint16_t)c | (0x0F << 8); // white on black
            cur_x++;
            break;
    }

    update_cursor(cur_x, cur_y);
}