#include "isr.h"
#include "idt.h"
#include "io.h"
#include "dev/serial.h"
#include "utils.h"

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
    serial_print(itoa(r->int_no));
    serial_print("\n");

    serial_print(" CS=");
    serial_print(itoa(r->cs));
    serial_print("\n");

    serial_print(" EIP=");
    serial_print(itoa(r->eip));
    serial_print("\n");

    serial_print(" Error code=");
    serial_print(itoa(r->err_code));
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
