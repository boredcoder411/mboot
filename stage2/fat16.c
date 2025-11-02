#include "fat16.h"
#include "dev/disk.h"
#include "dev/serial.h"
#include "mbr.h"
#include "mem.h"
#include "utils.h"
#include "vfs.h"

static vfs_driver_t fat16_driver;
static BPB_t g_bpb;
static uint32_t g_part_offset = 0;
static fat16_file_t open_files[MAX_OPEN_FILES];

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

void get_lfn_name(lfn_entry_t *lfn_entries, int count, char *out, int outsize) {
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

static inline uint32_t cluster_offset(uint16_t cluster) {
  uint32_t cluster_size = g_bpb.BytsPerSec * g_bpb.SecPerClus;
  return g_part_offset + BPB_Data_addr(&g_bpb) + (cluster - 2) * cluster_size;
}

static bool find_file_in_dir(uint16_t start_cluster, const char *target_name,
                             dir_t *out_dir) {
  uint32_t cluster_size = g_bpb.BytsPerSec * g_bpb.SecPerClus;
  uint8_t *buffer = kmalloc(cluster_size);
  if (!buffer)
    return false;

  uint16_t cluster = start_cluster;
  do {
    uint32_t offset = (cluster == 0) ? (g_part_offset + BPB_Root_addr(&g_bpb))
                                     : cluster_offset(cluster);

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

      char name[256];
      get_file_name(dir, lfn_entries, lfn_count, name, sizeof(name));
      lfn_count = 0;

      if (!(dir->Attr & DIR_ATTR_DIRECTORY)) {
        if (strcasecmp(name, target_name) == 0) {
          memcpy(out_dir, dir, sizeof(dir_t));
          kfree(buffer);
          return true;
        }
      }
    }

    if (start_cluster == 0)
      break;

    cluster = get_next_cluster(g_part_offset, &g_bpb, cluster);
  } while (cluster < 0xFFF8);

  kfree(buffer);
  return false;
}

static int fat16_open(const char *path) {
  if (!path || !*path)
    return -1;

  const char *fname = path;
  while (*fname == '/')
    fname++;

  dir_t dir;
  if (!find_file_in_dir(0, fname, &dir)) {
    WARN("FAT16", "file '%s' not found", fname);
    return -1;
  }

  for (int i = 0; i < MAX_OPEN_FILES; i++) {
    if (!open_files[i].used) {
      open_files[i].used = true;
      memcpy(open_files[i].path, fname, sizeof(open_files[i].path) - 1);
      open_files[i].path[sizeof(open_files[i].path) - 1] = '\0';
      open_files[i].start_cluster = dir.FstClusLO;
      open_files[i].size = dir.FileSize;
      open_files[i].offset = 0;
      INFO("FAT16", "opened '%s' (cluster=%u size=%u)", fname, dir.FstClusLO,
           dir.FileSize);
      return i;
    }
  }

  ERROR("FAT16", "too many open files");
  return -1;
}

static int fat16_close(int fd) {
  if (fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].used)
    return -1;
  open_files[fd].used = false;
  INFO("FAT16", "closed '%s'", open_files[fd].path);
  return 0;
}

static int fat16_read(int fd, void *buffer, size_t size) {
  if (fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].used)
    return -1;
  if (size == 0)
    return 0;

  fat16_file_t *file = &open_files[fd];
  if (file->offset >= file->size)
    return 0;

  uint32_t cluster_size = g_bpb.BytsPerSec * g_bpb.SecPerClus;
  uint32_t remaining = file->size - file->offset;
  if (size > remaining)
    size = remaining;

  uint16_t cluster = file->start_cluster;
  uint32_t skip = file->offset;
  uint32_t skip_clusters = skip / cluster_size;
  uint32_t offset_in_cluster = skip % cluster_size;

  for (uint32_t i = 0; i < skip_clusters; i++) {
    cluster = get_next_cluster(g_part_offset, &g_bpb, cluster);
    if (cluster >= 0xFFF8)
      return -1;
  }

  uint8_t *tmp = kmalloc(cluster_size);
  if (!tmp)
    return -1;

  uint8_t *dst = buffer;
  size_t bytes_read = 0;
  size_t to_read = size;

  while (to_read > 0 && cluster < 0xFFF8) {
    if (!read_bytes(cluster_offset(cluster), tmp, cluster_size))
      break;

    size_t copy = cluster_size - offset_in_cluster;
    if (copy > to_read)
      copy = to_read;

    memcpy(dst + bytes_read, tmp + offset_in_cluster, copy);
    bytes_read += copy;
    to_read -= copy;
    offset_in_cluster = 0;

    if (to_read > 0)
      cluster = get_next_cluster(g_part_offset, &g_bpb, cluster);
  }

  kfree(tmp);
  file->offset += bytes_read;
  return (int)bytes_read;
}

int fat16_get_size(int fd) {
  return open_files[fd].size;
}

void fat16_init() {
  memset(open_files, 0, sizeof(open_files));

  uint8_t *mbr = kmalloc(SECTOR_SIZE);
  if (!mbr)
    return;

  if (!read_bytes(0, mbr, SECTOR_SIZE)) {
    kfree(mbr);
    return;
  }

  uint32_t start_lba = 0;
  for (int i = 0; i < 4; i++) {
    uint8_t *entry = mbr + 446 + i * 16;
    uint8_t type = entry[4];
    if (type == 0x04 || type == 0x06 || type == 0x0E) {
      memcpy(&start_lba, entry + 8, 4);
      break;
    }
  }
  kfree(mbr);

  if (!start_lba) {
    ERROR("FAT16", "no FAT16 partition found");
    return;
  }

  g_part_offset = start_lba * SECTOR_SIZE;

  uint8_t *sector0 = kmalloc(SECTOR_SIZE);
  if (!sector0)
    return;
  if (!read_bytes(g_part_offset, sector0, SECTOR_SIZE)) {
    kfree(sector0);
    return;
  }

  memcpy(&g_bpb, sector0, sizeof(BPB_t));
  kfree(sector0);

  INFO("FAT16", "initialized: part_offset=%u BytsPerSec=%u SecPerClus=%u",
       g_part_offset, g_bpb.BytsPerSec, g_bpb.SecPerClus);

  fat16_driver.open_file = fat16_open;
  fat16_driver.close_file = fat16_close;
  fat16_driver.read_file = fat16_read;
  register_vfs_driver(&fat16_driver);

  INFO("FAT16", "registered FAT16 driver with VFS");
}
