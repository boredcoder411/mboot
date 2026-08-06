#pragma once

#include <stdint.h>

// we're in vga tty mode
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_BUF 0xB8000

#define VGA_REG_DEST 0x3D4
#define VGA_REG_DATA 0x3D5

void vga_write(char c);