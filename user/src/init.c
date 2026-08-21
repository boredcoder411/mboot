#include "libc.h"

#define TTY1 0
#define TTY2 1
#define TTY3 2
#define TTY4 3

static void spawn_background(const char *path, int tty, char **argv) {
  int pid = fork();
  if (pid != 0)
    return;

  int child = execv_tty(path, argv, tty);
  exit(child >= 0 ? 0 : 1);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  printf("init: booting\n");

  char *lua_argv[] = {"/lua.elf", NULL};
  printf("init: launching Lua interpreter on tty4\n");
  execv_tty("/lua.elf", lua_argv, TTY4);

  char *server_argv[] = {"/udp_lua.elf", NULL};
  printf("init: launching UDP Lua server on tty2\n");
  spawn_background("/udp_lua.elf", TTY2, server_argv);

  char *ping_argv[] = {"/ping.elf", "142.251.142.14", NULL};
  printf("init: launching ping on tty3\n");
  spawn_background("/ping.elf", TTY3, ping_argv);

  for (;;) {
    for (volatile int i = 0; i < 100000; ++i)
      asm volatile("pause");
  }

  return 0;
}