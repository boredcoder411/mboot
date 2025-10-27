#include "mbr.h"
#include <inttypes.h>
#include <stdio.h>

void lba_to_chs(uint32_t lba, uint8_t heads, uint8_t sectors,
                       uint8_t chs[3]) {
  uint32_t c, h, s;

  c = lba / (heads * sectors);
  h = (lba / sectors) % heads;
  s = (lba % sectors) + 1;

  if (c > 1023) {
    c = 1023;
    h = 254;
    s = 63;
  }

  chs[0] = h & 0xFF;
  chs[1] = ((c >> 2) & 0xC0) | (s & 0x3F);
  chs[2] = c & 0xFF;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <disk_image>\n", argv[0]);
    return 1;
  }

  const char *filename = argv[1];

  FILE *fp = fopen(filename, "r+b");
  if (!fp) {
    perror("fopen");
    return 1;
  }

  mbr_t mbr;
  if (fread(&mbr, 1, sizeof(mbr), fp) != sizeof(mbr)) {
    fprintf(stderr, "Failed to read existing MBR\n");
    fclose(fp);
    return 1;
  }

  const uint8_t heads = 255;
  const uint8_t sectors = 63;

  mbr.partitions[0].status = 0x80;
  lba_to_chs(2048, heads, sectors, mbr.partitions[0].first_chs);
  lba_to_chs(2048 + 11 - 1, heads, sectors, mbr.partitions[0].last_chs);
  mbr.partitions[0].type = 0x83;
  mbr.partitions[0].first_lba = 2048;
  mbr.partitions[0].sector_count = 11;

  mbr.partitions[1].status = 0x00;
  lba_to_chs(4096, heads, sectors, mbr.partitions[1].first_chs);
  lba_to_chs(4096 + 16384 - 1, heads, sectors, mbr.partitions[1].last_chs);
  mbr.partitions[1].type = 0xEF;
  mbr.partitions[1].first_lba = 4096;
  mbr.partitions[1].sector_count = 16384;

  mbr.boot_signature = 0xAA55;

  fseek(fp, 0, SEEK_SET);
  if (fwrite(&mbr, 1, sizeof(mbr), fp) != sizeof(mbr)) {
    fprintf(stderr, "Failed to write MBR\n");
    fclose(fp);
    return 1;
  }

  fclose(fp);

  printf("Updated partition table in %s\n", filename);
  for (int i = 0; i < 4; i++) {
    partition_entry_t *p = &mbr.partitions[i];
    printf("Partition %d:\n", i + 1);
    printf("  Status:        0x%02" PRIX8 "\n", p->status);
    printf("  Type:          0x%02" PRIX8 "\n", p->type);
    printf("  First LBA:     %" PRIu32 "\n", p->first_lba);
    printf("  Sector Count:  %" PRIu32 "\n", p->sector_count);
    printf("\n");
  }

  return 0;
}
