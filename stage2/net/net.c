// Initialize the network stack with the configuration used for the QEMU network.
#include "net/net.h"
#include "dev/serial.h"
#include "net/ipv4.h"
#include "net/socket.h"
#include "net/udp.h"

void net_init(void) {
  /* QEMU's user-mode network normally uses this subnet and gateway. These
     values are static: there is no DHCP discovery or lease renewal. */
  const uint8_t ip[4] = {10, 0, 2, 15};
  const uint8_t netmask[4] = {255, 255, 255, 0};
  const uint8_t gateway[4] = {10, 0, 2, 2};

  ipv4_set_address(ip);
  ipv4_set_netmask(netmask);
  ipv4_set_gateway(gateway);
  udp_init();
  socket_init();

  INFO("NET", "interface configured: %d.%d.%d.%d/24 via %d.%d.%d.%d", ip[0],
       ip[1], ip[2], ip[3], gateway[0], gateway[1], gateway[2], gateway[3]);
}
