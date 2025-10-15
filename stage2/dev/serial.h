#pragma once

#include <stdint.h>

int init_serial();
int is_transmit_empty();
void write_serial(char a);
void serial_print(char *str);