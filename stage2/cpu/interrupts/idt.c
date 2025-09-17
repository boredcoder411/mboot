#include <stdint.h>
#include "serial.h"
#include "utils.h"
#include "io.h"

typedef struct {
    uint16_t base_low;   // Lower 16 bits of handler function address
    uint16_t sel;        // Kernel segment selector
    uint8_t  always0;    // Always zero
    uint8_t  flags;      // Flags (present, DPL, type)
    uint16_t base_high;  // Upper 16 bits of handler function address
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

#define IDT_ENTRIES 256
idt_entry_t idt[IDT_ENTRIES];
idt_ptr_t idt_ptr;

extern void _isr0();
extern void _isr1();
extern void _isr2();
extern void _isr3();
extern void _isr4();
extern void _isr5();
extern void _isr6();
extern void _isr7();
extern void _isr8();
extern void _isr9();
extern void _isr10();
extern void _isr11();
extern void _isr12();
extern void _isr13();
extern void _isr14();

static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low  = base & 0xFFFF;
    idt[num].sel       = sel;
    idt[num].always0   = 0;
    idt[num].flags     = flags;
    idt[num].base_high = (base >> 16) & 0xFFFF;
}

extern void idt_load(uint32_t); // asm routine to lidt idt_ptr

void idt_init() {
    idt_ptr.limit = sizeof(idt_entry_t) * IDT_ENTRIES - 1;
    idt_ptr.base  = (uint32_t)&idt;

    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    idt_set_gate(0,  (uint32_t)_isr0,  0x08, 0x8E);
    idt_set_gate(1,  (uint32_t)_isr1,  0x08, 0x8E);
    idt_set_gate(2,  (uint32_t)_isr2,  0x08, 0x8E);
    idt_set_gate(3,  (uint32_t)_isr3,  0x08, 0x8E);
    idt_set_gate(4,  (uint32_t)_isr4,  0x08, 0x8E);
    idt_set_gate(5,  (uint32_t)_isr5,  0x08, 0x8E);
    idt_set_gate(6,  (uint32_t)_isr6,  0x08, 0x8E);
    idt_set_gate(7,  (uint32_t)_isr7,  0x08, 0x8E);
    idt_set_gate(8,  (uint32_t)_isr8,  0x08, 0x8E);
    idt_set_gate(9,  (uint32_t)_isr9,  0x08, 0x8E);
    idt_set_gate(10, (uint32_t)_isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint32_t)_isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)_isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)_isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)_isr14, 0x08, 0x8E);

    idt_load((uint32_t)&idt_ptr);
}


typedef struct {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} registers_t;

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

    serial_print(" Error=");
    serial_print(itoa(r->err_code));
    serial_print("\n");
}

