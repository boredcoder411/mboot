#include "irq.h"
#include "idt.h"
#include "isr.h"
#include "io.h"
#include "utils.h"
#include "serial.h"
#include "cpu/pic/pic.h"

void irq_handler(registers_t* r) {
    serial_print("Interrupt: ");
    serial_print(itoa(r->int_no));
    serial_print("\n");
    pic_send_eoi(r->int_no);
}

void install_irq(size_t n) {
  idt_set_gate(n + EXCEPTION_ISRS, (unsigned)irq_stub_table[n], 0x08, 0x8E);
}