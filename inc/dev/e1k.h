#pragma once

#include "dev/nic.h"
#include <stddef.h>
#include <stdint.h>

#define E1K_REG_CTRL 0x0000
#define E1K_REG_STATUS 0x0008
#define E1K_CTRL_RST (1 << 26)
#define E1K_REG_EERD 0x0014
#define E1K_EERD_START (1 << 0)
#define E1K_EERD_DONE (1 << 4)
#define E1K_REG_EECD 0x0010
#define E1K_REG_IMS 0x00D0
#define E1K_EECD_EE_PRES (1 << 8)
#define E1K_TDBAL 0x3800
#define E1K_TDBAH 0x3804
#define E1K_TDLEN 0x3808
#define E1K_TDH 0x3810
#define E1K_TDT 0x3818
#define E1K_TCTL 0x0400
#define E1K_TIPG 0x0410
#define E1K_TCTL_EN (1 << 1)
#define E1K_TCTL_PSP (1 << 3)
#define E1K_TCTL_CT_SHIFT 4
#define E1K_TCTL_COLD_SHIFT 12

#define CMD_EOP (1 << 0)
#define CMD_IFCS (1 << 1)
#define CMD_RS (1 << 3)

#define STATUS_DD (1 << 0)

#define NUM_TX_DESC 8
#define TX_BUF_SIZE 2048

typedef struct {
  uint64_t addr;
  uint16_t length;
  uint8_t cso;
  uint8_t cmd;
  uint8_t status;
  uint8_t css;
  uint16_t special;
} __attribute__((packed, aligned(16))) e1k_tx_desc_t;

void e1k_init(nic_descriptor nic_desc);
int e1k_send(void *frame, size_t len);
void e1k_send_arp_request(uint8_t src_ip[4], uint8_t target_ip[4]);
