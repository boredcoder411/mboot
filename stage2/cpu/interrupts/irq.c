// Dispatch PIC hardware interrupts, acknowledge them and let the timer select a saved task frame.
#include "cpu/interrupts/irq.h"
#include "cpu/interrupts/idt.h"
#include "cpu/interrupts/isr.h"
#include "cpu/pic/pic.h"
#include "dev/serial.h"
#include "io.h"
#include "scheduler.h"
#include "utils.h"

void (*irq_handlers[IRQs])(registers_t *regs) = {0};

uint32_t irq_dispatcher(registers_t *r) {
  /* Assembly pushed the remapped vector (0x20..0x2f); subtract the exception
     range to recover the physical IRQ number used to index handlers. */
  size_t irq = (size_t)(r->int_no - EXCEPTION_ISRS);

  if (irq < IRQs && irq_handlers[irq]) {
    irq_handlers[irq](r);
  }
  /* Acknowledge the PIC before a timer switch changes the active task. */
  pic_send_eoi(irq);

  if (irq == 0) {
    /* The returned frame pointer can belong to a different runnable task. */
    return scheduler_tick(r);
  }

  return (uint32_t)r;
}

void install_irq(size_t n, void (*handler)(registers_t *regs)) {
  /* Register a C callback and route the vector through its assembly stub.
     A NULL callback is useful for IRQ0, whose scheduling is handled above. */
  irq_handlers[n] = handler;
  idt_set_gate(n + EXCEPTION_ISRS, (unsigned)irq_stub_table[n], 0x08, 0x8E);
}
