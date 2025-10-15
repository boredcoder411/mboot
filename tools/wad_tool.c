#include "../stage2/fs.h"
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

wad_type_t w_type_arg;

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
  FILE *out_fp = fopen(out_path, "wb");
  if (!out_fp) {
    fprintf(stderr, "Error opening %s", out_path);
    return -1;
  }

  wad_header_t header;
  memcpy(header.identifier, wad_type_names[w_type_arg], 4);
  header.num_lumps = (uint32_t)num_in;
  header.dir_offset = 0;
  fwrite(&header, sizeof(header), 1, out_fp);

  lump_entry_t *entries = calloc(num_in, sizeof(lump_entry_t));

  uint32_t current_offset = sizeof(wad_header_t);

  for (int i = 0; i < num_in; i++) {
    entries[i].offset = current_offset;

    uint32_t bytes_written = 0;
    if (copy_file(out_fp, in_files[i], &bytes_written) < 0) {
      free(entries);
      fclose(out_fp);
      return -1;
    }

    entries[i].size = bytes_written;

    const char *fname = strrchr(in_files[i], '/');
    fname = fname ? fname + 1 : in_files[i];
    memset(entries[i].name, 0, sizeof(entries[i].name));
    strncpy(entries[i].name, fname, 8);

    current_offset += bytes_written;
  }

  header.dir_offset = current_offset;

  fwrite(entries, sizeof(lump_entry_t), num_in, out_fp);

  fseek(out_fp, 0, SEEK_SET);
  fwrite(&header, sizeof(header), 1, out_fp);

  fclose(out_fp);
  free(entries);

  return 0;
}

int unpack_wad(const char *in_file) {
  FILE *wad_fp = fopen(in_file, "rb");
  if (!wad_fp) {
    fprintf(stderr, "Error opening %s", in_file);
    return -1;
  }

  wad_header_t *hdr = malloc(sizeof(wad_header_t));
  fread(hdr, sizeof(wad_header_t), 1, wad_fp);

  fseek(wad_fp, hdr->dir_offset, SEEK_SET);

  lump_entry_t *entries = calloc(hdr->num_lumps, sizeof(lump_entry_t));
  fread(entries, sizeof(lump_entry_t), hdr->num_lumps, wad_fp);

  for (uint32_t i = 0; i < hdr->num_lumps; i++) {
    fseek(wad_fp, entries[i].offset, SEEK_SET);
    uint8_t *contents = malloc(entries[i].size);

    fread(contents, entries[i].size, 1, wad_fp);

    char lump_name[9];
    memcpy(lump_name, entries[i].name, 8);
    lump_name[8] = '\0';
    FILE *out_file = fopen(lump_name, "wb");
    fwrite(contents, entries[i].size, 1, out_file);

    free(contents);
    fclose(out_file);
  }

  fclose(wad_fp);
  free(hdr);
  free(entries);

  return 0;
}

int main(int argc, const char **argv) {
  if (argc < 3) {
    fprintf(stderr,
            "Usage: %s <pack|unpack> <wad file> [IWAD|PWAD] [extra files...]\n",
            argv[0]);
    return 1;
  }

  bool pack = strcmp(argv[1], "pack") == 0;
  bool unpack = strcmp(argv[1], "unpack") == 0;

  if (!pack && !unpack) {
    fprintf(stderr, "Error: first argument must be 'pack' or 'unpack'\n");
    return 1;
  }

  const char *wad_file = argv[2];

  if (pack) {
    if (argc < 5) {
      fprintf(stderr,
              "Usage for packing: %s pack <wad file> <IWAD|PWAD> <extra "
              "files...>\n",
              argv[0]);
      return 1;
    }

    if (!strcmp(argv[3], "IWAD")) {
      w_type_arg = WAD_IWAD;
    } else if (!strcmp(argv[3], "PWAD")) {
      w_type_arg = WAD_PWAD;
    } else {
      fprintf(stderr, "Error: third argument must be 'IWAD' or 'PWAD'\n");
      return 1;
    }

    const char **source_files = &argv[4];
    int num_source_files = argc - 4;

    return pack_wad(wad_file, source_files, num_source_files);

  } else {
    return unpack_wad(wad_file);
  }
}
