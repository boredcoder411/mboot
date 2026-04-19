#pragma once

#include "net/ipv4.h"
#include <stdint.h>

typedef struct {
  uint16_t src_port;
  uint16_t dst_port;
  uint16_t dgram_len;
  uint16_t dgram_cksum;
} __attribute__((packed)) udp_hdr;

typedef int (*udp_handler_t)(const uint8_t src_ip[4], uint16_t src_port,
                             const uint8_t dst_ip[4], uint16_t dst_port,
                             const uint8_t *payload, uint16_t payload_len);

void udp_init(void);
int udp_bind(uint16_t port, udp_handler_t handler);
int udp_send(const uint8_t src_ip[4], const uint8_t dst_ip[4],
             const uint8_t next_hop_mac[6], uint16_t src_port,
             uint16_t dst_port, const void *payload, uint16_t payload_len);
void udp_process_packet(uint8_t *frame, uint16_t frame_len, ipv4_hdr *ipv4,
                        uint8_t *udp_data, uint16_t udp_len);
