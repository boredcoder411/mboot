// Saved execution context and non-local jump declarations for i386.
#pragma once

typedef signed long jmp_buf[6];

int setjmp(jmp_buf env) __attribute__((returns_twice));
void longjmp(jmp_buf env, int val) __attribute__((noreturn));
