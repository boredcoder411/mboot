#pragma once

#ifndef NDEBUG
#define assert(expr) ((expr) ? (void)0 : __assert_fail(#expr, __FILE__, __LINE__))
void __assert_fail(const char *expr, const char *file, int line);
#else
#define assert(expr) ((void)0)
#endif
