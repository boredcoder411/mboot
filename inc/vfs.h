#pragma once

#include "fd.h"
#include <stddef.h>

typedef struct {
  int (*open_file)(const char *path);
  int (*close_file)(int handle);
  int (*read_file)(int handle, void *buffer, size_t size);
  int (*dup_file)(int handle);
} vfs_driver_t;

void register_vfs_driver(vfs_driver_t *drv);

/* file-handle level (kernel internal, no fd table involved) */
int file_open(const char *path);
int file_close(int handle);
int file_read(int handle, size_t size, void *dst);

/* per-process fd level */
int vfs_open(const char *path);
int vfs_read(int fd, size_t size, void *dst);
int vfs_close(int fd);
void vfs_fork_fds(fd_table_t *child);
void vfs_exit_fds(fd_table_t *table);
