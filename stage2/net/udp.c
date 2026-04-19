#include "net/udp.h"
#include "dev/serial.h"
#include "mem.h"
#include "net/arp.h"
#include "utils.h"

#define UDP_MAX_BINDINGS 8
#define UDP_ECHO_PORT 7

typedef struct {
  uint16_t port;
  udp_handler_t handler;
} udp_binding_t;

static udp_binding_t udp_bindings[UDP_MAX_BINDINGS];
static uint16_t udp_tx_identification = 1;

static uint16_t udp_checksum(const uint8_t src_ip[4], const uint8_t dst_ip[4],
                             const void *segment, uint16_t segment_len) {
  const uint8_t *bytes = (const uint8_t *)segment;
  uint32_t sum = 0;

  sum += ((uint32_t)src_ip[0] << 8) | src_ip[1];
  sum += ((uint32_t)src_ip[2] << 8) | src_ip[3];
  sum += ((uint32_t)dst_ip[0] << 8) | dst_ip[1];
  sum += ((uint32_t)dst_ip[2] << 8) | dst_ip[3];
  sum += IPV4_PROTOCOL_UDP;
  sum += segment_len;

  while (segment_len > 1) {
    sum += ((uint32_t)bytes[0] << 8) | bytes[1];
    bytes += 2;
    segment_len -= 2;
  }

  if (segment_len != 0) {
    sum += (uint32_t)bytes[0] << 8;
  }

  while ((sum >> 16) != 0) {
    sum = (sum & 0xFFFF) + (sum >> 16);
  }

  return (uint16_t)~sum;
}

static int udp_checksum_valid(const uint8_t src_ip[4], const uint8_t dst_ip[4],
                              const void *segment, uint16_t segment_len) {
  return udp_checksum(src_ip, dst_ip, segment, segment_len) == 0;
}

static udp_handler_t udp_find_handler(uint16_t port) {
  for (int i = 0; i < UDP_MAX_BINDINGS; ++i) {
    if (udp_bindings[i].handler != NULL && udp_bindings[i].port == port) {
      return udp_bindings[i].handler;
    }
  }

  return NULL;
}

static int udp_echo_handler(const uint8_t src_ip[4], uint16_t src_port,
                            const uint8_t dst_ip[4], uint16_t dst_port,
                            const uint8_t *payload, uint16_t payload_len) {
  INFO("UDP", "Echoing %u bytes back to port %u", payload_len, src_port);
  return udp_send(dst_ip, src_ip, NULL, dst_port, src_port, payload,
                  payload_len);
}

void udp_init(void) {
  memset(udp_bindings, 0, sizeof(udp_bindings));
  udp_bind(UDP_ECHO_PORT, udp_echo_handler);
  INFO("UDP", "UDP echo service listening on port %u", UDP_ECHO_PORT);
}

int udp_bind(uint16_t port, udp_handler_t handler) {
  for (int i = 0; i < UDP_MAX_BINDINGS; ++i) {
    if (udp_bindings[i].handler != NULL && udp_bindings[i].port == port) {
      udp_bindings[i].handler = handler;
      return 0;
    }
  }

  for (int i = 0; i < UDP_MAX_BINDINGS; ++i) {
    if (udp_bindings[i].handler == NULL) {
      udp_bindings[i].port = port;
      udp_bindings[i].handler = handler;
      INFO("UDP", "Bound handler to port %u", port);
      return 0;
    }
  }

  INFO("UDP", "No free UDP binding slots for port %u", port);
  return -1;
}

