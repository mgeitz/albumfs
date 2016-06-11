/* 
//   Program:             AlbumFS
//   File Name:           afs.c
//
//   Copyright (C) 2015 Michael Geitz
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

/* Read one bit of data from 1 byte in afs */
int readBit(png_bytep row, int64_t x_offset, int8_t pixel) {
    png_bytep px = &(row[x_offset * 3]);
    return (px[pixel] & 1);
}


/* Read one byte of data from 8 bytes in afs */
char readByte(int64_t offset) {
    unsigned char byte;
    int8_t x, bit, pixel, root;
    int32_t img_row_num, img_num, row_num, x_offset;
    if (offset < 0) { 
        root = 1; 
        offset = abs(offset);
    } else { root = 0; }

    pixel = offset % 3;
    x_offset = ((int) offset / 3) % afs->root_img->width;
    row_num = (offset / 3) / afs->root_img->width;
    img_num = row_num / afs->root_img->height;
    img_row_num = row_num % afs->root_img->height;

    if (afs_dbg) { printf("# Read Byte\n"); }
    for (x = 8; x > 0; x--) {
        if (!root) { bit = readBit(afs->images[img_num]->row_pointers[img_row_num], x_offset, pixel); }
        else if (root) { bit = readBit(afs->root_img->row_pointers[img_row_num], x_offset, pixel); }
        if (afs_dbg) { 
            if (!root) { printf("%d | %d(%d, %d):%d\n", bit, img_num, x_offset, img_row_num, pixel); }
            else if (root) { printf("%d | -1(%d, %d):%d\n", bit, x_offset, img_row_num, pixel); }
        }
        if (bit) { byte |= 1; }
        else { byte &= ~1; }
        if (x != 1) { byte <<= 1; }
        if (x_offset == (afs->root_img->width - 1) && pixel == 2) {
            if ((img_row_num == (afs->root_img->height - 1)) && !root) {
                img_num++;
                img_row_num = 0;
            }
            else { img_row_num++; }
            x_offset = 0;
            if ((img_num) > afs->img_count || (root && img_row_num == (afs->root_img->height - 1))) {
                fprintf(stderr, "attempting to read out of bounds on %s in %d\n", afs->name, img_num);
                break;
            }
        }
        if (pixel == 2) { 
            pixel = 0;
            x_offset++; 
        }
        else { pixel++; }
    }
    byte ^= afs->key[offset % strlen(afs->key)];
    return byte;
}


/* Read n bytes of data from n*8 bytes in afs */
int readBytes(char *buf, size_t size, off_t offset) {
    int i;

    if ((((offset + size) > afs->capacity) && (offset > 0)) || ((offset < 0) && ((abs(offset) + size) > ceil((afs->root_img->height * afs->root_img->width * 3) / 8)))) {
        fprintf(stderr, "Failed to read, filesystem %s out of bounds", afs->name);
        return -1;
    }
    for (i = 0; i < size; i++) {
        buf[i] = readByte(offset * 8);
        if (offset >= 0) { offset++; }
        else { offset--; }
    }
    return i;
}


/* Write one bit of data to 1 byte in afs */
void writeBit(png_bytep row, int64_t x_offset, int8_t pixel, int8_t bit) {
    png_bytep px = &(row[x_offset * 3]);
    if (bit) { px[pixel] |= 1; }
    else { px[pixel] &= ~1; }
}


/* Write one byte of data to 8 bytes in afs */
void writeByte(char *b, int64_t offset) {
    int root = 0;
    int8_t pixel, y, x, bit[8];
    int32_t img_row_num, img_num, row_num, x_offset;
    char byte;

    if (offset < 0) {
        root = 1; 
        offset = abs(offset);
    } else { root = 0; }
    strncpy(&byte, b, sizeof(char));
    byte ^= afs->key[offset % strlen(afs->key)];

    pixel = offset % 3;
    x_offset = ((int) offset / 3) % afs->root_img->width;
    row_num = (offset / 3) / afs->root_img->width;
    img_num = row_num / afs->root_img->height;
    img_row_num = row_num % afs->root_img->height;

    if (afs->images[img_num]->state != modified) { afs->images[img_num]->state = modified; }
    for (y = 0; y < 8; y++) {
        bit[y] = (byte & 1);
        byte >>= 1;
    }
    if (afs_dbg) { printf("# Write byte\n"); }
    for (x = 8; x > 0; x--) {
        if (afs_dbg) { 
            if (!root) { printf("%d | %d(%d, %d(%d)):%d\n", bit[x - 1], img_num, x_offset, img_row_num, row_num, pixel); }
            else if (root) { printf("%d | -1(%d, %d(%d)):%d\n", bit[x - 1], x_offset, img_row_num, row_num, pixel); }
        }
        if (!root) { writeBit(afs->images[img_num]->row_pointers[img_row_num], x_offset, pixel, bit[x - 1]); }
        else if (root) { writeBit(afs->root_img->row_pointers[img_row_num], x_offset, pixel, bit[x - 1]); }
        if (x_offset == (afs->root_img->width - 1) && pixel == 2) {
            if ((img_row_num == (afs->root_img->height - 1)) && !root) {
                img_num++;
                img_row_num = 0;
            }
            else { img_row_num++; }
            x_offset = 0;
            if ((img_num) > afs->img_count || (root && img_row_num == (afs->root_img->height - 1))) {
                fprintf(stderr, "attempting to read out of bounds on %s in %d\n", afs->name, img_num);
                return;
            }
        }
        if (pixel == 2) { 
            pixel = 0;
            x_offset++; 
        }
        else { pixel++; }
    }
    return;
}


