#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef long ssize_t;

void exit(int code);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int open(const char *pathname, int flags);
int close(int fd);

void put_pixel(int x, int y, uint8_t color);
void clear_screen(void);
uint8_t read_key(void);
void sleep_ms(uint32_t ms);
int has_key(void);

uint32_t rand(void);
void srand(uint32_t seed);
int abs(int x);
void *memcpy(void *dst, const void *src, uint32_t size);
void *memset(void *ptr, int value, uint32_t size);
char *strcpy(char *dst, const char *src);
uint32_t strlen(const char *str);
int strcmp(const char *str1, const char *str2);

void *malloc(uint32_t size);
void free(void *ptr);

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
