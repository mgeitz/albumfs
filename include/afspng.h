#ifndef AFSPNG_H
#define AFSPNG_H

#define MAX_FILENAME 64
#define MAX_PATH 512

/* System Header Files */

#include <png.h>


/* Structures */

typedef enum {is_file, hole, created} file_state;
typedef enum {modified, not_modified} image_state;

struct PNG_image_data {
    char filename [MAX_FILENAME];
    char md5[64];
    int32_t width, height, channels;
    unsigned char png_sig[8];
    png_byte color_type;
    png_byte bit_depth;
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep *row_pointers;
    image_state state;
};
typedef struct PNG_image_data png_data;

struct file_meta_data {
    char name[MAX_FILENAME];
    int64_t offset;
    size_t size;
    file_state state;
};
typedef struct file_meta_data afs_file;

struct filesystem_meta_data {
    char name[64];
    char key[4096];
    char img_dir [MAX_PATH];
    char root_dir [MAX_PATH];
    int32_t img_count;
    int32_t file_count;
    float capacity;
    float consumed;
    png_data *root_img;
    png_data **images;
    afs_file **files;
};
typedef struct filesystem_meta_data afs_filesystem;

/* Function Prototypes */

int read_png(png_data *img, char *img_dir);
int write_png(png_data *img, char *img_dir);
void clearAllLSB(png_data *img);
void writeRoot();
void readRoot();
void afs_format();
#endif
