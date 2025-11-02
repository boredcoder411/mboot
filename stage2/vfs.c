#include "vfs.h"
#include "dev/serial.h"

static vfs_driver_t *current_driver = NULL;

void register_vfs_driver(vfs_driver_t *drv) { current_driver = drv; }

int open_file(const char *path) {
  if (!current_driver || !current_driver->open_file) {
    ERROR("VFS", "No valid VFS driver registered");
    return -1;
  }

  return current_driver->open_file(path);
}

int close_file(int fd) {
  if (!current_driver || !current_driver->close_file) {
    ERROR("VFS", "No valid VFS driver registered");
    return -1;
  }

  return current_driver->close_file(fd);
}

int read_file(int fd, size_t size, void *dst) {
  if (!current_driver || !current_driver->read_file) {
    ERROR("VFS", "No valid VFS driver registered");
    return -1;
  }

  return current_driver->read_file(fd, dst, size);
}
