#pragma once

#include "net/ipv4.h"
#include <stdint.h>

#define ICMP_ECHO_REPLY 0
#define ICMP_ECHO_REQUEST 8

typedef struct {
  uint8_t type;
  uint8_t code;
  uint16_t checksum;
  uint16_t identifier;
  uint16_t sequence;
} __attribute__((packed)) icmp_echo_hdr;

void icmp_process_packet(uint8_t *frame, uint16_t frame_len, ipv4_hdr *ipv4,
                         uint8_t *icmp_data, uint16_t icmp_len);
int icmp_send_echo(uint8_t src_ip[4], uint8_t dst_ip[4],
                   uint8_t next_hop_mac[6], uint16_t identifier,
                   uint16_t sequence);
