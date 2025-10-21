#include "cpu/pic/pic.h"
#include "io.h"
#include <stdbool.h>

void pic_clear_mask(size_t i) {
  uint16_t port = (i < 8) ? PIC1_DATA : PIC2_DATA;
  uint8_t bit = (uint8_t)(1 << (i < 8 ? i : (i - 8)));
  uint8_t value = inb(port) & ~bit;
  outb(port, value);
}

void pic_set_mask(size_t i) {
  uint16_t port = (i < 8) ? PIC1_DATA : PIC2_DATA;
  uint8_t bit = (uint8_t)(1 << (i < 8 ? i : (i - 8)));
  uint8_t value = inb(port) | bit;
  outb(port, value);
}

bool pic_check_mask(size_t i) {
  uint16_t port = (i < 8) ? PIC1_DATA : PIC2_DATA;
  uint8_t bit = (uint8_t)(1 << (i < 8 ? i : (i - 8)));
  uint8_t mask = inb(port);
  return (mask & bit) != 0;
}

void pic_remap() {
  uint8_t mask1 = inb(PIC1_DATA);
  uint8_t mask2 = inb(PIC2_DATA);

  outb(PIC1, ICW1_INIT | ICW1_ICW4);
  outb(PIC2, ICW1_INIT | ICW1_ICW4);

  outb(PIC1_DATA, PIC1_OFFSET); // ICW2: Master vector offset
  outb(PIC2_DATA, PIC2_OFFSET); // ICW2: Slave vector offset

  outb(PIC1_DATA,
       0x04); // ICW3: tell Master that there is a slave at IRQ2 (0000 0100)
  outb(PIC2_DATA, 0x02); // ICW3: tell Slave its cascade identity (0000 0010)

  outb(PIC1_DATA, PIC_MODE_8086); // ICW4: 8086/88 (MCS-80/85) mode
  outb(PIC2_DATA, PIC_MODE_8086);

  outb(PIC1_DATA, mask1); // restore saved masks
  outb(PIC2_DATA, mask2);
}

void pic_send_eoi(size_t i) {
  if (i >= 8) {
    outb(PIC2, PIC_EOI);
  }
  outb(PIC1, PIC_EOI);
}
