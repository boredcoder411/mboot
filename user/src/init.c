#include "libc.h"

static int reopen_stdio(const char *tty) {
  int fd = open(tty, 2);
  if (fd < 0)
    return -1;

  dup2(fd, 0);
  dup2(fd, 1);
  dup2(fd, 2);
  if (fd > 2)
    close(fd);
  return 0;
}

static void spawn_background(const char *path, const char *tty, char **argv) {
  int pid = fork();
  if (pid != 0)
    return;

  reopen_stdio(tty);
  int child = execv(path, argv);
  exit(child >= 0 ? 0 : 1);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  printf("init: booting\n");

  char *lua_argv[] = {"/lua.elf", NULL};
  printf("init: launching Lua interpreter\n");
  execv("/lua.elf", lua_argv);

  reopen_stdio("/dev/tty4");

  char *server_argv[] = {"/udp_lua.elf", NULL};
  printf("init: launching UDP Lua server\n");
  spawn_background("/udp_lua.elf", "/dev/tty2", server_argv);

  char *ping_argv[] = {"/ping.elf", "142.251.142.14", NULL};
  printf("init: launching ping\n");
  spawn_background("/ping.elf", "/dev/tty3", ping_argv);

  for (;;) {
    for (volatile int i = 0; i < 100000; ++i)
      asm volatile("pause");
  }

  return 0;
}