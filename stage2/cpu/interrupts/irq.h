#pragma once

#include <stddef.h>

#define IRQs 16

extern void *irq_stub_table[IRQs];

void install_irq(size_t n);