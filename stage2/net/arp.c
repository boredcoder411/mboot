#include "net/arp.h"
#include "dev/e1k.h"
#include "dev/serial.h"
#include "mem.h"
#include "net/eth.h"
#include "net/ipv4.h"
#include "utils.h"

static uint8_t arp_cached_ip[4];
static uint8_t arp_cached_mac[6];
static int arp_cache_valid = 0;

void arp_update_cache(const uint8_t sender_ip[4], const uint8_t sender_mac[6]) {
  memcpy(arp_cached_ip, sender_ip, 4);
  memcpy(arp_cached_mac, sender_mac, 6);
  arp_cache_valid = 1;

  INFO("ARP", "Cache update: %d.%d.%d.%d is at %02x:%02x:%02x:%02x:%02x:%02x",
       sender_ip[0], sender_ip[1], sender_ip[2], sender_ip[3], sender_mac[0],
       sender_mac[1], sender_mac[2], sender_mac[3], sender_mac[4],
       sender_mac[5]);
}

static int ip_equal(const uint8_t lhs[4], const uint8_t rhs[4]) {
  for (int i = 0; i < 4; ++i) {
    if (lhs[i] != rhs[i])
      return 0;
  }
  return 1;
}

void arp_process_packet(uint8_t *data, uint16_t len) {
  if (len < sizeof(arp_pkt))
    return;

  arp_pkt *arp = (arp_pkt *)data;

  INFO("ARP", "Received ARP opcode=%u, sender=%d.%d.%d.%d, target=%d.%d.%d.%d",
       ntohs(arp->opcode), arp->sender_ip[0], arp->sender_ip[1],
       arp->sender_ip[2], arp->sender_ip[3], arp->target_ip[0],
       arp->target_ip[1], arp->target_ip[2], arp->target_ip[3]);

  arp_update_cache(arp->sender_ip, arp->sender_mac);

  if (ntohs(arp->opcode) == 1) {
    if (ipv4_is_local_address(arp->target_ip)) {
      INFO("ARP", "ARP request targets local host, sending reply");
      arp_send_reply(arp->sender_mac, arp->target_ip, arp->sender_ip);
    }
  }
}

void arp_send_request(uint8_t src_ip[4], uint8_t target_ip[4]) {
  size_t frame_len = sizeof(eth_hdr) + sizeof(arp_pkt);
  if (frame_len < 60)
    frame_len = 60;

  uint8_t *frame = kmalloc(frame_len);
  memset(frame, 0, frame_len);

  eth_hdr *eth = (eth_hdr *)frame;
  memset(eth->dst, 0xFF, 6);
  e1k_get_mac(eth->src);
  eth->ethertype = htons(0x0806);

  arp_pkt *arp = (arp_pkt *)(frame + sizeof(eth_hdr));
  arp->htype = htons(1);
  arp->ptype = htons(0x0800);
  arp->hlen = 6;
  arp->plen = 4;
  arp->opcode = htons(1);

  e1k_get_mac(arp->sender_mac);
  memcpy(arp->sender_ip, src_ip, 4);
  memset(arp->target_mac, 0x00, 6);
  memcpy(arp->target_ip, target_ip, 4);

  INFO("ARP", "Sending request: who has %d.%d.%d.%d ? tell %d.%d.%d.%d",
       target_ip[0], target_ip[1], target_ip[2], target_ip[3], src_ip[0],
       src_ip[1], src_ip[2], src_ip[3]);

  int r = e1k_send(frame, frame_len);
  if (r != 0) {
    INFO("ARP", "e1k_send failed with %d", r);
  } else {
    INFO("ARP", "Request sent successfully");
  }

  kfree(frame);
}

int arp_send_reply(const uint8_t target_mac[6], const uint8_t sender_ip[4],
                   const uint8_t target_ip[4]) {
  size_t frame_len = sizeof(eth_hdr) + sizeof(arp_pkt);
  if (frame_len < 60)
    frame_len = 60;

  uint8_t *frame = kmalloc(frame_len);
  memset(frame, 0, frame_len);

  eth_hdr *eth = (eth_hdr *)frame;
  memcpy(eth->dst, target_mac, 6);
  e1k_get_mac(eth->src);
  eth->ethertype = htons(0x0806);

  arp_pkt *arp = (arp_pkt *)(frame + sizeof(eth_hdr));
  arp->htype = htons(1);
  arp->ptype = htons(0x0800);
  arp->hlen = 6;
  arp->plen = 4;
  arp->opcode = htons(2);
  e1k_get_mac(arp->sender_mac);
  memcpy(arp->sender_ip, sender_ip, 4);
  memcpy(arp->target_mac, target_mac, 6);
  memcpy(arp->target_ip, target_ip, 4);

  int result = e1k_send(frame, frame_len);
  if (result != 0) {
    INFO("ARP", "Failed to send ARP reply: %d", result);
  }

  kfree(frame);
  return result;
}

int arp_try_get_mac(uint8_t target_ip[4], uint8_t out_mac[6]) {
  if (!arp_cache_valid || !ip_equal(target_ip, arp_cached_ip))
    return 0;

  memcpy(out_mac, arp_cached_mac, 6);
  return 1;
}
