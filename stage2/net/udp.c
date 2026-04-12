#include "net/udp.h"
#include "dev/serial.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"
void udp_process_packet(uint8_t *frame, uint16_t frame_len, ipv4_hdr *ipv4,
                        uint8_t *udp_data, uint16_t udp_len) {
  INFO("UDP", "got udp packet");
}
