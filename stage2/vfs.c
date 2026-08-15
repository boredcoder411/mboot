#include "vfs.h"
#include "cpu/cpuid.h"
#include "dev/serial.h"
#include "dev/tty.h"
#include "mem.h"
#include "scheduler.h"

static vfs_driver_t *current_driver = NULL;

#define VFS_EMPTY_HANDLE (-1)
#define VFS_DEV_NULL (-2)
#define VFS_DEV_ZERO (-3)
#define VFS_DEV_FULL (-4)
#define VFS_DEV_CPUINFO_BASE (-8)
#define VFS_DEV_TTY_BASE (-32)
#define CPUINFO_OPEN_FILES 4

typedef struct {
  int used;
  int refs;
  size_t offset;
} cpuinfo_file_t;

static cpuinfo_file_t cpuinfo_files[CPUINFO_OPEN_FILES];

void register_vfs_driver(vfs_driver_t *drv) { current_driver = drv; }

static const char *normalize_path(const char *path) {
  if (!path)
    return NULL;
  while (*path == '/' || (path[0] == '.' && path[1] == '/')) {
    if (*path == '/')
      path++;
    else
      path += 2;
  }
  return path;
}

static int path_eq(const char *a, const char *b) {
  if (!a || !b)
    return 0;
  while (*a && *b && *a == *b) {
    a++;
    b++;
  }
  return *a == '\0' && *b == '\0';
}

static int path_has_prefix(const char *path, const char *prefix) {
  if (!path || !prefix)
    return 0;
  while (*prefix) {
    if (*path != *prefix)
      return 0;
    path++;
    prefix++;
  }
  return 1;
}

static int tty_path_id(const char *name) {
  if (path_eq(name, "dev/tty"))
    return scheduler_current_tty();
  if (!path_has_prefix(name, "dev/tty"))
    return -1;
  char n = name[7];
  if (n < '1' || n > ('0' + TTY_COUNT) || name[8] != '\0')
    return -1;
  return n - '1';
}

static int cpuinfo_open(const char *name) {
  if (!path_eq(name, "dev/cpuinfo"))
    return VFS_EMPTY_HANDLE;

  for (int i = 0; i < CPUINFO_OPEN_FILES; i++) {
    if (!cpuinfo_files[i].used) {
      cpuinfo_files[i].used = 1;
      cpuinfo_files[i].refs = 1;
      cpuinfo_files[i].offset = 0;
      return VFS_DEV_CPUINFO_BASE - i;
    }
  }

  return VFS_EMPTY_HANDLE;
}

static int device_open(const char *path) {
  const char *name = normalize_path(path);
  if (path_eq(name, "dev/null"))
    return VFS_DEV_NULL;
  if (path_eq(name, "dev/zero"))
    return VFS_DEV_ZERO;
  if (path_eq(name, "dev/full"))
    return VFS_DEV_FULL;
  int cpuinfo = cpuinfo_open(name);
  if (cpuinfo != VFS_EMPTY_HANDLE)
    return cpuinfo;
  int tty = tty_path_id(name);
  if (tty >= 0)
    return VFS_DEV_TTY_BASE - tty;
  return VFS_EMPTY_HANDLE;
}

static int cpuinfo_handle_id(int handle) {
  if (handle <= VFS_DEV_CPUINFO_BASE &&
      handle > VFS_DEV_CPUINFO_BASE - CPUINFO_OPEN_FILES) {
    return VFS_DEV_CPUINFO_BASE - handle;
  }
  return -1;
}

static int tty_handle_id(int handle) {
  if (handle <= VFS_DEV_TTY_BASE &&
      handle > VFS_DEV_TTY_BASE - TTY_COUNT) {
    return VFS_DEV_TTY_BASE - handle;
  }
  return -1;
}

static int cpuinfo_read(int handle, size_t size, void *dst) {
  int id = cpuinfo_handle_id(handle);
  if (id < 0 || !cpuinfo_files[id].used || !dst)
    return -1;
  if (size == 0)
    return 0;

  const char *text = cpuid_cpuinfo_text();
  size_t len = cpuid_cpuinfo_size();
  size_t offset = cpuinfo_files[id].offset;
  if (offset >= len)
    return 0;

  size_t remaining = len - offset;
  size_t take = size < remaining ? size : remaining;
  memcpy(dst, text + offset, take);
  cpuinfo_files[id].offset += take;
  return (int)take;
}

static int device_read(int handle, size_t size, void *dst) {
  if (handle == VFS_DEV_NULL)
    return 0;
  if (handle == VFS_DEV_ZERO || handle == VFS_DEV_FULL) {
    unsigned char *out = dst;
    for (size_t i = 0; i < size; i++)
      out[i] = 0;
    return (int)size;
  }
  int tty = tty_handle_id(handle);
  if (tty >= 0)
    return tty_read(tty, size, dst);
  int cpuinfo = cpuinfo_handle_id(handle);
  if (cpuinfo >= 0)
    return cpuinfo_read(handle, size, dst);
  return -1;
}

static int cpuinfo_close(int handle) {
  int id = cpuinfo_handle_id(handle);
  if (id < 0 || !cpuinfo_files[id].used)
    return -1;

  cpuinfo_files[id].refs--;
  if (cpuinfo_files[id].refs <= 0) {
    cpuinfo_files[id].used = 0;
    cpuinfo_files[id].offset = 0;
  }

  return 0;
}

static int cpuinfo_dup(int handle) {
  int id = cpuinfo_handle_id(handle);
  if (id < 0 || !cpuinfo_files[id].used)
    return -1;

  cpuinfo_files[id].refs++;
  return 0;
}

static int device_close(int handle) {
  if (cpuinfo_handle_id(handle) >= 0)
    return cpuinfo_close(handle);
  return 0;
}

