#include "libc.h"

#define SYS_NET_INIT 200
#define SYS_ARP_REQUEST 201
#define SYS_ARP_LOOKUP 202
#define SYS_ICMP_ECHO 203
#define SYS_UDP_SERVER_BIND 204
#define SYS_UDP_SERVER_RECV 205
#define SYS_UDP_SEND 206

static long syscall6(uint32_t num, uint32_t a, uint32_t b, uint32_t c,
                     uint32_t d, uint32_t e) {
  long ret;
  asm volatile("int $0x80"
               : "=a"(ret)
               : "a"(num), "b"(a), "c"(b), "d"(c), "S"(d), "D"(e)
               : "memory");
  return ret;
}

static void print_ip(const char *label, const uint8_t ip[4]) {
  printf("%s %d.%d.%d.%d\n", label, ip[0], ip[1], ip[2], ip[3]);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  const uint8_t src_ip[4] = {10, 0, 2, 15};
  const uint8_t gateway_ip[4] = {10, 0, 2, 2};
  const uint8_t ping_ip[4] = {142, 251, 142, 14};

  printf("init: booting\n");

  printf("init: configuring network\n");
  syscall6(SYS_NET_INIT, (uint32_t)src_ip, 0, 0, 0, 0);
  print_ip("init: guest IP", src_ip);

  printf("init: resolving gateway via ARP\n");
  syscall6(SYS_ARP_REQUEST, (uint32_t)src_ip, (uint32_t)gateway_ip, 0, 0, 0);

  uint8_t gateway_mac[6];
  int resolved = 0;
  for (int i = 0; i < 20000000; ++i) {
    if (syscall6(SYS_ARP_LOOKUP, (uint32_t)gateway_ip,
                 (uint32_t)gateway_mac, 0, 0, 0) == 0) {
      resolved = 1;
      break;
    }
    asm volatile("pause");
  }

  if (resolved) {
    printf("init: gateway MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
           gateway_mac[0], gateway_mac[1], gateway_mac[2], gateway_mac[3],
           gateway_mac[4], gateway_mac[5]);
    printf("init: sending ICMP echo\n");
    syscall6(SYS_ICMP_ECHO, (uint32_t)src_ip, (uint32_t)ping_ip,
             (uint32_t)gateway_mac, 0xB007, 1);
    print_ip("init: pinged", ping_ip);
  } else {
    printf("init: failed to resolve gateway ARP, skipping ping\n");
  }

  printf("init: starting UDP echo server on port 7\n");
  if (syscall6(SYS_UDP_SERVER_BIND, 7, 0, 0, 0, 0) < 0) {
    printf("init: failed to bind UDP port 7\n");
  }

  printf("init: launching lua\n");
  system("/lua.elf");

  uint8_t buf[1472];
  uint8_t peer_ip[4];
  uint16_t peer_port;
  for (;;) {
    int n = (int)syscall6(SYS_UDP_SERVER_RECV, 7, (uint32_t)peer_ip,
                          (uint32_t)&peer_port, (uint32_t)buf, sizeof(buf));
    if (n > 0) {
      printf("init: UDP echo from %d.%d.%d.%d:%u (%d bytes)\n", peer_ip[0],
             peer_ip[1], peer_ip[2], peer_ip[3], peer_port, n);
      syscall6(SYS_UDP_SEND, 7, (uint32_t)peer_ip, peer_port,
               (uint32_t)buf, (uint32_t)n);
    } else {
      for (volatile int i = 0; i < 100000; ++i) {
        asm volatile("pause");
      }
    }
  }

  return 0;
}