/* Write n bytes of data to n*8 bytes in afs */
int writeBytes(char *buf, size_t size, off_t offset) {
    int i;

    if ((((offset + size) > afs->capacity) && (offset > 0)) || ((offset < 0) && ((abs(offset) + size) > ceil((afs->root_img->height * afs->root_img->width * 3) / 8)))) {
        fprintf(stderr, "Failed to write, attempting to write beyond memory!\no: %d s: %d\n", (int)offset, (int)size);
        return -1;
    }
    for (i = 0; i < size; i++) {
        writeByte(&buf[i], (offset * 8));
        if ((int)offset >= 0) { 
            offset = offset + 1;; 
            afs->consumed++;;
        }
        else { offset = offset - 1; }
    }
    return i;
}


/* Wipe one bit of data from 1 byte in afs */
void wipeBit(png_bytep row, int64_t x_offset, int8_t px_chan) {
    png_bytep px = &(row[x_offset * 3]);
    px[px_chan] &= ~1;
}


/* Wipe one byte of data to 8 bytes in afs */
void wipeByte(int64_t offset) {
    int root = 0;
    int8_t pixel, x;
    int32_t img_row_num, img_num, row_num, x_offset;

    if (offset < 0) {
        root = 1;
        offset = abs(offset);
    } else { root = 0; }

    pixel = offset % 3;
    x_offset = ((int) offset / 3) % afs->root_img->width;
    row_num = (offset / 3) / afs->root_img->width;
    img_num = row_num / afs->root_img->height;
    img_row_num = row_num % afs->root_img->height;

    if (afs->images[img_num]->state != modified) { afs->images[img_num]->state = modified; }
    if (afs_dbg) { printf("# Wipe byte\n"); }
    for (x = 8; x > 0; x--) {
        if (afs_dbg) {
            if (!root) { printf("0 | %d(%d, %d(%d)):%d\n", img_num, x_offset, img_row_num, row_num, pixel); }
            else if (root) { printf("0 | -1(%d, %d(%d)):%d\n", x_offset, img_row_num, row_num, pixel); }
        }
        if (!root) { wipeBit(afs->images[img_num]->row_pointers[img_row_num], x_offset, pixel); }
        else if (root) { wipeBit(afs->root_img->row_pointers[img_row_num], x_offset, pixel); }
        if (x_offset == (afs->root_img->width - 1) && pixel == 2) {
            if ((img_row_num == (afs->root_img->height - 1)) && !root) {
                img_num++;
                img_row_num = 0;
            }
            else { img_row_num++; }
            x_offset = 0;
            if ((img_num) > afs->img_count || (root && img_row_num == (afs->root_img->height - 1))) {
                fprintf(stderr, "attempting to read out of bounds on %s in %d\n", afs->name, img_num);
                return;
            }
        }
        if (pixel == 2) {
            pixel = 0;
            x_offset++;
        }
        else { pixel++; }
    }
}


/* Wipe n bytes of data */
int wipeBytes(afs_file *file) {
    int i;
    off_t offset = file->offset;

    for (i = 0; i < file->size + 1; i++) {
        wipeByte(offset * 8);
        afs->consumed--;
        offset++;
    }
    return i;
}


/* Create File */
int createFile(char *path) {
    int i;

    afs_file **files = malloc(sizeof(afs_file) * (afs->file_count + 1));
    for (i = 0; i < (afs->file_count); i++) { files[i] = afs->files[i]; }
    free(afs->files);
    afs->files = files;
    afs_file *newfile = malloc(sizeof(afs_file));
    strcpy(newfile->name, path);
    newfile->size = 0;
    newfile->state = created;
    if (afs->file_count == 0) { newfile->offset = 0; }
    else {
        newfile->offset = (afs->files[afs->file_count - 1]->offset + afs->files[afs->file_count - 1]->size + 1);
    }
    afs->files[afs->file_count] = newfile;
    afs->file_count++;
    return 0;
}


