#pragma once

#include <stdint.h>

#define MAX_X 320
#define MAX_Y 200

typedef struct {
  uint8_t x;
  uint8_t y;
  uint8_t colors[];
} imf_t;
