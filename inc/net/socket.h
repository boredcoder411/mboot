// Socket operations and address definitions shared with the VFS.
#pragma once

#include <stddef.h>
#include <stdint.h>

#define AF_INET 2
#define SOCK_DGRAM 2
#define SOCK_RAW 3
#define IPPROTO_ICMP 1

typedef uint32_t socklen_t;
typedef uint16_t sa_family_t;

struct sockaddr {
  sa_family_t sa_family;
  char sa_data[14];
};

struct in_addr {
  uint32_t s_addr;
};

struct sockaddr_in {
  sa_family_t sin_family;
  uint16_t sin_port;
  struct in_addr sin_addr;
  uint8_t sin_zero[8];
};

void socket_init(void);
int socket_create(int domain, int type, int protocol);
int socket_dup(int handle);
int socket_close(int handle);
int socket_bind(int handle, const struct sockaddr *addr, socklen_t addrlen);
int socket_read(int handle, void *buf, size_t len);
int socket_write(int handle, const void *buf, size_t len);
int socket_sendto(int handle, const void *buf, size_t len, int flags,
                  const struct sockaddr *dest, socklen_t addrlen);
int socket_recvfrom(int handle, void *buf, size_t len, int flags,
                    struct sockaddr *src, socklen_t *addrlen);
int socket_handle_valid(int handle);
int socket_icmp_rx(const uint8_t src_ip[4], const void *icmp_data,
                   uint16_t icmp_len);
