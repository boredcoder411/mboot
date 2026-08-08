#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <locale.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
#include <limits.h>
#include <float.h>
#include <assert.h>

typedef long ssize_t;

int open(const char *pathname, int flags);
int fork(void);
int execv(const char *pathname, char *const argv[]);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int close(int fd);
