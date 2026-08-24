#include "dev/tty.h"
#include "dev/vga.h"
#include "io.h"
#include "mem.h"
#include "scheduler.h"
#include "utils.h"

#define TTY_INPUT_SIZE 256
#define TTY_LINE_SIZE 256
#define TTY_CELL_COUNT (VGA_WIDTH * VGA_HEIGHT)

typedef struct {
  uint16_t cells[TTY_CELL_COUNT];
  int cur_x;
  int cur_y;
  char input[TTY_INPUT_SIZE];
  uint16_t input_head;
  uint16_t input_tail;
  char line[TTY_LINE_SIZE];
  int line_fill;
  int line_pos;
} tty_t;

static tty_t *ttys[TTY_COUNT];
static int active_tty;
static int tty_ready;

static uint16_t *vga_buffer(void) { return (uint16_t *)VGA_BUF; }

static void tty_clear(tty_t *tty) {
  uint16_t blank = (uint16_t)' ' | VGA_WHITE_ON_BLACK;
  for (int i = 0; i < TTY_CELL_COUNT; i++)
    tty->cells[i] = blank;
  tty->cur_x = 0;
  tty->cur_y = 0;
  tty->input_head = 0;
  tty->input_tail = 0;
  tty->line_fill = 0;
  tty->line_pos = 0;
}

static void tty_render(int tty_id) {
  tty_t *tty = ttys[tty_id];
  if (!tty)
    return;
  memcpy(vga_buffer(), tty->cells, sizeof(tty->cells));
  update_cursor(tty->cur_x, tty->cur_y);
}

static void tty_put_cell(tty_t *tty, int x, int y, char c) {
  tty->cells[y * VGA_WIDTH + x] = (uint16_t)c | VGA_WHITE_ON_BLACK;
  if (tty == ttys[active_tty])
    vga_buffer()[y * VGA_WIDTH + x] = (uint16_t)c | VGA_WHITE_ON_BLACK;
}

static void tty_scroll(tty_t *tty) {
  for (int y = 1; y < VGA_HEIGHT; y++) {
    memcpy(&tty->cells[(y - 1) * VGA_WIDTH], &tty->cells[y * VGA_WIDTH],
           VGA_WIDTH * sizeof(uint16_t));
  }

  uint16_t blank = (uint16_t)' ' | VGA_WHITE_ON_BLACK;
  for (int x = 0; x < VGA_WIDTH; x++)
    tty->cells[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = blank;

  tty->cur_y = VGA_HEIGHT - 1;
  if (tty == ttys[active_tty])
    tty_render(active_tty);
}

static void tty_push_input(tty_t *tty, char c) {
  uint16_t next_head = (tty->input_head + 1) % TTY_INPUT_SIZE;
  if (next_head == tty->input_tail)
    return;
  tty->input[tty->input_head] = c;
  tty->input_head = next_head;
}

static int tty_has_input(tty_t *tty) {
  return tty->input_head != tty->input_tail;
}

static char tty_pop_input(tty_t *tty) {
  for (;;) {
    CLI();
    if (tty_has_input(tty)) {
      STI();
      break;
    }
    scheduler_block(tty);
  }

  char c = tty->input[tty->input_tail];
  tty->input_tail = (tty->input_tail + 1) % TTY_INPUT_SIZE;
  return c;
}

int tty_valid(int tty) { return tty >= 0 && tty < TTY_COUNT; }

int tty_current(void) { return active_tty; }

void tty_init(void) {
  for (int i = 0; i < TTY_COUNT; i++) {
    ttys[i] = (tty_t *)kmalloc(sizeof(tty_t));
    if (!ttys[i])
      return;
    tty_clear(ttys[i]);
  }
  active_tty = 0;
  tty_ready = 1;
  tty_render(active_tty);
}

void tty_switch(int tty) {
  if (!tty_valid(tty) || tty == active_tty)
    return;
  active_tty = tty;
  tty_render(active_tty);
}

void tty_write(int tty_id, char c) {
  if (!tty_ready || !tty_valid(tty_id) || !ttys[tty_id]) {
    vga_write(c);
    return;
  }

  tty_t *tty = ttys[tty_id];

  switch (c) {
  case '\n':
    tty->cur_x = 0;
    tty->cur_y++;
    break;
  case '\r':
    tty->cur_x = 0;
    break;
  case '\b':
    if (tty->cur_x > 0) {
      tty->cur_x--;
      tty_put_cell(tty, tty->cur_x, tty->cur_y, ' ');
    }
    break;
  case '\t':
    tty->cur_x = (tty->cur_x + 8) & ~(8 - 1);
    break;
  default:
    tty_put_cell(tty, tty->cur_x, tty->cur_y, c);
    tty->cur_x++;
    break;
  }

  if (tty->cur_x >= VGA_WIDTH) {
    tty->cur_x = 0;
    tty->cur_y++;
  }

  if (tty->cur_y >= VGA_HEIGHT)
    tty_scroll(tty);

  if (tty_id == active_tty)
    update_cursor(tty->cur_x, tty->cur_y);
}

int tty_write_buf(int tty, const char *buf, size_t size) {
  if (!buf || !tty_valid(tty))
    return -1;

  for (size_t i = 0; i < size; i++)
    tty_write(tty, buf[i]);

  return (int)size;
}

int tty_read(int tty_id, size_t size, void *dst) {
  if (!dst || !tty_valid(tty_id))
    return -1;

  tty_t *tty = ttys[tty_id];
  if (!tty)
    return -1;

  if (tty->line_pos < tty->line_fill) {
    size_t take = size;
    if ((size_t)(tty->line_fill - tty->line_pos) < take)
      take = (size_t)(tty->line_fill - tty->line_pos);
    char *out = dst;
    for (size_t i = 0; i < take; i++)
      out[i] = tty->line[tty->line_pos++];
    return (int)take;
  }

  tty->line_fill = 0;
  tty->line_pos = 0;
  for (;;) {
    char c = tty_pop_input(tty);
    if (c == '\r')
      c = '\n';
    if (c == '\b' || c == 0x7F) {
      if (tty->line_fill > 0) {
        tty->line_fill--;
        tty_write(tty_id, '\b');
        tty_write(tty_id, ' ');
        tty_write(tty_id, '\b');
      }
    } else if (c == '\n') {
      if (tty->line_fill < TTY_LINE_SIZE)
        tty->line[tty->line_fill++] = '\n';
      tty_write(tty_id, '\r');
      tty_write(tty_id, '\n');
      break;
    } else if (c >= ' ' && c < 0x7F) {
      if (tty->line_fill < TTY_LINE_SIZE - 1) {
        tty->line[tty->line_fill++] = c;
        tty_write(tty_id, c);
      }
    }
  }

  return tty_read(tty_id, size, dst);
}

void tty_handle_key(uint8_t scancode, char ascii) {
  if (!tty_ready)
    return;

  if (scancode >= 0x3B && scancode <= 0x3E) {
    tty_switch(scancode - 0x3B);
    return;
  }

  if (ascii) {
    tty_push_input(ttys[active_tty], ascii);
    scheduler_wake(ttys[active_tty]);
  }
}
