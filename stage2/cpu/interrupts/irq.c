#include "cpu/interrupts/irq.h"
#include "cpu/interrupts/idt.h"
#include "cpu/interrupts/isr.h"
#include "cpu/pic/pic.h"
#include "dev/serial.h"
#include "io.h"
#include "utils.h"

void (*irq_handlers[IRQs])(registers_t *regs) = {0};

void irq_dispatcher(registers_t *r) {
  CLI()
  serial_printf("irq: %i\n", r->int_no);
  irq_handlers[r->int_no - EXCEPTION_ISRS](r);
  pic_send_eoi(r->int_no);
  STI()
}

void install_irq(size_t n, void (*handler)(registers_t *regs)) {
  irq_handlers[n] = handler;
  idt_set_gate(n + EXCEPTION_ISRS, (unsigned)irq_stub_table[n], 0x08, 0x8E);
}
