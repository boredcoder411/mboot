// ICMP echo-request transmission and echo-reply delivery for the ping demo.
#include "net/icmp.h"
#include "dev/serial.h"
#include "mem.h"
#include "net/arp.h"
#include "net/ipv4.h"
#include "net/socket.h"
#include "utils.h"

/* ICMP handling checks the incoming message and supports echo traffic used
   by the ping demonstration; this is not a complete ICMP implementation. */
void icmp_process_packet(uint8_t *frame, uint16_t frame_len, ipv4_hdr *ipv4,
                         uint8_t *icmp_data, uint16_t icmp_len) {
  if (icmp_len < sizeof(icmp_echo_hdr))
    return;

  icmp_echo_hdr *icmp = (icmp_echo_hdr *)icmp_data;
  /* Only replies are dispatched here: incoming echo requests are not
     answered, and the received ICMP checksum is not validated here. */
  if (icmp->type == ICMP_ECHO_REPLY && icmp->code == 0) {
    INFO("ICMP",
         "Echo reply from %d.%d.%d.%d id=0x%04x seq=%u payload=%u bytes",
         ipv4->src_ip[0], ipv4->src_ip[1], ipv4->src_ip[2], ipv4->src_ip[3],
         ntohs(icmp->identifier), ntohs(icmp->sequence),
         icmp_len - sizeof(icmp_echo_hdr));
    // skloppy deref cause unused
    (void)frame;
    (void)frame_len;
    (void)ipv4;
    socket_icmp_rx(ipv4->src_ip, icmp_data, icmp_len);
  }
}

int icmp_send_echo(const uint8_t src_ip[4], const uint8_t dst_ip[4],
                   uint16_t identifier, uint16_t sequence) {
  static const uint8_t payload[] = "mboot-icmp";
  size_t icmp_len = sizeof(icmp_echo_hdr) + sizeof(payload) - 1;
  uint8_t *icmp_packet = kmalloc(icmp_len);
  icmp_echo_hdr *icmp = (icmp_echo_hdr *)icmp_packet;

  icmp->type = ICMP_ECHO_REQUEST;
  icmp->code = 0;
  icmp->identifier = htons(identifier);
  icmp->sequence = htons(sequence);

  uint8_t *icmp_payload = (uint8_t *)(icmp + 1);
  memcpy(icmp_payload, payload, sizeof(payload) - 1);
  icmp->checksum = 0;
  /* Calculate over the entire ICMP message with its checksum field zeroed.
     Identifier and sequence let the receiver associate replies with a socket. */
  icmp->checksum = htons(ipv4_checksum(icmp, icmp_len));

  /* Resolve the destination on the local subnet, or the gateway when the
     destination is remote. The IP destination remains dst_ip in both cases. */
  uint8_t next_hop[6];
  if (ipv4_on_local_network(dst_ip)) {
    if (!arp_resolve(dst_ip, next_hop)) {
      INFO("ICMP", "No ARP entry for %d.%d.%d.%d", dst_ip[0], dst_ip[1],
           dst_ip[2], dst_ip[3]);
      kfree(icmp_packet);
      return -1;
    }
  } else {
    uint8_t gw[4];
    if (!ipv4_get_gateway(gw) || !arp_resolve(gw, next_hop)) {
      INFO("ICMP", "No route to %d.%d.%d.%d", dst_ip[0], dst_ip[1], dst_ip[2],
           dst_ip[3]);
      kfree(icmp_packet);
      return -1;
    }
  }

  INFO("ICMP",
       "Sending echo request: %d.%d.%d.%d via %02x:%02x:%02x:%02x:%02x:%02x",
       dst_ip[0], dst_ip[1], dst_ip[2], dst_ip[3], next_hop[0], next_hop[1],
       next_hop[2], next_hop[3], next_hop[4], next_hop[5]);

  int result = ipv4_send_packet(src_ip, dst_ip, next_hop, IPV4_PROTOCOL_ICMP,
                                sequence, icmp_packet, icmp_len);
  kfree(icmp_packet);
  return result;
}
