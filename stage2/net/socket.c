#include "net/socket.h"
#include "dev/serial.h"
#include "mem.h"
#include "net/icmp.h"
#include "net/ipv4.h"
#include "net/udp.h"
#include "scheduler.h"
#include "utils.h"
#include <stdint.h>

#define SOCKET_MAX 16
#define SOCKET_HANDLE_BASE (-4096)
#define SOCKET_BUF_SIZE 1472
#define SOCKET_OUT_SIZE 2048

typedef struct {
  int used;
  int refs;
  int domain;
  int type;
  int protocol;
  uint16_t local_port;
  int bound;
  int is_raw;
  uint16_t icmp_identifier;
  uint16_t icmp_sequence;
  int has_peer;
  uint8_t peer_ip[4];
  uint16_t peer_port;
  uint8_t inbuf[SOCKET_BUF_SIZE];
  uint16_t in_len;
  uint16_t in_pos;
  uint8_t outbuf[SOCKET_OUT_SIZE];
  uint16_t out_len;
} socket_t;

static socket_t sockets[SOCKET_MAX];

static int socket_index_from_handle(int handle) {
  int index = SOCKET_HANDLE_BASE - handle;
  if (index < 0 || index >= SOCKET_MAX)
    return -1;
  return index;
}

static int socket_handle_from_index(int index) {
  return SOCKET_HANDLE_BASE - index;
}

static socket_t *socket_from_handle(int handle) {
  int index = socket_index_from_handle(handle);
  if (index < 0 || !sockets[index].used)
    return NULL;
  return &sockets[index];
}

static int sockaddr_in_valid(const struct sockaddr *addr, socklen_t addrlen) {
  if (!addr || addrlen < sizeof(struct sockaddr_in))
    return 0;
  return ((const struct sockaddr_in *)addr)->sin_family == AF_INET;
}

static void sockaddr_from_peer(struct sockaddr *addr, socklen_t *addrlen,
                               const uint8_t ip[4], uint16_t port) {
  if (!addr || !addrlen || *addrlen < sizeof(struct sockaddr_in))
    return;

  struct sockaddr_in *in = (struct sockaddr_in *)addr;
  memset(in, 0, sizeof(*in));
  in->sin_family = AF_INET;
  in->sin_port = htons(port);
  in->sin_addr.s_addr = ((uint32_t)ip[0] << 24) | ((uint32_t)ip[1] << 16) |
                        ((uint32_t)ip[2] << 8) | ip[3];
  *addrlen = sizeof(*in);
}

static void socket_flush_output(socket_t *sock) {
  if (!sock || !sock->has_peer || sock->out_len == 0)
    return;
  if (udp_send_local(sock->local_port, sock->peer_ip, sock->peer_port,
                     sock->outbuf, sock->out_len) == 0) {
    sock->out_len = 0;
  }
}

void socket_init(void) { memset(sockets, 0, sizeof(sockets)); }

int socket_create(int domain, int type, int protocol) {
  if (domain != AF_INET)
    return -1;
  if (type == SOCK_DGRAM && protocol != 0)
    return -1;
  if (type == SOCK_RAW && protocol != IPPROTO_ICMP)
    return -1;
  if (type != SOCK_DGRAM && type != SOCK_RAW)
    return -1;

  for (int i = 0; i < SOCKET_MAX; i++) {
    if (!sockets[i].used) {
      memset(&sockets[i], 0, sizeof(sockets[i]));
      sockets[i].used = 1;
      sockets[i].refs = 1;
      sockets[i].domain = domain;
      sockets[i].type = type;
      sockets[i].protocol = protocol;
      if (type == SOCK_RAW) {
        sockets[i].is_raw = 1;
        sockets[i].icmp_identifier = (uint16_t)(0x1000 + i);
        sockets[i].icmp_sequence = 1;
      }
      INFO("SOCKET", "created %s socket %d",
           type == SOCK_RAW ? "raw ICMP" : "UDP", i);
      return socket_handle_from_index(i);
    }
  }

  return -1;
}

int socket_dup(int handle) {
  socket_t *sock = socket_from_handle(handle);
  if (!sock)
    return -1;
  sock->refs++;
  return 0;
}

int socket_close(int handle) {
  socket_t *sock = socket_from_handle(handle);
  if (!sock)
    return -1;
  if (sock->refs > 0)
    sock->refs--;
  if (sock->refs == 0)
    memset(sock, 0, sizeof(*sock));
  return 0;
}

int socket_bind(int handle, const struct sockaddr *addr, socklen_t addrlen) {
  socket_t *sock = socket_from_handle(handle);
  if (!sock || !sockaddr_in_valid(addr, addrlen))
    return -1;

  const struct sockaddr_in *in = (const struct sockaddr_in *)addr;
  uint16_t port = ntohs(in->sin_port);
  if (port == 0)
    return -1;

  if (udp_server_bind(port) < 0)
    return -1;

  sock->local_port = port;
  sock->bound = 1;
  INFO("SOCKET", "bound socket to UDP port %u", port);
  return 0;
}

int socket_read(int handle, void *buf, size_t len) {
  socket_t *sock = socket_from_handle(handle);
  if (!sock || !sock->bound || !buf || len > 65535)
    return -1;

  for (;;) {
    CLI();
    if (sock->in_pos < sock->in_len) {
      STI();
      break;
    }
    sock->in_len = 0;
    sock->in_pos = 0;
    int ret = udp_server_recv(sock->local_port, sock->peer_ip, &sock->peer_port,
                              sock->inbuf, sizeof(sock->inbuf), &sock->in_len);
    if (ret == 0) {
      sock->has_peer = 1;
      socket_flush_output(sock);
      STI();
      break;
    }
    scheduler_block((void *)(uintptr_t)sock->local_port);
  }

  uint16_t available = sock->in_len - sock->in_pos;
  size_t take = len < available ? len : available;
  memcpy(buf, sock->inbuf + sock->in_pos, take);
  sock->in_pos += (uint16_t)take;
  return (int)take;
}

