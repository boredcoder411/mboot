#pragma once

#include <stddef.h>

typedef struct {
  int (*open_file)(const char *path);
  int (*close_file)(int fd);
  int (*read_file)(int fd, void *buffer, size_t size);
} vfs_driver_t;

void register_vfs_driver(vfs_driver_t *drv);
int open_file(const char *path);
int close_file(int fd);
int read_file(int fd, size_t size, void *dst);
