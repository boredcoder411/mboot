#include "libc.h"

#define __NR_exit 1
#define __NR_read 3
#define __NR_write 4
#define __NR_open 5
#define __NR_close 6

void put_pixel(int x, int y, uint8_t color) {
  asm volatile("int $0x80" : : "a"(255), "b"(x), "c"(y), "d"(color));
}

void clear_screen(void) {
  asm volatile("int $0x80" : : "a"(254));
}

uint8_t read_key(void) {
  uint8_t key;
  asm volatile("int $0x80" : "=a"(key) : "a"(253));
  return key;
}

void sleep_ms(uint32_t ms) {
  asm volatile("int $0x80" : : "a"(252), "b"(ms));
}

int has_key(void) {
  int result;
  asm volatile("int $0x80" : "=a"(result) : "a"(251));
  return result;
}

ssize_t read(int fd, void *buf, size_t count) {
  ssize_t ret;
  asm volatile("int $0x80"
               : "=a"(ret)
               : "0"(__NR_read), "b"(fd), "c"(buf), "d"(count));
  return ret;
}

ssize_t write(int fd, const void *buf, size_t count) {
  ssize_t ret;
  asm volatile("int $0x80"
               : "=a"(ret)
               : "0"(__NR_write), "b"(fd), "c"(buf), "d"(count));
  return ret;
}

void exit(int code) {
  asm volatile("int $0x80" : : "a"(__NR_exit), "b"(code));
  while (1) {
    asm volatile("hlt");
  }
}

int open(const char *pathname, int flags) {
  int ret;
  asm volatile("int $0x80"
               : "=a"(ret)
               : "0"(__NR_open), "b"(pathname), "c"(flags), "d"(0));
  return ret;
}

int close(int fd) {
  int ret;
  asm volatile("int $0x80"
               : "=a"(ret)
               : "0"(__NR_close), "b"(fd));
  return ret;
}

void *memcpy(void *dst, const void *src, uint32_t size) {
  uint8_t *d = (uint8_t *)dst;
  const uint8_t *s = (const uint8_t *)src;
  for (uint32_t i = 0; i < size; i++) {
    d[i] = s[i];
  }
  return dst;
}

void *memset(void *ptr, int value, uint32_t size) {
  uint8_t *p = (uint8_t *)ptr;
  for (uint32_t i = 0; i < size; i++) {
    p[i] = (uint8_t)value;
  }
  return ptr;
}

char *strcpy(char *dst, const char *src) {
  char *d = dst;
  while (*src) {
    *d++ = *src++;
  }
  *d = '\0';
  return dst;
}

uint32_t strlen(const char *str) {
  uint32_t len = 0;
  while (str[len])
    len++;
  return len;
}

int strcmp(const char *str1, const char *str2) {
  while (*str1 && *str1 == *str2) {
    str1++;
    str2++;
  }
  return (int)*str1 - (int)*str2;
}

int abs(int x) { return x < 0 ? -x : x; }

static uint32_t seed = 0x12345678;

uint32_t rand(void) {
  seed = seed * 1103515245 + 12345;
  return (seed >> 16) & 0x7fff;
}

void srand(uint32_t s) { seed = s; }

void draw_square(int x, int y, int size, uint8_t color) {
  for (int py = y; py < y + size; py++) {
    for (int px = x; px < x + size; px++) {
      if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
        put_pixel(px, py, color);
      }
    }
  }
}

void draw_rect(int x, int y, int width, int height, uint8_t color) {
  for (int py = y; py < y + height; py++) {
    for (int px = x; px < x + width; px++) {
      if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
        put_pixel(px, py, color);
      }
    }
  }
}

void draw_line(int x0, int y0, int x1, int y1, uint8_t color) {
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx - dy;

  while (1) {
    if (x0 >= 0 && x0 < SCREEN_WIDTH && y0 >= 0 && y0 < SCREEN_HEIGHT) {
      put_pixel(x0, y0, color);
    }
    if (x0 == x1 && y0 == y1)
      break;
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

void draw_circle(int cx, int cy, int radius, uint8_t color) {
  int x = 0;
  int y = radius;
  int d = 3 - 2 * radius;

  while (x <= y) {
    if (x >= 0 && x < SCREEN_WIDTH && (cy - y) >= 0 && (cy - y) < SCREEN_HEIGHT)
      put_pixel(cx + x, cy - y, color);
    if (x >= 0 && x < SCREEN_WIDTH && (cy + y) >= 0 && (cy + y) < SCREEN_HEIGHT)
      put_pixel(cx + x, cy + y, color);
    if (y >= 0 && y < SCREEN_WIDTH && (cy - x) >= 0 && (cy - x) < SCREEN_HEIGHT)
      put_pixel(cx + y, cy - x, color);
    if (y >= 0 && y < SCREEN_WIDTH && (cy + x) >= 0 && (cy + x) < SCREEN_HEIGHT)
      put_pixel(cx + y, cy + x, color);
    if ((-x) >= 0 && (-x) < SCREEN_WIDTH && (cy - y) >= 0 &&
        (cy - y) < SCREEN_HEIGHT)
      put_pixel(cx - x, cy - y, color);
    if ((-x) >= 0 && (-x) < SCREEN_WIDTH && (cy + y) >= 0 &&
        (cy + y) < SCREEN_HEIGHT)
      put_pixel(cx - x, cy + y, color);
    if ((-y) >= 0 && (-y) < SCREEN_WIDTH && (cy - x) >= 0 &&
        (cy - x) < SCREEN_HEIGHT)
      put_pixel(cx - y, cy - x, color);
    if ((-y) >= 0 && (-y) < SCREEN_WIDTH && (cy + x) >= 0 &&
        (cy + x) < SCREEN_HEIGHT)
      put_pixel(cx - y, cy + x, color);

    if (d < 0) {
      d = d + 4 * x + 6;
    } else {
      d = d + 4 * (x - y) + 10;
      y--;
    }
    x++;
  }
}

static uint8_t heap[65536];
static uint32_t heap_used = 0;

void *malloc(uint32_t size) {
  if (heap_used + size >= sizeof(heap)) {
    return 0;
  }
  void *ptr = &heap[heap_used];
  heap_used += size;
  return ptr;
}

void free(void *ptr) {
  (void)ptr;
}
