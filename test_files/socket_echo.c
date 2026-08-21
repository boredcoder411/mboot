#include "libc.h"

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    printf("socket_echo: socket failed\n");
    return 1;
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(8);

  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    printf("socket_echo: bind failed\n");
    return 1;
  }

  printf("socket_echo: listening on UDP port 8\n");
  uint8_t buf[512];
  for (;;) {
    struct sockaddr_in peer;
    socklen_t peer_len = sizeof(peer);
    int n = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *)&peer,
                     &peer_len);
    if (n > 0)
      sendto(fd, buf, (size_t)n, 0, (struct sockaddr *)&peer, peer_len);
  }
}
