#pragma once

#include <stddef.h>
#include <stdint.h>

#define TTY_COUNT 4

void tty_init(void);
int tty_valid(int tty);
int tty_current(void);
void tty_switch(int tty);
void tty_write(int tty, char c);
int tty_write_buf(int tty, const char *buf, size_t size);
int tty_read(int tty, size_t size, void *dst);
void tty_handle_key(uint8_t scancode, char ascii);
