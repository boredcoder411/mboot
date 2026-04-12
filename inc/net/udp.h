#pragma once

#include "net/ipv4.h"
#include <stdint.h>

// from https://doc.dpdk.org/api-2.2/structudp__hdr.html
typedef struct {
  uint16_t src_port;
  uint16_t dst_port;
  uint16_t dgram_len;
  uint16_t dgram_cksum;
} __attribute__((packed)) udp_pkt;

void udp_process_packet(uint8_t *frame, uint16_t frame_len, ipv4_hdr *ipv4,
                        uint8_t *udp_data, uint16_t udp_len);
