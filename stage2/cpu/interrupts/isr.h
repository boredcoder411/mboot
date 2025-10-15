#pragma once

#define EXCEPTION_ISRS 32

extern void *isr_stub_table[EXCEPTION_ISRS];

void install_exception_isrs();