int socket_write(int handle, const void *buf, size_t len) {
  socket_t *sock = socket_from_handle(handle);
  if (!sock || !sock->bound || !buf || len > 65535)
    return -1;

  if (sock->has_peer) {
    return udp_send_local(sock->local_port, sock->peer_ip, sock->peer_port, buf,
                          (uint16_t)len) == 0
               ? (int)len
               : -1;
  }

  size_t room = SOCKET_OUT_SIZE - sock->out_len;
  size_t take = len < room ? len : room;
  memcpy(sock->outbuf + sock->out_len, buf, take);
  sock->out_len += (uint16_t)take;
  return (int)len;
}

int socket_sendto(int handle, const void *buf, size_t len, int flags,
                  const struct sockaddr *dest, socklen_t addrlen) {
  (void)flags;
  socket_t *sock = socket_from_handle(handle);
  if (!sock || !buf || !sockaddr_in_valid(dest, addrlen) || len > 65535)
    return -1;

  const struct sockaddr_in *in = (const struct sockaddr_in *)dest;
  const uint8_t dst_ip[4] = {
      (uint8_t)(in->sin_addr.s_addr >> 24),
      (uint8_t)(in->sin_addr.s_addr >> 16),
      (uint8_t)(in->sin_addr.s_addr >> 8),
      (uint8_t)in->sin_addr.s_addr,
  };

  if (sock->is_raw) {
    uint8_t src_ip[4];
    if (!ipv4_get_address(src_ip))
      return -1;
    if (icmp_send_echo(src_ip, dst_ip, sock->icmp_identifier,
                       sock->icmp_sequence) < 0)
      return -1;
    sock->icmp_sequence++;
    memcpy(sock->peer_ip, dst_ip, sizeof(sock->peer_ip));
    sock->has_peer = 1;
    return (int)len;
  }

  uint16_t src_port = sock->bound ? sock->local_port : 49152;
  uint16_t dst_port = ntohs(in->sin_port);
  if (udp_send_local(src_port, dst_ip, dst_port, buf, (uint16_t)len) < 0)
    return -1;
  memcpy(sock->peer_ip, dst_ip, sizeof(sock->peer_ip));
  sock->peer_port = dst_port;
  sock->has_peer = 1;
  return (int)len;
}

int socket_recvfrom(int handle, void *buf, size_t len, int flags,
                    struct sockaddr *src, socklen_t *addrlen) {
  (void)flags;
  socket_t *sock = socket_from_handle(handle);
  if (!sock || !buf || len > 65535)
    return -1;

  if (sock->is_raw) {
    for (;;) {
      CLI();
      if (sock->in_pos < sock->in_len) {
        STI();
        break;
      }
      sock->in_len = 0;
      sock->in_pos = 0;
      scheduler_block(sock);
    }

    uint16_t available = sock->in_len - sock->in_pos;
    size_t take = len < available ? len : available;
    memcpy(buf, sock->inbuf + sock->in_pos, take);
    sock->in_pos += (uint16_t)take;
    sockaddr_from_peer(src, addrlen, sock->peer_ip, 0);
    return (int)take;
  }

  if (!sock->bound)
    return -1;

  uint8_t peer_ip[4];
  uint16_t peer_port = 0;
  uint16_t payload_len = 0;
  for (;;) {
    CLI();
    if (udp_server_recv(sock->local_port, peer_ip, &peer_port, buf,
                        (uint16_t)len, &payload_len) >= 0) {
      STI();
      break;
    }
    scheduler_block((void *)(uintptr_t)sock->local_port);
  }

  memcpy(sock->peer_ip, peer_ip, sizeof(sock->peer_ip));
  sock->peer_port = peer_port;
  sock->has_peer = 1;
  socket_flush_output(sock);
  sockaddr_from_peer(src, addrlen, peer_ip, peer_port);
  return payload_len;
}

int socket_icmp_rx(const uint8_t src_ip[4], const void *icmp_data,
                   uint16_t icmp_len) {
  if (!icmp_data || icmp_len < sizeof(icmp_echo_hdr))
    return -1;

  const icmp_echo_hdr *icmp = (const icmp_echo_hdr *)icmp_data;
  if (icmp->type != ICMP_ECHO_REPLY || icmp->code != 0)
    return -1;

  uint16_t identifier = ntohs(icmp->identifier);
  for (int i = 0; i < SOCKET_MAX; i++) {
    socket_t *sock = &sockets[i];
    if (!sock->used || !sock->is_raw || sock->icmp_identifier != identifier)
      continue;

    uint16_t take = icmp_len < SOCKET_BUF_SIZE ? icmp_len : SOCKET_BUF_SIZE;
    memcpy(sock->inbuf, icmp_data, take);
    sock->in_len = take;
    sock->in_pos = 0;
    memcpy(sock->peer_ip, src_ip, sizeof(sock->peer_ip));
    sock->has_peer = 1;
    INFO("SOCKET", "raw ICMP reply queued to socket %d (%u bytes)", i, take);
    scheduler_wake(sock);
    return 0;
  }

  return -1;
}

int socket_handle_valid(int handle) {
  return socket_from_handle(handle) != NULL;
}
