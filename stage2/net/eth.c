#include "net/eth.h"
#include "dev/serial.h"
#include "net/arp.h"
#include "net/ipv4.h"
#include "utils.h"

void eth_process_packet(uint8_t *data, uint16_t len) {
  eth_hdr *eth = (eth_hdr *)data;

  if (len < sizeof(eth_hdr))
    return;

  INFO("E1K", "RX packet: ethertype=0x%04x, len=%u", ntohs(eth->ethertype),
       len);

  uint16_t ethertype = ntohs(eth->ethertype);

  if (ethertype == 0x0806) {
    arp_process_packet(data + sizeof(eth_hdr), len - sizeof(eth_hdr));
  } else if (ethertype == 0x0800) {
    ipv4_process_packet(data, len, data + sizeof(eth_hdr),
                        len - sizeof(eth_hdr));
  }
}
