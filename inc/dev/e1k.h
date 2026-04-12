#pragma once

#include "dev/nic.h"
#include <stddef.h>
#include <stdint.h>

#define E1K_REG_CTRL 0x0000
#define E1K_REG_STATUS 0x0008
#define E1K_CTRL_RST (1 << 26)
#define E1K_CTRL_SLU (1 << 6)
#define E1K_CTRL_FRCSPD (1 << 11)
#define E1K_CTRL_FRCDPX (1 << 12)
#define E1K_CTRL_FD (1 << 0)
#define E1K_REG_EERD 0x0014
#define E1K_EERD_START (1 << 0)
#define E1K_EERD_DONE (1 << 4)
#define E1K_REG_EECD 0x0010
#define E1K_REG_RDTR 0x2820
#define E1K_REG_RADV 0x282C
#define E1K_REG_IMS 0x00D0
#define E1K_REG_ICR 0x00C0
#define E1K_REG_ITR 0x00C4
#define E1K_EECD_EE_PRES (1 << 8)

// Interrupt Cause/Mask bits
#define E1K_ICR_TXDW (1 << 0)   // Transmit Descriptor Written Back
#define E1K_ICR_TXQE (1 << 1)   // Transmit Queue Empty
#define E1K_ICR_LSC (1 << 2)    // Link Status Change
#define E1K_ICR_RXSEQ (1 << 3)  // Receive Sequence Error
#define E1K_ICR_RXDMT0 (1 << 4) // Receive Descriptor Minimum Threshold Hit
#define E1K_ICR_RXO (1 << 6)    // Receiver Overrun
#define E1K_ICR_RXT0 (1 << 7)   // Receiver Timer Interrupt
#define E1K_ICR_RXDW (1 << 11)  // Receive Descriptor Written Back
#define E1K_ICR_ACK (1 << 15)   // TCP ACK Received
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

#define E1K_RAL0 0x5400
#define E1K_RAH0 0x5404
#define E1K_RAH_AV (1U << 31)
#define E1K_RDBAL 0x2800
#define E1K_RDBAH 0x2804
#define E1K_RDLEN 0x2808
#define E1K_RDH 0x2810
#define E1K_RDT 0x2818
#define E1K_RXDCTL 0x2828
#define E1K_RXDCTL_EN (1 << 25)
#define E1K_RCTL 0x0100
#define E1K_RCTL_EN (1 << 1)
#define E1K_RCTL_SBP (1 << 2)
#define E1K_RCTL_UPE (1 << 3)
#define E1K_RCTL_MPE (1 << 4)
#define E1K_RCTL_LPE (1 << 5)
#define E1K_RCTL_LBM_NONE (0 << 6)
#define E1K_RCTL_LBM_LOOPBACK (1 << 6)
#define E1K_RCTL_RDMTS_HALF (0 << 8)
#define E1K_RCTL_MO_3 (3 << 12)
#define E1K_RCTL_MDR (1 << 16)
#define E1K_RCTL_BAM (1 << 15)
#define E1K_RCTL_SZ_2048 (0 << 16)
#define E1K_RCTL_VFE (1 << 18)
#define E1K_RCTL_CFIEN (1 << 19)
#define E1K_RCTL_SECRC (1 << 26)

#define E1K_RXD_STAT_DD (1 << 0)
#define E1K_RXD_STAT_EOP (1 << 1)
#define E1K_RXD_STAT_IXSM (1 << 2)
#define E1K_RXD_STAT_VP (1 << 3)
#define E1K_RXD_STAT_UDPCS (1 << 4)
#define E1K_RXD_STAT_TCPCS (1 << 5)
#define E1K_RXD_STAT_IPCS (1 << 6)
#define E1K_RXD_STAT_PIF (1 << 7)
#define E1K_RXD_ERR_CE (1 << 0)
#define E1K_RXD_ERR_SE (1 << 1)
#define E1K_RXD_ERR_PEO (1 << 2)
#define E1K_RXD_ERR_RXE (1 << 3)
#define E1K_RXD_ERR_FRAME_ERR_MASK                                             \
  (E1K_RXD_ERR_CE | E1K_RXD_ERR_SE | E1K_RXD_ERR_PEO | E1K_RXD_ERR_RXE)

#define CMD_EOP (1 << 0)
#define CMD_IFCS (1 << 1)
#define CMD_RS (1 << 3)

#define STATUS_DD (1 << 0)

#define NUM_TX_DESC 8
#define TX_BUF_SIZE 2048
#define NUM_RX_DESC 32
#define RX_BUF_SIZE 2048

typedef struct {
  uint64_t addr;
  uint16_t length;
  uint8_t cso;
  uint8_t cmd;
  uint8_t status;
  uint8_t css;
  uint16_t special;
} __attribute__((packed, aligned(16))) e1k_tx_desc_t;

typedef struct {
  uint64_t addr;     // 0-7: Buffer address
  uint16_t length;   // 8-9: Packet length
  uint16_t checksum; // 10-11: Checksum
  uint8_t status;    // 12: Status flags
  uint8_t errors;    // 13: Error flags
  uint16_t special;  // 14-15: Special flags
} __attribute__((packed, aligned(16))) e1k_rx_desc_t;

// RX descriptor is 16 bytes
_Static_assert(sizeof(e1k_rx_desc_t) == 16, "RX descriptor must be 16 bytes");

#define E1K_RX_DESC_ALIGN __attribute__((aligned(128)))
#define E1K_TX_DESC_ALIGN __attribute__((aligned(128)))

void e1k_init(nic_descriptor nic_desc);
int e1k_send(void *frame, size_t len);
void e1k_drain_rx(void);
void e1k_get_mac(uint8_t out_mac[6]);