/* Write file */
int writeFile(char *path, char *buf, size_t size, off_t offset) {
    int result = 0;
    int f = findFile(path);

    if (size > (int)(afs->capacity - afs->consumed)) { return -ENOSPC; }
    if (f == -1) { return -ENOENT; }
    afs_file *file = afs->files[f];

    if (file->state == created && (f == (afs->file_count - 1))) {
        file->size = size;
        file->state = is_file;
        result = writeBytes(buf, file->size, file->offset);
    }
    else if (file->state == is_file && (f == (afs->file_count - 1))) {
        if (file->size < size + offset) {
            result = writeBytes(buf, size, file->offset + file->size);
            file->size = file->size + size;
        }
        else { result = writeBytes(buf, size, offset); }
    }
    else {
        fprintf(stderr, "Attempting to write to invalid file %s\n", afs->files[f]->name);
        result = 0;
    }
    return result;
}


/* Read file */
int readFile(char *path, char *buf, size_t size, off_t offset) {
    int result = 0;
    int f = findFile(path);

    if (f == -1) { return -ENOENT; }
    afs_file *file = afs->files[f];
    if (file->size > offset) {
        result = readBytes(buf, size, file->offset + offset);
    }
    else { return 0; }
    return result;
}


/* Wipe file */
int wipeFile(char *path) {
    int result = 0;
    int f = findFile(path);
    int x;

    if (f == -1) { return -ENOENT; }
    afs_file *deleted_file = afs->files[f];

    result += wipeBytes(deleted_file);
    if (f != (afs->file_count - 1)) { 
        afs_file **new_files = malloc(sizeof(afs_file) * afs->file_count - 1);
        for (x = 0; x < f; x++) { new_files[x] = afs->files[x]; }
        for (x = 0; x < afs->file_count - f - 1; x++) {
            char *tmp = calloc(1, afs->files[(f + x + 1)]->size);
            afs_file *new_file = malloc(sizeof(afs_file));
            readBytes(tmp, afs->files[(f + x + 1)]->size, afs->files[(f + x + 1)]->offset);
            result += wipeBytes(afs->files[(f + x + 1)]);
            if (x == 0) { new_file->offset = afs->files[(f + x)]->offset; }
            else { new_file->offset = new_files[(f + x - 1)]->offset + new_files[(f + x - 1)]->size + 1; }
            strcpy(new_file->name, afs->files[(f + x + 1)]->name);
            new_file->size = afs->files[(f + x + 1)]->size;
            writeBytes(tmp, new_file->size, new_file->offset);
            new_files[f + x] = new_file;
            free(afs->files[(f + x + 1)]);
            free(tmp);
        }
        free(afs->files);
        afs->files = new_files;
    }
    else {
        free(afs->files[f]);
    }
    afs->file_count--;
    return result;
}


/* Find a files meta data from its path */
int findFile(char *path) {
    int y;

    for (y = 0; y < afs->file_count; y++) {
        if (strcmp(afs->files[y]->name, path) == 0) { return y; }
    }
    return -1;
}


/* Expand existing filesystem */
void afs_expand() {
    DIR *FD;
    struct dirent *dir;
    int x, y, exists;
    int added = 0;

    readRoot();
    FD = opendir(afs->img_dir);
    if (!FD) {
        fprintf(stderr, "Cannot open directory %s", afs->img_dir);
        free(afs);
        exit(1);
    }
    while ((dir = readdir(FD)) != NULL) {
        if (strncmp(dir->d_name + strlen(dir->d_name) - 4, ".png", 4) == 0) {
            exists = 0;
            for (x = 0; x < afs->img_count; x++) {
                if ((strcmp(dir->d_name, afs->images[x]->filename) == 0) || (strcmp(afs->root_img->filename, dir->d_name) == 0)) { exists = 1; }
            }
            if (!exists) {
                png_data **new_images = malloc(sizeof(png_data) * afs->img_count + 1);
                for (y = 0; y < afs->img_count; y++) { new_images[y] = afs->images[y]; }
                png_data *new_img = malloc(sizeof(png_data));
                strcpy(new_img->filename, dir->d_name);
                if (!read_png(new_img, afs->img_dir)) {
                    free(new_img);
                    free(new_images);
                }
                else if (new_img->height != afs->root_img->height || new_img->width != afs->root_img->width) {
                    if (afs_dbg) { printf("%s not same size as root image\n", dir->d_name); }
                    free(new_img);
                    free(new_images);
                }
                else {
                    clearAllLSB(new_img);
                    afs->capacity += ((new_img->width * new_img->height * 3) / 8);
                    new_images[afs->img_count++] = new_img;
                    free(afs->images);
                    afs->images = new_images;
                    added++;
                }
            }
        }
        afs_file **files = malloc(sizeof(afs_file*) * afs->file_count);
        afs->files = files;
    }
    if (added) {
        printf("%d images added to %s\n", added, afs->name);
        printf("#### Updated Filesystem ####\n");
        printf("Name       : %s\n", afs->name);
        printf("PNG Count  : %d\n", afs->img_count);
        printf("Capacity   : %.2f MB\n", ((afs->capacity / 1000) / 1000));
        printf("Consumed   : %.2f MB\n", ((afs->consumed / 1000) / 1000));
    }
    else { printf("No new valid images found, filesystem not expanded.\n"); }
}


