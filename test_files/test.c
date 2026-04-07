#include "./user/include/libc.h"

static char buffer[256];

void main() {
  int fd = open("/hi.txt", 0);
  if (fd < 0) {
    write(1, "Failed to open hi.txt\n", 22);
    exit(1);
  }

  int n = read(fd, buffer, sizeof(buffer) - 1);
  if (n < 0) {
    write(1, "Failed to read hi.txt\n", 21);
    close(fd);
    exit(1);
  }

  buffer[n] = '\0';
  write(1, buffer, n);

  close(fd);
  exit(0);
}
