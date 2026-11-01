// Per-task descriptor table mapping local descriptors to VFS handles.
#pragma once

#define MAX_FDS_PER_PROCESS 16

typedef struct {
  /* index i holds the global file handle for user fd i, or -1 if unused */
  int handle[MAX_FDS_PER_PROCESS];
} fd_table_t;
