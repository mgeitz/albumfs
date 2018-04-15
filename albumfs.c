/*
//   Program:             AlbumFS
//   File Name:           albumfs.c
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

#include "include/albumfs.h"
#include "include/afspng.h"

afs_filesystem *afs;

#include "afs.c"
#include "afsfuse.c"

int main(int argc, char *argv[]) {
    char actualpath [MAX_PATH];
    char* base;
    int8_t fargc = 6;
    const char *fargv[fargc];
    char key_input[4096];

    // Check minimum argc and root
    if ((getuid() == 0) || (geteuid() == 0)) {
        fprintf(stderr, "Will not run as root!!\n");
        exit(1);
    }
    if (argc < 3) { afs_usage(); }

    // Check help/debug options
    afs_dbg = 0;
    if(parseArgv(argc, argv, DEBUG_OPTION)) { afs_dbg = 1; }
    if(parseArgv(argc, argv, HELP_OPTION)) { afs_usage(); }

    // Get absolute path, img_dir path, and basename
    realpath(argv[argc - 1], actualpath);
    base = basename(actualpath);
    afs = malloc(sizeof(afs_filesystem));
    afs->root_img = malloc(sizeof(png_data));
    strncpy(afs->img_dir, actualpath, strlen(actualpath) - strlen(base));
    strcpy(afs->root_img->filename, base);

    // Read root image
    if (!read_png(afs->root_img, afs->img_dir)) { afs_usage(); }

    // Drive name
    printf("Enter drive name:\n");
    fgets(afs->name, sizeof(afs->name), stdin);
    afs->name[strlen(afs->name) - 1] = '\0';
    strcat(afs->name, ".afs");

    // Key
    printf("Enter encryption key for %s:\n", afs->name);
    fgets(key_input, sizeof(key_input), stdin);
    SHA512_CTX sha512;
    SHA512_Init(&sha512);
    SHA512_Update(&sha512, key_input, strlen(key_input));
    SHA512_Final((unsigned char *)afs->key, &sha512);
    printf("\e[1;1H\e[2J");

    // Check mount
    if (parseArgv(argc, argv, MNT_OPTION)) { readRoot(); }
    // Check format
    else if (parseArgv(argc, argv, FORMAT_OPTION)) { afs_format(); }
    // Check expand
    else if (parseArgv(argc, argv, EXPAND_OPTION)) { afs_expand(); }
    else { afs_usage(); }

    // Fuse
    fargv[0] = argv[0];
    fargv[1] = "-f";
    fargv[2] = "-o";
    fargv[3] = "big_writes";
    fargv[4] = "-s";
    fargv[5] = afs->name;
    mkdir(afs->name, 0755);
    printf("\n%s mounted.\nAlbumFS will continue to run in the foreground.\nUse Ctrl+C to safely unmount the filesystem.\n", afs->name);
    return fuse_main(fargc, (char **)fargv, &afs_oper, NULL);
}


/* Print help and exit */
void afs_usage() {
    fprintf(stderr, "albumfs [-help] [-debug] [-expand] [-format] [-mount] <path/to/root.png>\n");
    exit(1);
}


/* Parse options passed for particular option and return its position in argv */
int parseArgv(int argc, char *argv[], char *option) {
    int8_t length = strlen(option);
    int8_t y;
    for(y = 0; y < argc; y++) {
        if(strncmp(argv[y], option, length) == 0) { return y; }
    }
    return 0;
}


/* Calculate MD5 of a file */
int getMD5(char *filename, char *md5_sum) {
    char path[MAX_PATH];
    strcpy(path, afs->img_dir);
    strcat(path, filename);
    FILE *f = fopen(path, "rb");
    char data[sizeof(md5_sum)];

    if (f == NULL) { return 0; }
    MD5_CTX mdContext;
    MD5_Init (&mdContext);
	while (fread (data, 1, sizeof(md5_sum), f) != 0) {
            MD5_Update (&mdContext, data, sizeof(md5_sum));
        }
    MD5_Final ((unsigned char *)md5_sum, &mdContext);
    pclose(f);
    return 1;
}
