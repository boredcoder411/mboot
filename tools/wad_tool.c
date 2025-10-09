#include "fs.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int copy_file(FILE *dst, const char *src_path, uint32_t *bytes_written) {
  FILE *src = fopen(src_path, "rb");
  if (!src) {
    fprintf(stderr, "Failed to open %s: %s\n", src_path, strerror(errno));
    return -1;
  }

  char buffer[4096];
  size_t n;
  uint32_t total = 0;
  while ((n = fread(buffer, 1, sizeof(buffer), src)) > 0) {
    if (fwrite(buffer, 1, n, dst) != n) {
      fprintf(stderr, "Write error while packing %s\n", src_path);
      fclose(src);
      return -1;
    }
    total += (uint32_t)n;
  }

  fclose(src);
  if (bytes_written)
    *bytes_written = total;
  return 0;
}

int pack_wad(const char *out_path, const char **in_files, const int num_in) {
  FILE *out = fopen(out_path, "wb");
  if (!out) {
    perror("fopen");
    return -1;
  }

  wad_header_t header = {"PWAD", (uint32_t)num_in, 0};
  fwrite(&header, sizeof(header), 1, out);

  lump_entry_t *entries = calloc(num_in, sizeof(lump_entry_t));
  if (!entries) {
    perror("calloc");
    fclose(out);
    return -1;
  }

  uint32_t current_offset = sizeof(wad_header_t);

  for (int i = 0; i < num_in; i++) {
    entries[i].offset = current_offset;

    uint32_t bytes_written = 0;
    if (copy_file(out, in_files[i], &bytes_written) < 0) {
      free(entries);
      fclose(out);
      return -1;
    }

    entries[i].size = bytes_written;

    const char *fname = strrchr(in_files[i], '/');
    fname = fname ? fname + 1 : in_files[i];
    strncpy(entries[i].name, fname, 8);
    entries[i].name[8 - 1] = '\0';

    current_offset += bytes_written;
    printf("Packed %-8s size=%u offset=%u\n", entries[i].name, entries[i].size,
           entries[i].offset);
  }

  header.dir_offset = current_offset;

  fwrite(entries, sizeof(lump_entry_t), num_in, out);

  fseek(out, 0, SEEK_SET);
  fwrite(&header, sizeof(header), 1, out);

  fclose(out);
  free(entries);

  printf("WAD written: %s\n", out_path);
  printf("  lumps: %d\n", num_in);
  printf("  dir offset: %u\n", header.dir_offset);

  return 0;
}

int main(int argc, const char **argv) {
  if (argc < 3) {
    fprintf(stderr, "Usage: %s <output.wad> <input files...>\n", argv[0]);
    return 1;
  }

  const char *out_file = argv[1];
  const char **in_files = &argv[2];
  int num_in_files = argc - 2;

  return pack_wad(out_file, in_files, num_in_files);
}
