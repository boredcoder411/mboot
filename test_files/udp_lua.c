// Fork a Lua process with its standard descriptors connected to a UDP socket.
#include "libc.h"

static void spawn_remote_lua(int udp_fd) {
  int pid = fork();
  if (pid != 0)
    return;

  /* In the child, route all three standard descriptors through the socket.
     exec retains the descriptors, so Lua can use its normal stream APIs. */
  dup2(udp_fd, 0);
  dup2(udp_fd, 1);
  dup2(udp_fd, 2);
  if (udp_fd > 2)
    close(udp_fd);

  char *argv[] = {"/lua.elf", NULL};
  int child = execv("/lua.elf", argv);
  exit(child >= 0 ? 0 : 1);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  printf("udp_lua: creating UDP socket\n");
  int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (udp_fd < 0) {
    printf("udp_lua: socket failed\n");
    return 1;
  }

  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(7);

  if (bind(udp_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    printf("udp_lua: bind failed\n");
    return 1;
  }

  printf("udp_lua: launching remote Lua child\n");
  spawn_remote_lua(udp_fd);

  /* Keep the parent alive; this demo does not supervise or restart Lua. */
  for (;;) {
    for (volatile int i = 0; i < 100000; ++i)
      asm volatile("pause");
  }
}
