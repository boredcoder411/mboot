#include "cpu/pic/pic.h"
#include "io.h"

void pic_mask_all() {
  outb(PIC1_DATA, 0xFF);
  outb(PIC2_DATA, 0xFF);
}

void pic_clear_mask(size_t i) {
  uint16_t port = i < 8 ? PIC1_DATA : PIC2_DATA;
  uint8_t value = inb(port) & ~(1 << i);
    outb(port, value);
}

void pic_remap() {
  uint8_t mask1 = inb(PIC1_DATA);
  uint8_t mask2 = inb(PIC2_DATA);
  outb(PIC1, ICW1_INIT | ICW1_ICW4);
  outb(PIC2, ICW1_INIT | ICW1_ICW4);
  outb(PIC1_DATA, PIC1_OFFSET);
  outb(PIC2_DATA, PIC2_OFFSET);
  outb(PIC1_DATA, 0x04);
  outb(PIC2_DATA, 0x02);
  outb(PIC1_DATA, PIC_MODE_8086);
  outb(PIC1_DATA, mask1);
  outb(PIC2_DATA, mask2);
}
