#pragma once

#include <stdint.h>

typedef struct {
  uint16_t htype;
  uint16_t ptype;
  uint8_t hlen;
  uint8_t plen;
  uint16_t opcode;
  uint8_t sender_mac[6];
  uint8_t sender_ip[4];
  uint8_t target_mac[6];
  uint8_t target_ip[4];
} __attribute__((packed)) arp_pkt;

void arp_process_packet(uint8_t *data, uint16_t len);
void arp_send_request(uint8_t src_ip[4], uint8_t target_ip[4]);
int arp_send_reply(const uint8_t target_mac[6], const uint8_t sender_ip[4],
                   const uint8_t target_ip[4]);
int arp_try_get_mac(uint8_t target_ip[4], uint8_t out_mac[6]);