/* Format new filesystem */
void afs_format() {
    DIR *FD;
    struct dirent *dir;
    int png_cnt, valid_png_cnt, y;

    FD = opendir(afs->img_dir);
    if (!FD) {
        fprintf(stderr, "Cannot open directory %s", afs->img_dir);
        free(afs);
        exit(1);
    }
    png_cnt = 0;
    while ((dir = readdir(FD)) != NULL) {
        if (strncmp(dir->d_name + strlen(dir->d_name) - 4, ".png", 4) == 0) {
            png_cnt++;
        }
    }
    seekdir(FD, 0);
    png_data **dir_images = malloc(sizeof(png_data*) * png_cnt);
    valid_png_cnt = 0;
    while ((dir = readdir(FD)) != NULL) {
        if (strncmp(dir->d_name + strlen(dir->d_name) - 4, ".png", 4) == 0) {
            png_data *new_img = malloc(sizeof(png_data));
            strcpy(new_img->filename, dir->d_name);
            if (!read_png(new_img, afs->img_dir)) {
                printf("%s not valid read\n", dir->d_name);
                free(new_img);
            }
            else if (new_img->height != afs->root_img->height || new_img->width != afs->root_img->width) {
                if (afs_dbg) { printf("%s not same size as root image\n", dir->d_name); }
                free(new_img);
            }
            else if (strcmp(afs->root_img->filename, new_img->filename) == 0) {
                free(new_img);
            }
            else {
                clearAllLSB(new_img);
                dir_images[valid_png_cnt] = new_img;
                valid_png_cnt++;
            }
        }
    }
    closedir(FD);
    if (valid_png_cnt < MINIMUM_PNG) {
        fprintf(stderr, "Not enough images in %s to create %s. At least %d valid storage PNG images required.\n", afs->img_dir, afs->name, MINIMUM_PNG);
        if (valid_png_cnt) {
        fprintf(stderr, "Current valid images:\n");
            for (y = 0; y < valid_png_cnt; y++) {
                fprintf(stderr, "%s\n", dir_images[y]->filename);
            }
        }
        else { fprintf(stderr, "No other valid images found.\n"); }
        exit(1);
    }
    afs->img_count = valid_png_cnt;
    afs->images = dir_images;
    afs->capacity = ceil(((valid_png_cnt * afs->root_img->height * afs->root_img->width * 3) / 8));
    afs->consumed = 0;
    printf("#### New Filesystem ####\n");
    printf("Name       : %s\n", afs->name);
    printf("PNG Count  : %d\n", afs->img_count);
    printf("Capacity   : %.2f MB\n", ((afs->capacity / 1000) / 1000));
    printf("Consumed   : %.2f MB\n", ((afs->consumed / 1000) / 1000));
    for (y = 0; y < afs->img_count; y++) {
        if (afs_dbg) { printf("%d: %s\n", y, afs->images[y]->filename); } 
    }
}


