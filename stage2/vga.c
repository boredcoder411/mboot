#include "vga.h"

void put_pixel(int x, int y, uint8_t color) {
    uint8_t* video_memory = (uint8_t*)VIDEO_MEMORY;
    video_memory[y * SCREEN_WIDTH + x] = color;
}