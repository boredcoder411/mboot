#include "cpu/interrupts/isr.h"
#include "cpu/interrupts/idt.h"
#include "dev/serial.h"
#include "io.h"
#include "utils.h"

char *exceptions[32] = {"Divide by zero",
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
                        "RESERVED"};

void isr_handler(registers_t *r) {
  serial_printf("exception: %i\n", r->int_no);

  serial_printf(" CS=%i\n", r->cs);

  serial_printf(" EIP=%i\n", r->eip);

  serial_printf(" Error code=%i\n", r->err_code);

  serial_printf(" Exception message: %s\n", exceptions[r->int_no]);

  HALT()
}

void install_exception_isrs() {
  for (int i = 0; i < EXCEPTION_ISRS; i++) {
    idt_set_gate(i, (unsigned)isr_stub_table[i], 0x08, 0x8E);
  }
}
