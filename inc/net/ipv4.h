#pragma once

#include <stddef.h>
#include <stdint.h>

#define IPV4_PROTOCOL_ICMP 1
#define IPV4_PROTOCOL_UDP 17

typedef struct {
  uint8_t version_ihl;
  uint8_t dscp_ecn;
  uint16_t total_length;
  uint16_t identification;
  uint16_t flags_fragment_offset;
  uint8_t ttl;
  uint8_t protocol;
  uint16_t header_checksum;
  uint8_t src_ip[4];
  uint8_t dst_ip[4];
} __attribute__((packed)) ipv4_hdr;

void ipv4_set_address(const uint8_t ip[4]);
int ipv4_get_address(uint8_t out_ip[4]);
int ipv4_is_local_address(const uint8_t ip[4]);
uint16_t ipv4_checksum(const void *data, size_t len);
int ipv4_send_packet(const uint8_t src_ip[4], const uint8_t dst_ip[4],
                     const uint8_t next_hop_mac[6], uint8_t protocol,
                     uint16_t identification, const void *payload,
                     uint16_t payload_len);
void ipv4_process_packet(uint8_t *frame, uint16_t frame_len, uint8_t *data,
                         uint16_t len);
