#pragma once

#include <stddef.h>
#include "io.h"

#define IRQs 16

extern void *irq_stub_table[IRQs];
extern void (*irq_handlers[IRQs])(registers_t *regs);

void install_irq(size_t n, void (*handler)(registers_t* regs));
