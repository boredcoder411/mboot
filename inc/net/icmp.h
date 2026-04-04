#pragma once

#include <stdint.h>

#define ICMP_ECHO_REPLY 0
#define ICMP_ECHO_REQUEST 8

typedef struct __attribute__((packed)) {
  uint8_t type;
  uint8_t code;
  uint16_t checksum;
  uint16_t identifier;
  uint16_t sequence;
} icmp_echo_hdr;
