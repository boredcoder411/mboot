#pragma once

#include <stddef.h>

#define PIC1 0x20
#define PIC1_OFFSET 0x20
#define PIC1_DATA (PIC1 + 1)

#define PIC2 0xA0
#define PIC2_OFFSET 0x28
#define PIC2_DATA (PIC2 + 1)

#define PIC_EOI 0x20
#define PIC_MODE_8086 0x01
#define ICW1_ICW4 0x01
#define ICW1_INIT 0x10

void pic_clear_mask(size_t i);
void pic_set_mask(size_t i);
void pic_remap();
void pic_send_eoi(size_t i);