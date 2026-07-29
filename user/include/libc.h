#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <locale.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
#include <limits.h>
#include <float.h>
#include <assert.h>

typedef long ssize_t;

void put_pixel(int x, int y, uint8_t color);
void clear_screen(void);
uint8_t read_key(void);
void sleep_ms(uint32_t ms);
int has_key(void);

void draw_square(int x, int y, int size, uint8_t color);
void draw_circle(int cx, int cy, int radius, uint8_t color);
void draw_line(int x0, int y0, int x1, int y1, uint8_t color);
void draw_rect(int x, int y, int width, int height, uint8_t color);

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

#define VGA_BLACK 0
#define VGA_RED 108
#define VGA_GREEN 36
#define VGA_YELLOW 180
#define VGA_BLUE 145
#define VGA_CYAN 73
#define VGA_MAGENTA 109
#define VGA_WHITE 215

int open(const char *pathname, int flags);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int close(int fd);
