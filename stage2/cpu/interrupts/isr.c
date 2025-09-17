#include "isr.h"
#include "idt.h"
#include "io.h"
#include "serial.h"
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
    "RESERVED"
};

void isr_handler(registers_t* r) {
    serial_print("Interrupt: ");
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
  idt_set_gate(0, (uint32_t)_isr0, 0x08, 0x8E);
  idt_set_gate(1, (uint32_t)_isr1, 0x08, 0x8E);
  idt_set_gate(2, (uint32_t)_isr2, 0x08, 0x8E);
  idt_set_gate(3, (uint32_t)_isr3, 0x08, 0x8E);
  idt_set_gate(4, (uint32_t)_isr4, 0x08, 0x8E);
  idt_set_gate(5, (uint32_t)_isr5, 0x08, 0x8E);
  idt_set_gate(6, (uint32_t)_isr6, 0x08, 0x8E);
  idt_set_gate(7, (uint32_t)_isr7, 0x08, 0x8E);
  idt_set_gate(8, (uint32_t)_isr8, 0x08, 0x8E);
  idt_set_gate(9, (uint32_t)_isr9, 0x08, 0x8E);
  idt_set_gate(10, (uint32_t)_isr10, 0x08, 0x8E);
  idt_set_gate(11, (uint32_t)_isr11, 0x08, 0x8E);
  idt_set_gate(12, (uint32_t)_isr12, 0x08, 0x8E);
  idt_set_gate(13, (uint32_t)_isr13, 0x08, 0x8E);
  idt_set_gate(14, (uint32_t)_isr14, 0x08, 0x8E);
}
