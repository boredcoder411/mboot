#pragma once

#include <stdint.h>

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200
#define VIDEO_MEMORY (uint8_t*)0xA0000

#define abs(x) ((x) < 0 ? -(x) : (x))

void put_pixel(int x, int y, uint8_t color);