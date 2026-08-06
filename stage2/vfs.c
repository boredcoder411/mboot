#include "vfs.h"
#include "dev/serial.h"
#include "scheduler.h"

static vfs_driver_t *current_driver = NULL;

void register_vfs_driver(vfs_driver_t *drv) { current_driver = drv; }

int file_open(const char *path) {
  if (!current_driver || !current_driver->open_file) {
    ERROR("VFS", "No valid VFS driver registered");
    return -1;
  }

  return current_driver->open_file(path);
}

int file_close(int handle) {
  if (!current_driver || !current_driver->close_file) {
    ERROR("VFS", "No valid VFS driver registered");
    return -1;
  }

  return current_driver->close_file(handle);
}

int file_read(int handle, size_t size, void *dst) {
  if (!current_driver || !current_driver->read_file) {
    ERROR("VFS", "No valid VFS driver registered");
    return -1;
  }

  return current_driver->read_file(handle, dst, size);
}

int vfs_open(const char *path) {
  int handle = file_open(path);
  if (handle < 0)
    return -1;

  fd_table_t *table = scheduler_current_fds();
  if (!table) {
    file_close(handle);
    return -1;
  }

  for (int i = 0; i < MAX_FDS_PER_PROCESS; i++) {
    if (table->handle[i] < 0) {
      table->handle[i] = handle;
      return i + 3;
    }
  }

  ERROR("VFS", "process fd table full");
  file_close(handle);
  return -1;
}

int vfs_read(int fd, size_t size, void *dst) {
  if (fd < 3)
    return -1;

  fd_table_t *table = scheduler_current_fds();
  int idx = fd - 3;
  if (!table || idx >= MAX_FDS_PER_PROCESS || table->handle[idx] < 0)
    return -1;

  return file_read(table->handle[idx], size, dst);
}

int vfs_close(int fd) {
  if (fd < 3)
    return 0;

  fd_table_t *table = scheduler_current_fds();
  int idx = fd - 3;
  if (!table || idx >= MAX_FDS_PER_PROCESS || table->handle[idx] < 0)
    return -1;

  int result = file_close(table->handle[idx]);
  if (result == 0)
    table->handle[idx] = -1;
  return result;
}

void vfs_fork_fds(fd_table_t *child) {
  fd_table_t *parent = scheduler_current_fds();
  if (!parent || !child)
    return;

  for (int i = 0; i < MAX_FDS_PER_PROCESS; i++) {
    child->handle[i] = parent->handle[i];
    if (child->handle[i] >= 0 && current_driver && current_driver->dup_file) {
      current_driver->dup_file(child->handle[i]);
    }
  }
}

void vfs_exit_fds(fd_table_t *table) {
  if (!table || !current_driver || !current_driver->close_file)
    return;

  for (int i = 0; i < MAX_FDS_PER_PROCESS; i++) {
    if (table->handle[i] >= 0) {
      current_driver->close_file(table->handle[i]);
      table->handle[i] = -1;
    }
  }
}
