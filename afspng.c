/*
//   Program:             AlbumFS
//   File Name:           afspng.c
//
//   Copyright (C) 2015-2018 Michael Geitz
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License along
//   with this program; if not, write to the Free Software Foundation, Inc.,
//   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>
#include "include/afspng.h"

/* Read new png image into memory*/
int read_png(png_data *img, char *img_dir) {
    int y;
    char fullpath [PATH_MAX];

    sprintf(fullpath, "%s%s", img_dir, img->filename);

    // Open file
    FILE *fp = fopen(fullpath, "r+");
    if (!fp) {
        fprintf(stderr, "Cannot open %s\n", img->filename);
        free(img);
        return 0;
    }
    // Check if file is png (89  50  4E  47  0D  0A  1A  0A)
    fread(img->png_sig, 1, 8, fp);
    if (png_sig_cmp(img->png_sig, 0, 8)) {
        fprintf(stderr, "%s has invalid signature\n", img->filename);
        fclose(fp);
        free(img);
        return 0;
    }
    // Create and check png_struct
    img->png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!img->png_ptr) {
        fprintf(stderr, "out of memory\n");
        fclose(fp);
        free(img);
        return 0;
    }
    // Create and check png_info
    img->info_ptr = png_create_info_struct(img->png_ptr);
    if (!img->info_ptr) {
        png_destroy_read_struct(&img->png_ptr, NULL, NULL);
        fprintf(stderr, "out of memory\n");
        fclose(fp);
        free(img);
        return 0;
    }
    // Destroy struct on exception
    if (setjmp(png_jmpbuf(img->png_ptr))) {
        png_destroy_read_struct(&img->png_ptr, NULL, NULL);
        fclose(fp);
        free(img);
        return 0;
    }

    png_init_io(img->png_ptr, fp);
    png_set_sig_bytes(img->png_ptr, 8);
    png_read_info(img->png_ptr, img->info_ptr);

    img->width = png_get_image_width(img->png_ptr, img->info_ptr);
    img->height = png_get_image_height(img->png_ptr, img->info_ptr);
    img->bit_depth = png_get_bit_depth(img->png_ptr, img->info_ptr);
    img->color_type = png_get_color_type(img->png_ptr, img->info_ptr);
    img->channels = png_get_channels(img->png_ptr, img->info_ptr);
    img->state = not_modified;
    // Check if not RGB or RGBA
    if (img->color_type != PNG_COLOR_TYPE_RGB && img->color_type != PNG_COLOR_TYPE_RGB_ALPHA) {
        fprintf(stderr, "Invalid colortype; must be RGB/RGBA\n");
        free(img);
        return 0;
    }
    // If channels have 16 bit resolution, strip to 8 bits
    if (img->bit_depth == 16) { png_set_strip_16(img->png_ptr); }
    // If an alpha channel exists, remove it
    if (img->color_type & PNG_COLOR_MASK_ALPHA) { png_set_strip_alpha(img->png_ptr); }
    png_read_update_info(img->png_ptr, img->info_ptr);

    // Create and populate row pointers
    img->row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * img->height);
    for(y = 0; y < img->height; y++) {
        img->row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(img->png_ptr, img->info_ptr));
    }
    png_read_image(img->png_ptr, img->row_pointers);
    fclose(fp);
    return 1;
}


/* Write row pointers to png image */
int write_png(png_data *img, char *img_dir) {
    char fullpath [PATH_MAX];

    sprintf(fullpath, "%s%s", img_dir, img->filename);
    FILE *fp = fopen(fullpath, "wb");
    if (!fp) {
        fprintf(stderr, "Failed to write %s! Cannot open for writing\n", img->filename);
        free(img);
        return 0;
    }
    img->png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!img->png_ptr) {
        fprintf(stderr, "Failed to write %s! Out of memory\n", img->filename);
        fclose(fp);
        free(img);
        return 0;
    }
    img->info_ptr = png_create_info_struct(img->png_ptr);
    if (!img->info_ptr) {
        png_destroy_read_struct(&img->png_ptr, NULL, NULL);
        fprintf(stderr, "Failed to write %s! Out of memory\n", img->filename);
        fclose(fp);
        free(img);
        return 0;
    }
    if (setjmp(png_jmpbuf(img->png_ptr))) {
        png_destroy_read_struct(&img->png_ptr, NULL, NULL);
        fclose(fp);
        free(img);
        return 0;
    }
    png_init_io(img->png_ptr, fp);

    png_set_IHDR(
        img->png_ptr,
        img->info_ptr,
        img->width, img->height,
        8,
        PNG_COLOR_TYPE_RGB,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE,
        PNG_FILTER_TYPE_BASE
    );
    png_write_info(img->png_ptr, img->info_ptr);
    png_write_image(img->png_ptr, img->row_pointers);
    png_write_end(img->png_ptr, NULL);
    fclose(fp);
    return 1;
}
