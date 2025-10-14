#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define SECTOR_SIZE 512
#define PARTITION_ENTRY_OFFSET 446
#define PARTITION_ENTRY_SIZE 16
#define PARTITION_COUNT 4

struct PartitionEntry {
    uint8_t status;
    uint8_t chs_first[3];
    uint8_t type;
    uint8_t chs_last[3];
    uint32_t lba_first_sector;
    uint32_t num_sectors;
};

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <disk.img> <partition_number> <assets.wad>\n", argv[0]);
        return 1;
    }

    const char *img_path = argv[1];
    int part_num = atoi(argv[2]);
    const char *wad_path = argv[3];

    if (part_num < 1 || part_num > 4) {
        fprintf(stderr, "Error: Partition number must be between 1 and 4.\n");
        return 1;
    }

    FILE *img = fopen(img_path, "r+b");
    if (!img) {
        perror("Error opening disk image");
        return 1;
    }

    uint8_t mbr[SECTOR_SIZE];
    if (fread(mbr, 1, SECTOR_SIZE, img) != SECTOR_SIZE) {
        perror("Error reading MBR");
        fclose(img);
        return 1;
    }

    struct PartitionEntry part;
    size_t entry_offset = PARTITION_ENTRY_OFFSET + (part_num - 1) * PARTITION_ENTRY_SIZE;
    fseek(img, entry_offset, SEEK_SET);
    if (fread(&part, sizeof(part), 1, img) != 1) {
        perror("Error reading partition entry");
        fclose(img);
        return 1;
    }

    uint64_t offset_bytes = (uint64_t)part.lba_first_sector * SECTOR_SIZE;
    printf("Partition %d starts at sector %u (offset %llu bytes)\n",
           part_num, part.lba_first_sector, (unsigned long long)offset_bytes);

    FILE *wad = fopen(wad_path, "rb");
    if (!wad) {
        perror("Error opening assets.wad");
        fclose(img);
        return 1;
    }

    if (fseek(img, offset_bytes, SEEK_SET) != 0) {
        perror("Error seeking to partition start");
        fclose(img);
        fclose(wad);
        return 1;
    }

    printf("Writing %s to partition %d...\n", wad_path, part_num);
    uint8_t buffer[4096];
    size_t bytes;
    size_t total_written = 0;

    while ((bytes = fread(buffer, 1, sizeof(buffer), wad)) > 0) {
        if (fwrite(buffer, 1, bytes, img) != bytes) {
            perror("Error writing to image");
            fclose(img);
            fclose(wad);
            return 1;
        }
        total_written += bytes;
    }

    printf("Done. Wrote %zu bytes to partition %d.\n", total_written, part_num);

    fclose(img);
    fclose(wad);
    return 0;
}

