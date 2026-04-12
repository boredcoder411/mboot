#include "net/ipv4.h"
#include "dev/serial.h"
#include "net/icmp.h"
#include "net/udp.h"
#include "utils.h"

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
  }
}
