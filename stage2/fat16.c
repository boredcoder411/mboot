#include "fat16.h"
#include "dev/disk.h"
#include "dev/serial.h"
#include "mbr.h"
#include "mem.h"
#include "utils.h"

bool read_bytes(uint32_t part_offset, void *buf, size_t len) {
  uint32_t start_lba = part_offset / SECTOR_SIZE;
  uint32_t end_lba = (part_offset + len - 1) / SECTOR_SIZE;
  uint8_t *tmp = kmalloc((end_lba - start_lba + 1) * SECTOR_SIZE);
  if (!tmp)
    return false;

  ata_lba_read(start_lba, end_lba - start_lba + 1, tmp, 0);
  memcpy(buf, tmp + (part_offset % SECTOR_SIZE), len);
  kfree(tmp);
  return true;
}

uint32_t BPB_FAT_addr(const BPB_t *bpb) {
  return bpb->RsvdSecCnt * bpb->BytsPerSec;
}

uint32_t BPB_Root_addr(const BPB_t *bpb) {
  return BPB_FAT_addr(bpb) + bpb->NumFATs * bpb->FATSz16 * bpb->BytsPerSec;
}

uint32_t BPB_Data_addr(const BPB_t *bpb) {
  return BPB_Root_addr(bpb) + bpb->RootEntCnt * 32;
}

uint16_t get_next_cluster(uint32_t part_offset, const BPB_t *bpb,
                                 uint16_t cluster) {
  uint16_t val;
  read_bytes(part_offset + BPB_FAT_addr(bpb) + cluster * 2, &val, sizeof(val));
  return val;
}

void get_lfn_name(lfn_entry_t *lfn_entries, int count, char *out,
                         int outsize) {
  int pos = 0;
  for (int i = count - 1; i >= 0; i--) {
    for (int j = 0; j < 5; j++) {
      uint16_t c = lfn_entries[i].Name1[j];
      if (c == 0 || pos >= outsize - 1)
        break;
      out[pos++] = (char)c;
    }
    for (int j = 0; j < 6; j++) {
      uint16_t c = lfn_entries[i].Name2[j];
      if (c == 0 || pos >= outsize - 1)
        break;
      out[pos++] = (char)c;
    }
    for (int j = 0; j < 2; j++) {
      uint16_t c = lfn_entries[i].Name3[j];
      if (c == 0 || pos >= outsize - 1)
        break;
      out[pos++] = (char)c;
    }
  }
  out[pos] = 0;
}

void get_file_name(dir_t *dir, lfn_entry_t *lfn_entries, int lfn_count,
                          char *out, int outsize) {
  if (lfn_count > 0) {
    get_lfn_name(lfn_entries, lfn_count, out, outsize);
  } else {
    char name[9], ext[4];
    memcpy(name, dir->Name, 8);
    name[8] = 0;
    memcpy(ext, dir->Name + 8, 3);
    ext[3] = 0;
    for (int i = 7; i >= 0; i--)
      if (name[i] == ' ')
        name[i] = 0;
    for (int i = 2; i >= 0; i--)
      if (ext[i] == ' ')
        ext[i] = 0;

    int pos = 0;
    for (int i = 0; name[i] && pos < outsize - 1; i++)
      out[pos++] = name[i];
    if (ext[0] && pos < outsize - 1) {
      out[pos++] = '.';
      for (int i = 0; ext[i] && pos < outsize - 1; i++)
        out[pos++] = ext[i];
    }
    out[pos] = 0;
  }
}

void read_directory(const BPB_t *bpb, uint32_t part_offset,
                           uint16_t start_cluster, const char *prefix) {
  uint32_t cluster_size = bpb->BytsPerSec * bpb->SecPerClus;
  uint8_t *buffer = kmalloc(cluster_size);
  if (!buffer)
    return;

  uint16_t cluster = start_cluster;
  do {
    uint32_t offset =
        (cluster == 0)
            ? (part_offset + BPB_Root_addr(bpb))
            : (part_offset + BPB_Data_addr(bpb) + (cluster - 2) * cluster_size);

    if (!read_bytes(offset, buffer, cluster_size))
      break;

    int entries = cluster_size / sizeof(dir_t);
    lfn_entry_t lfn_entries[20];
    int lfn_count = 0;

    for (int i = 0; i < entries; i++) {
      dir_t *dir = (dir_t *)(buffer + i * sizeof(dir_t));
      if (dir->Name[0] == 0)
        break;
      if (dir->Name[0] == 0xE5) {
        lfn_count = 0;
        continue;
      }

      if (dir->Attr == DIR_ATTR_LFN) {
        if (lfn_count < 20)
          lfn_entries[lfn_count++] = *(lfn_entry_t *)dir;
        continue;
      }

      char *name = kmalloc(256);
      if (!name)
        continue;
      get_file_name(dir, lfn_entries, lfn_count, name, 256);
      lfn_count = 0;

      char *path = kmalloc(512);
      if (!path) {
        kfree(name);
        continue;
      }

      unsigned int pos = 0;
      for (int j = 0; prefix[j] && pos < 511; j++)
        path[pos++] = prefix[j];
      if (pos > 0 && pos < 511)
        path[pos++] = '/';
      for (int j = 0; name[j] && pos < 511; j++)
        path[pos++] = name[j];
      path[pos] = 0;

      bool skip = (name[0] == '.' ||
                   (strlen(name) == 10 && !strncmp(name, ".fseventsd", 10)));

      if (!skip) {
        if (dir->Attr & DIR_ATTR_DIRECTORY) {
          INFO("FAT16", "%s <DIR>", path);
          read_directory(bpb, part_offset, dir->FstClusLO, path);
        } else {
          INFO("FAT16", "%s <FILE> size=%u cluster=%u", path, dir->FileSize,
               dir->FstClusLO);
        }
      }

      kfree(name);
      kfree(path);
    }

    if (start_cluster == 0)
      break;

    cluster = get_next_cluster(part_offset, bpb, cluster);
  } while (cluster < 0xFFF8);

  kfree(buffer);
}

void fat16_scan(uint32_t disk_lba_start) {
  uint8_t *sector = kmalloc(SECTOR_SIZE);
  ata_lba_read(disk_lba_start, 1, sector, 0);

  uint16_t sig = *(uint16_t *)(sector + 510);
  if (sig != MBR_SIG) {
    INFO("FAT16", "No valid MBR");
    return;
  }

  partition_entry_t parts[4];
  memcpy(parts, sector + 0x1BE, sizeof(parts));
  int fat_index = -1;
  for (int i = 0; i < 4; i++) {
    if (parts[i].type == 0x06) {
      fat_index = i;
      break;
    }
  }
  if (fat_index < 0) {
    INFO("FAT16", "No FAT16 partition found");
    return;
  }

  uint32_t part_lba = parts[fat_index].first_lba;
  uint32_t part_offset = part_lba * SECTOR_SIZE;
  INFO("FAT16", "FAT16 partition @ LBA %u (offset %u bytes)", part_lba,
       part_offset);

  BPB_t bpb;
  read_bytes(part_offset, &bpb, sizeof(bpb));
  INFO("FAT16",
       "BytsPerSec=%u SecPerClus=%u NumFATs=%u RootEntCnt=%u FATSz16=%u",
       bpb.BytsPerSec, bpb.SecPerClus, bpb.NumFATs, bpb.RootEntCnt,
       bpb.FATSz16);

  read_directory(&bpb, part_offset, 0, "");

  kfree(sector);
}
