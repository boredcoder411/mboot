#pragma once

#include <stdint.h>

#define IPV4_PROTOCOL_ICMP 1

typedef struct __attribute__((packed)) {
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
} ipv4_hdr;
