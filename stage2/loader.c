#include <stdint.h>
#include <stdbool.h>
#include "dev/vga.h"
#include "cpu/interrupts/idt.h"
#include "cpu/interrupts/isr.h"
#include "cpu/interrupts/irq.h"
#include "cpu/pic/pic.h"
#include "cpu/pit/pit.h"
#include "dev/keyboard.h"
#include "utils.h"
#include "dev/serial.h"

extern void enable_fpu();

float cosf(float x) {
  float result = 1.0f;
  float term = 1.0f;
  for (int n = 1; n < 10; n++) {
    term *= -x * x / (2 * n * (2 * n - 1));
    result += term;
  }
  return result;
}

float sinf(float x) {
  float result = x;
  float term = x;
  for (int n = 1; n < 10; n++) {
    term *= -x * x / (2 * n * (2 * n + 1));
    result += term;
  }
  return result;
}

typedef struct {
  float x, y, z;
} Vec3;

typedef struct {
  int x, y;
} Vec2;

#define CUBE_SIZE 40
Vec3 cube_vertices[8] = {
  {-1, -1, -1}, {1, -1, -1},
  {1,  1, -1}, {-1,  1, -1},
  {-1, -1,  1}, {1, -1,  1},
  {1,  1,  1}, {-1,  1,  1}
};

int cube_edges[12][2] = {
  {0,1}, {1,2}, {2,3}, {3,0},
  {4,5}, {5,6}, {6,7}, {7,4},
  {0,4}, {1,5}, {2,6}, {3,7}
};

float angle_x = 0.0f;
float angle_y = 0.0f;
float angle_z = 0.0f;

#define CX (SCREEN_WIDTH / 2)
#define CY (SCREEN_HEIGHT / 2)

Vec2 project(Vec3 v) {
  float scale = CUBE_SIZE;
  float dist = 3.0f;
  float z = v.z + dist;
  float px = (v.x / z) * scale + CX;
  float py = (v.y / z) * scale + CY;
  return (Vec2){ (int)px, (int)py };
}

void draw_cube() {
  for (int x = 0; x < SCREEN_WIDTH; x++) {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
      put_pixel(x, y, 0);
    }
  }

  if (angle_x > 6.283185f) angle_x -= 6.283185f;
  if (angle_y > 6.283185f) angle_y -= 6.283185f;
  if (angle_z > 6.283185f) angle_z -= 6.283185f;

  Vec2 projected[8];
  for (int i = 0; i < 8; i++) {
    Vec3 v = cube_vertices[i];

    float y = v.y * cosf(angle_x) - v.z * sinf(angle_x);
    float z = v.y * sinf(angle_x) + v.z * cosf(angle_x);
    v.y = y; v.z = z;

    float x = v.x * cosf(angle_y) + v.z * sinf(angle_y);
    z = -v.x * sinf(angle_y) + v.z * cosf(angle_y);
    v.x = x; v.z = z;

    x = v.x * cosf(angle_z) - v.y * sinf(angle_z);
    y = v.x * sinf(angle_z) + v.y * cosf(angle_z);
    v.x = x; v.y = y;

    projected[i] = project(v);
  }

  for (int i = 0; i < 12; i++) {
    Vec2 p1 = projected[cube_edges[i][0]];
    Vec2 p2 = projected[cube_edges[i][1]];
    draw_line(p1.x, p1.y, p2.x, p2.y, 15);
  }

  angle_x += 0.02f;
  angle_y += 0.03f;
  angle_z += 0.015f;
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void pit_handler(registers_t *r) {
  draw_cube();
}

typedef struct __attribute__((packed)) {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi_extended_attributes;
} e820_entry_t;

void check_overlaps(uint16_t count, e820_entry_t* entries) {
  int overlap_count = 0;
  for (uint16_t i = 0; i < count; i++) {
    e820_entry_t* a = &entries[i];
    uint64_t a_start = a->base;
    uint64_t a_end = a->base + a->length;

    for (uint16_t j = i + 1; j < count; j++) {
      e820_entry_t* b = &entries[j];
      uint64_t b_start = b->base;
      uint64_t b_end = b->base + b->length;

      if ((a_start < b_end) && (b_start < a_end)) {
        overlap_count++;
        serial_print("Overlap detected between entries ");
        serial_print(itoa(i));
        serial_print(" and ");
        serial_print(itoa(j));
        serial_print("\n");
      }
    }
  }

  if (overlap_count == 0) {
    serial_print("No overlaps detected in memory map.\n");
  } else {
    serial_print(itoa(overlap_count));
    serial_print(" overlaps detected in memory map.\n");
  }
}

void dump_mmap(uint16_t count, e820_entry_t* entries) {
  for (uint16_t i = 0; i < count; i++) {
    e820_entry_t* entry = &entries[i];
    serial_print("Base: ");
    serial_print(hextoa((int)(entry->base >> 32)));
    serial_print(hextoa((int)(entry->base & 0xFFFFFFFF)));
    serial_print(", Length: ");
    serial_print(hextoa((int)(entry->length >> 32)));
    serial_print(hextoa((int)(entry->length & 0xFFFFFFFF)));
    serial_print(", Type: ");
    serial_print(itoa(entry->type));
    serial_print("\n");
  }
}

void loader_start() {
  for (int i = 0; i < IRQs; i++) {
    pic_set_mask(i);
  }

  pic_remap();
  idt_init();
  install_exception_isrs();
  pit_init();
  install_irq(0, pit_handler);
  pic_clear_mask(0);
  enable_fpu();
  install_keyboard();
  asm("sti");

  for (int x = 0; x < SCREEN_WIDTH; x++) {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
      put_pixel(x, y, 0);
    }
  }

  e820_entry_t* mem_map = (e820_entry_t*)0x9000;
  uint16_t entry_count = (*(uint16_t*)0x8E00);

  dump_mmap(entry_count, mem_map);
  check_overlaps(entry_count, mem_map);

  while (1);
}
