#include "irq.h"
#include "idt.h"
#include "isr.h"
#include "io.h"
#include "utils.h"
#include "serial.h"
#include "cpu/pic/pic.h"

void (*irq_handlers[IRQs])(registers_t *regs) = {0};

void irq_dispatcher(registers_t* r) {
  serial_print("irq: ");
  serial_print(itoa(r->int_no));
  serial_print("\n");
  irq_handlers[r->int_no - EXCEPTION_ISRS](r);
  pic_send_eoi(r->int_no);
}

void install_irq(size_t n, void (*handler)(registers_t *regs)) {
  irq_handlers[n] = handler;
  idt_set_gate(n + EXCEPTION_ISRS, (unsigned)irq_stub_table[n], 0x08, 0x8E);
}
