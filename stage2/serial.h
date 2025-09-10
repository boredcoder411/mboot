#pragma once

#include <stdint.h>

int init_serial();
static int is_transmit_empty();
static void write_serial(char a);
void serial_print(char* str);