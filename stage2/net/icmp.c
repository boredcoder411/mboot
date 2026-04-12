#include "net/icmp.h"
#include "dev/e1k.h"
#include "dev/serial.h"
#include "mem.h"
#include "net/eth.h"
#include "net/ipv4.h"
#include "utils.h"

static uint16_t icmp_checksum(const void *data, size_t len) {
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
  const size_t payload_len = sizeof(payload) - 1;
  const size_t packet_len =
      sizeof(eth_hdr) + sizeof(ipv4_hdr) + sizeof(icmp_echo_hdr) + payload_len;
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
  ipv4->total_length =
      htons(sizeof(ipv4_hdr) + sizeof(icmp_echo_hdr) + payload_len);
  ipv4->identification = htons(sequence);
  ipv4->flags_fragment_offset = htons(0x4000);
  ipv4->ttl = 64;
  ipv4->protocol = IPV4_PROTOCOL_ICMP;
  memcpy(ipv4->src_ip, src_ip, 4);
  memcpy(ipv4->dst_ip, dst_ip, 4);
  ipv4->header_checksum = 0;
  ipv4->header_checksum = htons(icmp_checksum(ipv4, sizeof(ipv4_hdr)));

  icmp_echo_hdr *icmp =
      (icmp_echo_hdr *)(frame + sizeof(eth_hdr) + sizeof(ipv4_hdr));
  icmp->type = ICMP_ECHO_REQUEST;
  icmp->code = 0;
  icmp->identifier = htons(identifier);
  icmp->sequence = htons(sequence);

  uint8_t *icmp_payload = (uint8_t *)(icmp + 1);
  memcpy(icmp_payload, payload, payload_len);
  icmp->checksum = 0;
  icmp->checksum =
      htons(icmp_checksum(icmp, sizeof(icmp_echo_hdr) + payload_len));

  INFO("ICMP",
       "Sending echo request: %d.%d.%d.%d via %02x:%02x:%02x:%02x:%02x:%02x",
       dst_ip[0], dst_ip[1], dst_ip[2], dst_ip[3], next_hop_mac[0],
       next_hop_mac[1], next_hop_mac[2], next_hop_mac[3], next_hop_mac[4],
       next_hop_mac[5]);

  int result = e1k_send(frame, frame_len);
  if (result != 0) {
    INFO("ICMP", "Failed to send echo request: %d", result);
  }

  kfree(frame);
  return result;
}
