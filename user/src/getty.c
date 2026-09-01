#include "libc.h"

int main(int argc, char **argv) {
  int tty = 0;
  if (argc > 1)
    tty = atoi(argv[1]);

  printf("tty%d login: \n", tty + 1);

  char *lua_argv[] = {"/lua.elf", NULL};
  ioctl(0, TIOCSCTTY, (unsigned long)tty);
  execv("/lua.elf", lua_argv);

  return 0;
}