int udp_send(const uint8_t src_ip[4], const uint8_t dst_ip[4],
             const uint8_t next_hop_mac[6], uint16_t src_port,
             uint16_t dst_port, const void *payload, uint16_t payload_len) {
  uint8_t resolved_next_hop[6];
  if (next_hop_mac == NULL) {
    uint8_t dst_lookup[4];
    memcpy(dst_lookup, dst_ip, sizeof(dst_lookup));
    if (!arp_try_get_mac(dst_lookup, resolved_next_hop)) {
      INFO("UDP", "No ARP entry for %d.%d.%d.%d", dst_ip[0], dst_ip[1],
           dst_ip[2], dst_ip[3]);
      return -1;
    }
    next_hop_mac = resolved_next_hop;
  }

  uint16_t segment_len = sizeof(udp_hdr) + payload_len;
  uint8_t *segment = kmalloc(segment_len);
  udp_hdr *udp = (udp_hdr *)segment;

  udp->src_port = htons(src_port);
  udp->dst_port = htons(dst_port);
  udp->dgram_len = htons(segment_len);
  udp->dgram_cksum = 0;

  if (payload_len != 0) {
    memcpy(segment + sizeof(udp_hdr), payload, payload_len);
  }

  uint16_t checksum = udp_checksum(src_ip, dst_ip, segment, segment_len);
  udp->dgram_cksum = htons(checksum == 0 ? 0xFFFF : checksum);

  int result =
      ipv4_send_packet(src_ip, dst_ip, next_hop_mac, IPV4_PROTOCOL_UDP,
                       udp_tx_identification++, segment, segment_len);
  kfree(segment);
  return result;
}

void udp_process_packet(uint8_t *frame, uint16_t frame_len, ipv4_hdr *ipv4,
                        uint8_t *udp_data, uint16_t udp_len) {
  (void)frame;
  (void)frame_len;

  if (udp_len < sizeof(udp_hdr)) {
    INFO("UDP", "Dropping runt UDP packet");
    return;
  }

  udp_hdr *udp = (udp_hdr *)udp_data;
  uint16_t src_port = ntohs(udp->src_port);
  uint16_t dst_port = ntohs(udp->dst_port);
  uint16_t dgram_len = ntohs(udp->dgram_len);

  if (dgram_len < sizeof(udp_hdr) || dgram_len > udp_len) {
    INFO("UDP", "Dropping malformed UDP datagram");
    return;
  }

  if (udp->dgram_cksum != 0) {
    uint16_t received_checksum = ntohs(udp->dgram_cksum);
    udp->dgram_cksum = 0;
    uint16_t expected_checksum =
        udp_checksum(ipv4->src_ip, ipv4->dst_ip, udp, dgram_len);
    udp->dgram_cksum = htons(received_checksum);

    if (!udp_checksum_valid(ipv4->src_ip, ipv4->dst_ip, udp, dgram_len)) {
      INFO("UDP",
           "Checksum mismatch: got %04x expected %04x for "
           "%d.%d.%d.%d:%u -> %d.%d.%d.%d:%u len=%u",
           received_checksum, expected_checksum, ipv4->src_ip[0],
           ipv4->src_ip[1], ipv4->src_ip[2], ipv4->src_ip[3], src_port,
           ipv4->dst_ip[0], ipv4->dst_ip[1], ipv4->dst_ip[2], ipv4->dst_ip[3],
           dst_port, dgram_len);
      return;
    }
  }

  uint8_t *payload = udp_data + sizeof(udp_hdr);
  uint16_t payload_len = dgram_len - sizeof(udp_hdr);

  INFO("UDP", "RX %d.%d.%d.%d:%u -> %d.%d.%d.%d:%u (%u bytes)",
       ipv4->src_ip[0], ipv4->src_ip[1], ipv4->src_ip[2], ipv4->src_ip[3],
       src_port, ipv4->dst_ip[0], ipv4->dst_ip[1], ipv4->dst_ip[2],
       ipv4->dst_ip[3], dst_port, payload_len);

  udp_handler_t handler = udp_find_handler(dst_port);
  if (handler == NULL) {
    INFO("UDP", "No UDP handler bound for port %u", dst_port);
    return;
  }

  handler(ipv4->src_ip, src_port, ipv4->dst_ip, dst_port, payload, payload_len);
}
