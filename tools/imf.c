#include "imf.h"
#include <math.h>
#include <png.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static void vga_palette(uint8_t palette[256][3]) {
  int i = 0;
  for (int r = 0; r < 6; r++) {
    for (int g = 0; g < 6; g++) {
      for (int b = 0; b < 6; b++) {
        palette[i][0] = r * 51;
        palette[i][1] = g * 51;
        palette[i][2] = b * 51;
        i++;
      }
    }
  }

  for (int g = 0; i < 256; i++, g++) {
    uint8_t v = (g * 255) / 17;
    palette[i][0] = palette[i][1] = palette[i][2] = v;
  }
}

uint8_t find_nearest_vga(uint8_t r, uint8_t g, uint8_t b,
                         uint8_t palette[256][3]) {
  int best = 0;
  int best_dist = 999999;
  for (int i = 0; i < 256; i++) {
    int dr = r - palette[i][0];
    int dg = g - palette[i][1];
    int db = b - palette[i][2];
    int dist = dr * dr + dg * dg + db * db;
    if (dist < best_dist) {
      best_dist = dist;
      best = i;
    }
  }
  return (uint8_t)best;
}

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s input.png output.imf\n", argv[0]);
    return 1;
  }

  FILE *fp = fopen(argv[1], "rb");
  if (!fp) {
    perror("fopen");
    return 1;
  }

  uint8_t header[8];
  fread(header, 1, 8, fp);
  if (png_sig_cmp(header, 0, 8)) {
    fprintf(stderr, "Not a PNG file\n");
    fclose(fp);
    return 1;
  }

  png_structp png =
      png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  png_infop info = png_create_info_struct(png);
  if (setjmp(png_jmpbuf(png))) {
    fprintf(stderr, "libpng error\n");
    fclose(fp);
    png_destroy_read_struct(&png, &info, NULL);
    return 1;
  }

  png_init_io(png, fp);
  png_set_sig_bytes(png, 8);
  png_read_info(png, info);

  int width = png_get_image_width(png, info);
  int height = png_get_image_height(png, info);
  int color_type = png_get_color_type(png, info);
  int bit_depth = png_get_bit_depth(png, info);

  if (width > MAX_X || height > MAX_Y) {
    fprintf(stderr, "Image too large: %dx%d (max %dx%d)\n", width, height,
            MAX_X, MAX_Y);
    return 1;
  }

  if (bit_depth == 16)
    png_set_strip_16(png);
  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png);
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand_gray_1_2_4_to_8(png);
  if (png_get_valid(png, info, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(png);
  if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
  if (color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png);

  png_read_update_info(png, info);

  png_bytep *row_pointers = malloc(sizeof(png_bytep) * height);
  for (int y = 0; y < height; y++)
    row_pointers[y] = malloc(png_get_rowbytes(png, info));

  png_read_image(png, row_pointers);
  fclose(fp);

  uint8_t palette[256][3];
  vga_palette(palette);

  size_t imf_size = sizeof(imf_t) + width * height * sizeof(uint8_t);
  imf_t *imf = malloc(imf_size);
  imf->x = width;
  imf->y = height;

  uint8_t *dst = imf->colors;
  for (int y = 0; y < height; y++) {
    png_bytep row = row_pointers[y];
    for (int x = 0; x < width; x++) {
      uint8_t r = row[x * 4 + 0];
      uint8_t g = row[x * 4 + 1];
      uint8_t b = row[x * 4 + 2];
      *dst++ = find_nearest_vga(r, g, b, palette);
    }
  }

  FILE *out = fopen(argv[2], "wb");
  if (!out) {
    perror("fopen output");
    return 1;
  }
  fwrite(imf, imf_size, 1, out);
  fclose(out);

  for (int y = 0; y < height; y++)
    free(row_pointers[y]);
  free(row_pointers);
  png_destroy_read_struct(&png, &info, NULL);
  free(imf);

  printf("Converted %s → %s (%dx%d)\n", argv[1], argv[2], width, height);
  return 0;
}
