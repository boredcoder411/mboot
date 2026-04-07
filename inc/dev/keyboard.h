#pragma once

#include "io.h"
#include <stdint.h>

#define KEYBOARD_BUFFER_SIZE 256

void keyboard_handler(registers_t *r);
void install_keyboard();
uint8_t keyboard_read_key();
int keyboard_has_key();
