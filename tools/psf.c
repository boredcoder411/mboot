#include <stdio.h>
#include <stdlib.h>
#include <png.h>

#define GLYPH_WIDTH 8
#define GLYPH_HEIGHT 8
#define GLYPHS_X 16
#define GLYPHS_Y 8
#define NUM_GLYPHS (GLYPHS_X * GLYPHS_Y)

unsigned char psf1_header[4] = {0x36, 0x04, 0x00, GLYPH_HEIGHT};

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s input.png output.psf\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) return 1;
    png_infop info = png_create_info_struct(png);
    if (!info) return 1;
    if (setjmp(png_jmpbuf(png))) return 1;

    png_init_io(png, fp);
    png_read_info(png, info);

    int width = png_get_image_width(png, info);
    int height = png_get_image_height(png, info);
    int color_type = png_get_color_type(png, info);
    int bit_depth = png_get_bit_depth(png, info);

    if (bit_depth == 16) png_set_strip_16(png);
    if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png);
    if (png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);
    if (color_type == PNG_COLOR_TYPE_RGB ||
        color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);

    png_read_update_info(png, info);

    png_bytep *rows = malloc(sizeof(png_bytep) * height);
    for (int y = 0; y < height; y++) {
        rows[y] = malloc(png_get_rowbytes(png, info));
    }
    png_read_image(png, rows);
    fclose(fp);

    FILE *out = fopen(argv[2], "wb");
    if (!out) {
        perror("fopen output");
        return 1;
    }

    fwrite(psf1_header, 1, 4, out);

    for (int gy = 0; gy < GLYPHS_Y; gy++) {
        for (int gx = 0; gx < GLYPHS_X; gx++) {
            for (int y = 0; y < GLYPH_HEIGHT; y++) {
                unsigned char byte = 0;
                for (int x = 0; x < GLYPH_WIDTH; x++) {
                    int px = gx * GLYPH_WIDTH + x;
                    int py = gy * GLYPH_HEIGHT + y;
                    png_bytep pixel = &rows[py][px * 4];
                    int brightness = (pixel[0] + pixel[1] + pixel[2]) / 3;
                    int bit = brightness > 128 ? 1 : 0;
                    byte |= bit << (7 - x);
                }
                fwrite(&byte, 1, 1, out);
            }
        }
    }

    fclose(out);
    for (int y = 0; y < height; y++) free(rows[y]);
    free(rows);
    png_destroy_read_struct(&png, &info, NULL);

    printf("PSF font written to %s\n", argv[2]);
    return 0;
}

