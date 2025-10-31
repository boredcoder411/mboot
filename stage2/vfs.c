#include "vfs.h"
#include "dev/serial.h"

static vfs_driver_t *current_driver = NULL;

void register_vfs_driver(vfs_driver_t *drv) { current_driver = drv; }

int read_file(const char *path) {
  if (!current_driver || !current_driver->open_file ||
      !current_driver->read_file || !current_driver->close_file) {
    ERROR("VFS", "No valid VFS driver registered.");
    return -1;
  }

  int fd = current_driver->open_file(path);
  if (fd < 0) {
    ERROR("VFS", "Failed to open file '%s'", path);
    return -1;
  }

  char buffer[256];
  int bytes = current_driver->read_file(fd, buffer, sizeof(buffer));

  current_driver->close_file(fd);

  INFO("VFS", "Read %d bytes from %s: '%.20s'", bytes, path, buffer);

  return bytes;
}
