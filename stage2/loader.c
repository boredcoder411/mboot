#include "cpu/interrupts/idt.h"
#include "cpu/interrupts/irq.h"
#include "cpu/interrupts/isr.h"
#include "cpu/pic/pic.h"
#include "cpu/pit/pit.h"
#include "dev/disk.h"
#include "dev/keyboard.h"
#include "dev/pci.h"
#include "dev/rtc.h"
#include "dev/serial.h"
#include "dev/vga.h"
#include "fs.h"
#include "mbr.h"
#include "mem.h"
#include "psf.h"
#include "utils.h"
#include <stdbool.h>
#include <stdint.h>

extern void enable_fpu();

#ifdef CUBE_DEMO
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
  float x, y;
} Vec2;

#define CUBE_SIZE 40
Vec3 cube_vertices[8] = {{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
                         {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1}};

int cube_edges[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
                         {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};

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
  return (Vec2){(int)px, (int)py};
}

void draw_cube() {
  if (angle_x > 6.283185f)
    angle_x -= 6.283185f;
  if (angle_y > 6.283185f)
    angle_y -= 6.283185f;
  if (angle_z > 6.283185f)
    angle_z -= 6.283185f;

  Vec2 projected[8];
  for (int i = 0; i < 8; i++) {
    Vec3 v = cube_vertices[i];

    float y = v.y * cosf(angle_x) - v.z * sinf(angle_x);
    float z = v.y * sinf(angle_x) + v.z * cosf(angle_x);
    v.y = y;
    v.z = z;

    float x = v.x * cosf(angle_y) + v.z * sinf(angle_y);
    z = -v.x * sinf(angle_y) + v.z * cosf(angle_y);
    v.x = x;
    v.z = z;

    x = v.x * cosf(angle_z) - v.y * sinf(angle_z);
    y = v.x * sinf(angle_z) + v.y * cosf(angle_z);
    v.x = x;
    v.y = y;

    projected[i] = project(v);
  }

  for (int i = 0; i < 12; i++) {
    Vec2 p1 = projected[cube_edges[i][0]];
    Vec2 p2 = projected[cube_edges[i][1]];
    draw_line(p1.x, p1.y, p2.x, p2.y, VGA_GREEN);
  }

  angle_x += 0.02f;
  angle_y += 0.03f;
  angle_z += 0.015f;
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
void pit_handler(registers_t *r) {
  clear_screen();
  draw_cube();
  char *time = fetch_rtc();
  display_string(time, VGA_WHITE);
}
#endif
void loader_start() {
  for (int i = 0; i < IRQs; i++) {
    pic_set_mask(i);
  }

  pic_remap();
  idt_init();
  install_exception_isrs();
  pit_init();
  enable_fpu();
  install_keyboard();
#ifdef CUBE_DEMO
  install_irq(0, pit_handler);
  pic_clear_mask(0);
#endif
  e820_entry_t *mem_map = (e820_entry_t *)0x9000;
  uint16_t entry_count = (*(uint16_t *)0x8E00);

  init_alloc(entry_count, mem_map);

  mbr_t *mbr = (mbr_t *)0x7C00;
  uint32_t found = 0;
  for (uint32_t i = 0; i < 4; i++) {
    if (mbr->partitions[i].type == 0xef) {
      found = i;
      break;
    }
  }

  if (found == 4) {
    serial_printf("couldn't find second partition");
    HALT()
  }

  wad_header_t *wad = kmalloc(4096);
  ata_lba_read(mbr->partitions[found].first_lba, 4, wad, 0);

  psf_header_t *psf = find_file("font.psf", wad);
  if (psf->magic != PSF1_FONT_MAGIC) {
    serial_printf("invalid psf file\n");
    HALT()
  }
  uint8_t *glyphs = (uint8_t *)(psf + 1);
  vga_init(glyphs);

  uint8_t *welcome = find_file("test.txt", wad);
  display_string((char *)welcome, VGA_WHITE);

  imf_t *imf_file = find_file("icon.imf", wad);
  display_imf(imf_file, 0, 16);

  pci_enumerate();

  STI()

  while (1) {
  }
}
