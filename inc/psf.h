#pragma once

#include <stdint.h>

#define NUM_GLYPHS 256
#define PSF1_FONT_MAGIC 0x0436

typedef struct {
  uint16_t magic;
  uint8_t fontMode;
  uint8_t characterSize;
} psf_header_t;
