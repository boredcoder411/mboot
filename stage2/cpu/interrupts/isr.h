#pragma once

#include <stddef.h>
#include "io.h"

#define EXCEPTION_ISRS 32
#define IRQs 16

extern void *isr_stub_table[EXCEPTION_ISRS];

void install_exception_isrs();

extern void *irq_stub_table[IRQs];
extern void (*irq_handlers[IRQs])(registers_t *regs);

void install_irq(size_t n, void (*handler)(registers_t* regs));
