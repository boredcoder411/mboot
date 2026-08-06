#pragma once

#define MAX_FDS_PER_PROCESS 8

typedef struct {
  /* index i holds the global file handle for user fd i + 3, or -1 if unused */
  int handle[MAX_FDS_PER_PROCESS];
} fd_table_t;