static int device_dup(int handle) {
  if (cpuinfo_handle_id(handle) >= 0)
    return cpuinfo_dup(handle);
  return 0;
}

static int device_write(int handle, const void *src, size_t size) {
  if (handle == VFS_DEV_NULL || handle == VFS_DEV_ZERO)
    return (int)size;
  if (handle == VFS_DEV_FULL)
    return -1;
  int tty = tty_handle_id(handle);
  if (tty >= 0)
    return tty_write_buf(tty, src, size);
  return -1;
}

int file_open(const char *path) {
  int device = device_open(path);
  if (device != VFS_EMPTY_HANDLE)
    return device;

  if (!current_driver || !current_driver->open_file) {
    ERROR("VFS", "No valid VFS driver registered");
    return -1;
  }

  return current_driver->open_file(path);
}

int file_close(int handle) {
  if (handle < VFS_EMPTY_HANDLE)
    return device_close(handle);

  if (!current_driver || !current_driver->close_file) {
    ERROR("VFS", "No valid VFS driver registered");
    return -1;
  }

  return current_driver->close_file(handle);
}

int file_read(int handle, size_t size, void *dst) {
  if (handle < VFS_EMPTY_HANDLE)
    return device_read(handle, size, dst);

  if (!current_driver || !current_driver->read_file) {
    ERROR("VFS", "No valid VFS driver registered");
    return -1;
  }

  return current_driver->read_file(handle, dst, size);
}

int file_write(int handle, const void *src, size_t size) {
  if (handle < VFS_EMPTY_HANDLE)
    return device_write(handle, src, size);

  if (!current_driver || !current_driver->write_file)
    return -1;

  return current_driver->write_file(handle, src, size);
}

int vfs_open(const char *path) {
  int handle = file_open(path);
  if (handle == VFS_EMPTY_HANDLE)
    return -1;

  fd_table_t *table = scheduler_current_fds();
  if (!table) {
    file_close(handle);
    return -1;
  }

  for (int i = 0; i < MAX_FDS_PER_PROCESS; i++) {
    if (table->handle[i] == VFS_EMPTY_HANDLE) {
      table->handle[i] = handle;
      return i;
    }
  }

  ERROR("VFS", "process fd table full");
  file_close(handle);
  return -1;
}

int vfs_read(int fd, size_t size, void *dst) {
  if (fd < 0)
    return -1;

  fd_table_t *table = scheduler_current_fds();
  if (!table || fd >= MAX_FDS_PER_PROCESS ||
      table->handle[fd] == VFS_EMPTY_HANDLE)
    return -1;

  return file_read(table->handle[fd], size, dst);
}

int vfs_write(int fd, const void *src, size_t size) {
  if (fd < 0)
    return -1;

  fd_table_t *table = scheduler_current_fds();
  if (!table || fd >= MAX_FDS_PER_PROCESS ||
      table->handle[fd] == VFS_EMPTY_HANDLE)
    return -1;

  return file_write(table->handle[fd], src, size);
}

int vfs_close(int fd) {
  if (fd < 0)
    return -1;

  fd_table_t *table = scheduler_current_fds();
  if (!table || fd >= MAX_FDS_PER_PROCESS ||
      table->handle[fd] == VFS_EMPTY_HANDLE)
    return -1;

  int result = file_close(table->handle[fd]);
  if (result == 0)
    table->handle[fd] = VFS_EMPTY_HANDLE;
  return result;
}

int vfs_dup2(int oldfd, int newfd) {
  fd_table_t *table = scheduler_current_fds();
  if (!table || oldfd < 0 || newfd < 0 || oldfd >= MAX_FDS_PER_PROCESS ||
      newfd >= MAX_FDS_PER_PROCESS ||
      table->handle[oldfd] == VFS_EMPTY_HANDLE)
    return -1;

  if (oldfd == newfd)
    return newfd;

  if (table->handle[newfd] != VFS_EMPTY_HANDLE)
    vfs_close(newfd);

  table->handle[newfd] = table->handle[oldfd];
  if (table->handle[newfd] < VFS_EMPTY_HANDLE) {
    device_dup(table->handle[newfd]);
  } else if (table->handle[newfd] >= 0 && current_driver &&
             current_driver->dup_file) {
    current_driver->dup_file(table->handle[newfd]);
  }

  int tty = tty_handle_id(table->handle[newfd]);
  if (newfd == 0 && tty >= 0)
    scheduler_set_current_tty(tty);

  return newfd;
}

void vfs_init_fds(fd_table_t *table, int tty) {
  if (!table)
    return;

  for (int i = 0; i < MAX_FDS_PER_PROCESS; i++)
    table->handle[i] = VFS_EMPTY_HANDLE;

  if (!tty_valid(tty))
    tty = 0;

  int handle = VFS_DEV_TTY_BASE - tty;
  table->handle[0] = handle;
  table->handle[1] = handle;
  table->handle[2] = handle;
}

void vfs_fork_fds(fd_table_t *child) {
  fd_table_t *parent = scheduler_current_fds();
  if (!parent || !child)
    return;

  for (int i = 0; i < MAX_FDS_PER_PROCESS; i++) {
    child->handle[i] = parent->handle[i];
    if (child->handle[i] < VFS_EMPTY_HANDLE) {
      device_dup(child->handle[i]);
    } else if (child->handle[i] >= 0 && current_driver &&
               current_driver->dup_file) {
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
      table->handle[i] = VFS_EMPTY_HANDLE;
    } else if (table->handle[i] < VFS_EMPTY_HANDLE) {
      device_close(table->handle[i]);
      table->handle[i] = VFS_EMPTY_HANDLE;
    }
  }
}
