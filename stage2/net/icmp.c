#include "net/icmp.h"
#include "dev/serial.h"
#include "mem.h"
#include "net/ipv4.h"
#include "utils.h"

static void icmp_dump_bytes(const uint8_t *data, uint16_t len) {
  INFO("ICMP", "Raw bytes (%u bytes):", len);

  for (uint16_t i = 0; i < len; ++i) {
    if ((i % 16) == 0) {
      serial_printf("%03x: ", i);
    }

    serial_printf("%02x ", data[i]);

    if ((i % 16) == 15 || i == (uint16_t)(len - 1)) {
      write_serial('\n');
    }
  }
}

void icmp_process_packet(uint8_t *frame, uint16_t frame_len, ipv4_hdr *ipv4,
                         uint8_t *icmp_data, uint16_t icmp_len) {
  if (icmp_len < sizeof(icmp_echo_hdr))
    return;

  icmp_echo_hdr *icmp = (icmp_echo_hdr *)icmp_data;
  if (icmp->type == ICMP_ECHO_REPLY && icmp->code == 0) {
    INFO("ICMP",
         "Echo reply from %d.%d.%d.%d id=0x%04x seq=%u payload=%u bytes",
         ipv4->src_ip[0], ipv4->src_ip[1], ipv4->src_ip[2], ipv4->src_ip[3],
         ntohs(icmp->identifier), ntohs(icmp->sequence),
         icmp_len - sizeof(icmp_echo_hdr));
    icmp_dump_bytes(frame, frame_len);
  }
}

int icmp_send_echo(uint8_t src_ip[4], uint8_t dst_ip[4],
                   uint8_t next_hop_mac[6], uint16_t identifier,
                   uint16_t sequence) {
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
  icmp->checksum = htons(ipv4_checksum(icmp, icmp_len));

  INFO("ICMP",
       "Sending echo request: %d.%d.%d.%d via %02x:%02x:%02x:%02x:%02x:%02x",
       dst_ip[0], dst_ip[1], dst_ip[2], dst_ip[3], next_hop_mac[0],
       next_hop_mac[1], next_hop_mac[2], next_hop_mac[3], next_hop_mac[4],
       next_hop_mac[5]);

  int result = ipv4_send_packet(src_ip, dst_ip, next_hop_mac, IPV4_PROTOCOL_ICMP,
                                sequence, icmp_packet, icmp_len);
  kfree(icmp_packet);
  return result;
}
