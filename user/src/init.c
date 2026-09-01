#include "libc.h"

#define TTY_COUNT 4

static void spawn_background(const char *path, int tty, char **argv) {
  int pid = fork();
  if (pid != 0)
    return;

  if (tty >= 0) {
    ioctl(0, TIOCSCTTY, (unsigned long)tty);
  }
  int child = execv(path, argv);
  exit(child >= 0 ? 0 : 1);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  printf("init: booting\n");

  static const char *tty_nums[] = {"0", "1", "2", "3"};
  for (int i = 0; i < TTY_COUNT; i++) {
    char *getty_argv[] = {"/getty.elf", (char *)tty_nums[i], NULL};
    printf("init: launching getty on tty%d\n", i + 1);
    spawn_background("/getty.elf", i, getty_argv);
  }

  for (;;) {
    for (volatile int i = 0; i < 100000; ++i)
      asm volatile("pause");
  }

  return 0;
}
