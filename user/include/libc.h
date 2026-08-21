#pragma once

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef long ssize_t;
typedef uint32_t socklen_t;
typedef uint16_t sa_family_t;

#define AF_INET 2
#define SOCK_DGRAM 2
#define SOCK_RAW 3
#define IPPROTO_ICMP 1
#define htons(x) ((((x) & 0xFF) << 8) | ((x) >> 8))
#define ntohs(x) htons(x)
#define htonl(x)                                                               \
  ((((uint32_t)(x) & 0x000000FFU) << 24) |                                     \
   (((uint32_t)(x) & 0x0000FF00U) << 8) |                                      \
   (((uint32_t)(x) & 0x00FF0000U) >> 8) |                                      \
   (((uint32_t)(x) & 0xFF000000U) >> 24))
#define ntohl(x) htonl(x)

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

int open(const char *pathname, int flags);
int fork(void);
int execv(const char *pathname, char *const argv[]);
int dup2(int oldfd, int newfd);
int socket(int domain, int type, int protocol);
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest, socklen_t addrlen);
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src, socklen_t *addrlen);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int close(int fd);
