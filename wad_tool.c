#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "fs.h"

int dump_wad(char *path) {
    const char *wad_file = path;
    FILE *file = fopen(wad_file, "rb");
    if (!file) {
        perror("Failed to open WAD file");
        return 1;
    }

    wad_header_t *wad = malloc(sizeof(wad_header_t));
    if (fread(wad, sizeof(wad_header_t), 1, file) != 1) {
        perror("Failed to read WAD header");
        fclose(file);
        free(wad);
        return 1;
    }

    if (strncmp(wad->identifier, "IWAD", 4) != 0 && strncmp(wad->identifier, "PWAD", 4) != 0) {
        printf("Invalid WAD file identifier: %.4s\n", wad->identifier);
        fclose(file);
        free(wad);
        return 1;
    }

    printf("WAD Identifier: %.4s\n", wad->identifier);
    printf("Number of Lumps: %u\n", wad->num_lumps);

    for(uint32_t i = 0; i < wad->num_lumps; i++) {
        lump_entry_t lump;
        fseek(file, wad->dir_offset + i * sizeof(lump_entry_t), SEEK_SET);
        if (fread(&lump, sizeof(lump_entry_t), 1, file) != 1) {
            perror("Failed to read lump entry");
            fclose(file);
            free(wad);
            return 1;
        }
        printf("Lump %u: Name: %.8s, Offset: %u, Size: %u\n", i, lump.name, lump.offset, lump.size);
    }

    return 0;
}

int pack_wad(char *out_path, char **input_files, int file_count) {
    // make a new wad file
    const char *wad_file = out_path;
    FILE *file = fopen(wad_file, "wb");
    if (!file) {
        perror("Failed to create WAD file");
        return 1;
    }

    wad_header_t wad;
    memcpy(wad.identifier, "IWAD", 4);
    wad.num_lumps = file_count;
    wad.dir_offset = sizeof(wad_header_t) + 0; // will update later
    fwrite(&wad, sizeof(wad_header_t), 1, file);

    lump_entry_t *lumps = malloc(sizeof(lump_entry_t) * file_count);
    uint32_t current_offset = sizeof(wad_header_t);

    for (int i = 0; i < file_count; i++) {
        const char *input_file = input_files[i];
        FILE *in = fopen(input_file, "rb");
        if (!in) {
            perror("Failed to open input file");
            fclose(file);
            free(lumps);
            return 1;
        }

        fseek(in, 0, SEEK_END);
        uint32_t size = ftell(in);
        fseek(in, 0, SEEK_SET);

        lumps[i].offset = current_offset;
        lumps[i].size = size;
        strncpy(lumps[i].name, input_file, 8);
        lumps[i].name[7] = '\0'; // ensure null termination

        uint8_t *buffer = malloc(size);
        fread(buffer, size, 1, in);
        fwrite(buffer, size, 1, file);
        free(buffer);
        fclose(in);

        current_offset += size;
    }

    wad.dir_offset = current_offset;
    fseek(file, 0, SEEK_SET);
    fwrite(&wad, sizeof(wad_header_t), 1, file);
    fseek(file, current_offset, SEEK_SET);
    fwrite(lumps, sizeof(lump_entry_t), file_count, file);
    free(lumps);
    fclose(file);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <pack|dump> <path_to_wad>\n", argv[0]);
        return 1;
    }
    
    if (strcmp(argv[1], "dump") == 0) {
        return dump_wad(argv[2]);
    } else if (strcmp(argv[1], "pack") == 0) {
        return pack_wad(argv[2], &argv[3], argc - 3);
    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        return 1;
    }
}