/* Write some filesystem_meta and png_data to root img */
void writeRoot() {
    int y;
    int offset = -1;

    // Write filesystem name, consumed, capacity and image count
    clearAllLSB(afs->root_img);
    writeBytes((void *) afs->name, sizeof(afs->name), offset);
    offset = offset - sizeof(afs->name);
    writeBytes((void *) &afs->consumed, sizeof(float), offset);
    offset = offset - sizeof(float);
    writeBytes((void *) &afs->capacity, sizeof(float), offset);
    offset = offset - sizeof(float);
    writeBytes((void *) &afs->img_count, sizeof(int32_t), offset);
    offset = offset - sizeof(int32_t);
    writeBytes((void *) &afs->file_count, sizeof(int32_t), offset);
    offset = offset - sizeof(int32_t);
    
    
    // Write png_data for each valid image
    for (y = 0; y < afs->img_count; y++) {
        getMD5(afs->images[y]->filename, afs->images[y]->md5);
        writeBytes((void *) afs->images[y]->md5, sizeof(afs->images[y]->md5), offset);
        offset = offset -sizeof(afs->images[y]->md5);
    }

    // Write file_meta for each file
    for (y = 0; y < afs->file_count; y++) {
        writeBytes((void *) afs->files[y], sizeof(afs_file), offset);
        offset = offset - sizeof(afs_file);
    }
    write_png(afs->root_img, afs->img_dir);
}


/* Read filesystem_meta and png_data from root img */
void readRoot() {
    DIR *FD;
    struct dirent *dir;
    int y;
    int offset = -1;
    char name[64];

    // Read filesystem name, consumed, capacity, and image count
    readBytes((void *) name, sizeof(afs->name), offset);
    offset = offset - sizeof(afs->name);
    if (strncmp(name, afs->name, (strlen(afs->name))) != 0) {
        fprintf(stderr, "Cannot read filesystem! Key, name, or root image supplied is incorrect.\n");
        exit(1);
    }
    readBytes((void *) &afs->consumed, sizeof(float), offset);
    offset = offset - sizeof(float);
    readBytes((void *) &afs->capacity, sizeof(float), offset);
    offset = offset - sizeof(float);
    readBytes((void *) &afs->img_count, sizeof(int32_t), offset);
    offset = offset - sizeof(int32_t);
    readBytes((void *) &afs->file_count, sizeof(int32_t), offset);
    offset = offset - sizeof(int32_t);
    printf("Found filesystem %s [%0.2f/%0.2f] %d files in %d images\n", name, afs->consumed, afs->capacity, afs->file_count, afs->img_count);

    png_data **dir_images = malloc((sizeof(png_data*) * afs->img_count));
    FD = opendir(afs->img_dir);
    if (!FD) {
        fprintf(stderr, "Cannot open directory %s", afs->img_dir);
        free(afs);
        exit(1);
    }

    // Read png_data for each valid image
    for (y = 0; y < afs->img_count; y++) {
        png_data *new_img = malloc(sizeof(png_data));
        char tmp[strlen(new_img->md5)];
        readBytes((void *) new_img->md5, sizeof(new_img->md5), offset);
        offset = offset - sizeof(new_img->md5);
        while ((dir = readdir(FD)) != NULL) {
            memset(tmp, 0, sizeof(new_img->md5));
            getMD5(dir->d_name, tmp);;
            if (strncmp(tmp, new_img->md5, sizeof(new_img->md5)) == 0) {
                strcpy(new_img->filename, dir->d_name);
                if (!read_png(new_img, afs->img_dir)) {
                    fprintf(stderr, "Filesystem is missing image %s!", new_img->filename);
                    exit(1);
                }
                dir_images[y] = new_img;
                break;
            }
        }
        seekdir(FD, 0);
    }
    closedir(FD);
    afs->images = dir_images;

    afs_file **files = malloc((sizeof(afs_file*) * afs->file_count));

    // Read file_meta for each file
    for (y = 0; y < afs->file_count; y++) {
        afs_file *new_file = malloc(sizeof(afs_file));
        readBytes((void *) new_file, sizeof(afs_file), offset);
        offset = offset - sizeof(afs_file);
        files[y] = new_file;
    }
    afs->files = files;
    if (afs_dbg) { printf("Filesystem %s loaded\n", afs->name); }
}


/* Clear LSB in each R, G, B */
void clearAllLSB(png_data *img) {
    int cnty, cntx, px_chan;
    for(cnty = 0; cnty < img->height; cnty++) {
        png_bytep row = img->row_pointers[cnty];
        for(cntx = 0; cntx < img->width; cntx++) {
            png_bytep px = &(row[cntx * 3]);
            for(px_chan = 0; px_chan < 3; px_chan++) {
                px[px_chan] &= ~1;
            }
        }
    }
    img->state = modified;
}


/* Write state to modified images and exit */
void saveState() {
    int i;
    for (i = 0; i < afs->img_count; i++) {
        if (afs->images[i]->state == modified) {
            write_png(afs->images[i], afs->img_dir);
            afs->images[i]->state = not_modified;
        }
    }
    writeRoot();
    rmdir(afs->name);
    printf("\nSafely unmounted %s, goodbye!\n", afs->name);
}
