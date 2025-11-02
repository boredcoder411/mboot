#pragma once

#include <stdbool.h>
#include <stdint.h>

#define DIR_ATTR_LFN 0x0F
#define DIR_ATTR_DIRECTORY 0x10
#define SECTOR_SIZE 512
#define MAX_OPEN_FILES 8

typedef struct {
  char path[256];
  uint16_t start_cluster;
  uint32_t size;
  uint32_t offset;
  bool used;
} fat16_file_t;

typedef struct {
  uint8_t jmpBoot[3];
  uint8_t OEMName[8];
  uint16_t BytsPerSec;
  uint8_t SecPerClus;
  uint16_t RsvdSecCnt;
  uint8_t NumFATs;
  uint16_t RootEntCnt;
  uint16_t TotSec16;
  uint8_t Media;
  uint16_t FATSz16;
  uint16_t SecPerTrk;
  uint16_t NumHeads;
  uint32_t HiddSec;
  uint32_t TotSec32;
} __attribute__((packed)) BPB_t;

typedef struct {
  uint8_t Name[11];
  uint8_t Attr;
  uint8_t NTRes;
  uint8_t CrtTimeTenth;
  uint16_t CrtTime;
  uint16_t CrtDate;
  uint16_t LstAccDate;
  uint16_t FstClusHI;
  uint16_t WrtTime;
  uint16_t WrtDate;
  uint16_t FstClusLO;
  uint32_t FileSize;
} __attribute__((packed)) dir_t;

typedef struct {
  uint8_t Ord;
  uint16_t Name1[5];
  uint8_t Attr;
  uint8_t Type;
  uint8_t Chksum;
  uint16_t Name2[6];
  uint16_t FstClusLO;
  uint16_t Name3[2];
} __attribute__((packed)) lfn_entry_t;

void fat16_init();
int fat16_get_size(int fd);
