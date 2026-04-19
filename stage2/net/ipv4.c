#include "net/ipv4.h"
#include "dev/e1k.h"
#include "dev/serial.h"
#include "mem.h"
#include "net/eth.h"
#include "net/icmp.h"
#include "net/udp.h"
#include "utils.h"

static uint8_t ipv4_local_ip[4];
static int ipv4_local_ip_valid = 0;

void ipv4_set_address(const uint8_t ip[4]) {
  memcpy(ipv4_local_ip, ip, 4);
  ipv4_local_ip_valid = 1;

  INFO("IPV4", "Local address set to %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

int ipv4_get_address(uint8_t out_ip[4]) {
  if (!ipv4_local_ip_valid)
    return 0;

  memcpy(out_ip, ipv4_local_ip, 4);
  return 1;
}

int ipv4_is_local_address(const uint8_t ip[4]) {
  if (!ipv4_local_ip_valid)
    return 0;

  for (int i = 0; i < 4; ++i) {
    if (ipv4_local_ip[i] != ip[i])
      return 0;
  }

  return 1;
}

uint16_t ipv4_checksum(const void *data, size_t len) {
  const uint8_t *bytes = (const uint8_t *)data;
  uint32_t sum = 0;

  while (len > 1) {
    sum += ((uint32_t)bytes[0] << 8) | bytes[1];
    bytes += 2;
    len -= 2;
  }

  if (len != 0) {
    sum += (uint32_t)bytes[0] << 8;
  }

  while ((sum >> 16) != 0) {
    sum = (sum & 0xFFFF) + (sum >> 16);
  }

  return (uint16_t)~sum;
}

int ipv4_send_packet(const uint8_t src_ip[4], const uint8_t dst_ip[4],
                     const uint8_t next_hop_mac[6], uint8_t protocol,
                     uint16_t identification, const void *payload,
                     uint16_t payload_len) {
  size_t packet_len = sizeof(eth_hdr) + sizeof(ipv4_hdr) + payload_len;
  size_t frame_len = packet_len;

  if (frame_len < 60)
    frame_len = 60;

  uint8_t *frame = kmalloc(frame_len);
  memset(frame, 0, frame_len);

  eth_hdr *eth = (eth_hdr *)frame;
  memcpy(eth->dst, next_hop_mac, 6);
  e1k_get_mac(eth->src);
  eth->ethertype = htons(0x0800);

  ipv4_hdr *ipv4 = (ipv4_hdr *)(frame + sizeof(eth_hdr));
  ipv4->version_ihl = 0x45;
  ipv4->dscp_ecn = 0;
  ipv4->total_length = htons(sizeof(ipv4_hdr) + payload_len);
  ipv4->identification = htons(identification);
  ipv4->flags_fragment_offset = htons(0x4000);
  ipv4->ttl = 64;
  ipv4->protocol = protocol;
  memcpy(ipv4->src_ip, src_ip, 4);
  memcpy(ipv4->dst_ip, dst_ip, 4);
  ipv4->header_checksum = 0;
  ipv4->header_checksum = htons(ipv4_checksum(ipv4, sizeof(ipv4_hdr)));

  if (payload_len != 0) {
    memcpy(frame + sizeof(eth_hdr) + sizeof(ipv4_hdr), payload, payload_len);
  }

  int result = e1k_send(frame, frame_len);
  if (result != 0) {
    INFO("IPV4", "Failed to send IPv4 packet: %d", result);
  }

  kfree(frame);
  return result;
}

void ipv4_process_packet(uint8_t *frame, uint16_t frame_len, uint8_t *data,
                         uint16_t len) {
  if (len < sizeof(ipv4_hdr))
    return;

  ipv4_hdr *ipv4 = (ipv4_hdr *)data;
  uint8_t version = ipv4->version_ihl >> 4;
  uint8_t ihl = (ipv4->version_ihl & 0x0F) * 4;
  uint16_t total_length = ntohs(ipv4->total_length);

  if (version != 4 || ihl < sizeof(ipv4_hdr) || len < ihl ||
      total_length < ihl || total_length > len) {
    INFO("IPV4", "Dropping malformed IPv4 packet");
    return;
  }

  uint16_t header_checksum = ipv4->header_checksum;
  ipv4->header_checksum = 0;
  uint16_t computed_checksum = htons(ipv4_checksum(ipv4, ihl));
  ipv4->header_checksum = header_checksum;

  if (header_checksum != computed_checksum) {
    INFO("IPV4", "Dropping packet with bad header checksum");
    return;
  }

  uint16_t fragment_field = ntohs(ipv4->flags_fragment_offset);
  if ((fragment_field & 0x3FFF) != 0 || (fragment_field & 0x2000) != 0) {
    INFO("IPV4", "Dropping fragmented IPv4 packet");
    return;
  }

  if (ipv4_local_ip_valid && !ipv4_is_local_address(ipv4->dst_ip) &&
      !(ipv4->dst_ip[0] == 255 && ipv4->dst_ip[1] == 255 &&
        ipv4->dst_ip[2] == 255 && ipv4->dst_ip[3] == 255)) {
    INFO("IPV4", "Dropping packet not addressed to local host");
    return;
  }

  INFO("IPV4", "RX from %d.%d.%d.%d, protocol=%u", ipv4->src_ip[0],
       ipv4->src_ip[1], ipv4->src_ip[2], ipv4->src_ip[3], ipv4->protocol);

  switch (ipv4->protocol) {
  case IPV4_PROTOCOL_ICMP: {
    uint8_t *icmp_data = data + ihl;
    uint16_t icmp_len = total_length - ihl;
    icmp_process_packet(frame, frame_len, ipv4, icmp_data, icmp_len);
    break;
  }
  case IPV4_PROTOCOL_UDP: {
    uint8_t *udp_data = data + ihl;
    uint16_t udp_len = total_length - ihl;
    udp_process_packet(frame, frame_len, ipv4, udp_data, udp_len);
    break;
  }
  default:
    INFO("IPV4", "Unsupported IPv4 protocol %u", ipv4->protocol);
    break;
  }
}
