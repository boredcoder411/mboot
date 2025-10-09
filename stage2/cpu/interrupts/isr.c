#include "isr.h"
#include "idt.h"
#include "io.h"
#include "dev/serial.h"
#include "utils.h"
#include "cpu/pic/pic.h"

void (*irq_handlers[IRQs])(registers_t *regs) = {0};

char *exceptions[32] = {
    "Divide by zero",
    "Debug",
    "NMI",
    "Breakpoint",
    "Overflow",
    "OOB",
    "Invalid opcode",
    "No coprocessor",
    "Double fault",
    "Coprocessor segment overrun",
    "Bad TSS",
    "Segment not present",
    "Stack fault",
    "General protection fault",
    "Page fault",
    "Unrecognized interrupt",
    "Coprocessor fault",
    "Alignment check",
    "Machine check",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED"
};

void isr_handler(registers_t* r) {
    serial_print("exception: ");
    serial_print(int_to_str(r->int_no));
    serial_print("\n");

    serial_print(" CS=");
    serial_print(int_to_str(r->cs));
    serial_print("\n");

    serial_print(" EIP=");
    serial_print(int_to_str(r->eip));
    serial_print("\n");

    serial_print(" Error code=");
    serial_print(int_to_str(r->err_code));
    serial_print("\n");

    serial_print(" Exception message: ");
    serial_print(exceptions[r->int_no]);
    serial_print("\n");

    asm("cli");
    asm("hlt");
}

void install_exception_isrs() {
    for(int i = 0; i < EXCEPTION_ISRS; i++) {
        idt_set_gate(i, (unsigned)isr_stub_table[i], 0x08, 0x8E);
    }
}

void irq_dispatcher(registers_t* r) {
  asm("cli");
  serial_print("irq: ");
  serial_print(int_to_str(r->int_no));
  serial_print("\n");
  irq_handlers[r->int_no - EXCEPTION_ISRS](r);
  pic_send_eoi(r->int_no);
  asm("sti");
}

void install_irq(size_t n, void (*handler)(registers_t *regs)) {
  irq_handlers[n] = handler;
  idt_set_gate(n + EXCEPTION_ISRS, (unsigned)irq_stub_table[n], 0x08, 0x8E);
}
