// Build and send ICMP echo traffic through the raw socket interface.
#include "libc.h"

static int parse_ip(const char *s, uint8_t ip[4]) {
  /* Accept exactly four decimal octets separated by dots. The final test
     rejects any trailing text after the fourth octet. */
  for (int i = 0; i < 4; i++) {
    int v = 0;
    int digits = 0;
    while (*s >= '0' && *s <= '9') {
      v = v * 10 + (*s - '0');
      s++;
      digits++;
    }
    if (digits == 0 || v > 255)
      return -1;
    ip[i] = (uint8_t)v;
    if (i < 3 && *s == '.') {
      s++;
    } else if (i < 3) {
      return -1;
    }
  }
  return *s == '\0' ? 0 : -1;
}

int main(int argc, char **argv) {
  uint8_t dst[4] = {8, 8, 8, 8};
  if (argc > 1 && parse_ip(argv[1], dst) < 0) {
    printf("ping: invalid address\n");
    return 1;
  }

  int fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (fd < 0) {
    printf("ping: socket failed\n");
    return 1;
  }

  struct sockaddr_in dest;
  bzero(&dest, sizeof(dest));
  dest.sin_family = AF_INET;
  dest.sin_addr.s_addr = ((uint32_t)dst[0] << 24) | ((uint32_t)dst[1] << 16) |
                         ((uint32_t)dst[2] << 8) | dst[3];

  printf("ping: pinging %d.%d.%d.%d\n", dst[0], dst[1], dst[2], dst[3]);
  const char *payload = "mboot-ping";

  /* The kernel's raw-socket path constructs an ICMP echo request; recvfrom
     waits for the reply routed back to this socket's ICMP identifier. */
  for (int seq = 1; seq <= 3; seq++) {
    int n = sendto(fd, payload, strlen(payload), 0, (struct sockaddr *)&dest,
                   sizeof(dest));
    if (n < 0) {
      printf("ping: sendto failed\n");
      return 1;
    }

    struct sockaddr_in src;
    socklen_t src_len = sizeof(src);
    uint8_t reply[512];
    int m = recvfrom(fd, reply, sizeof(reply), 0, (struct sockaddr *)&src,
                     &src_len);
    if (m > 0 && reply[0] == 0) {
      uint8_t *rip = (uint8_t *)&src.sin_addr.s_addr;
      printf("ping: reply from %d.%d.%d.%d seq=%d len=%d\n", rip[3], rip[2],
             rip[1], rip[0], seq, m);
    } else {
      printf("ping: no reply\n");
    }
  }

  close(fd);
  return 0;
}
