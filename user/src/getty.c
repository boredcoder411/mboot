// Print a terminal label and replace this process with Lua; this is not an authentication service.
#include "libc.h"

/* Getty displays a label and starts the interpreter; it does not validate
   credentials or restart Lua if it exits. */
int main(int argc, char **argv) {
  int tty = 0;
  if (argc > 1)
    tty = atoi(argv[1]);

  printf("tty%d login: \n", tty + 1);

  /* Bind standard streams to this TTY, then replace the getty image with
     Lua. A successful exec continues in Lua rather than returning here. */
  char *lua_argv[] = {"/lua.elf", NULL};
  ioctl(0, TIOCSCTTY, (unsigned long)tty);
  execv("/lua.elf", lua_argv);

  return 0;
}
