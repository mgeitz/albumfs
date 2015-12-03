/* 
//   Program:             AlbumFS
//   File Name:           albumfs.c
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

    // Check minimum argc and root
    if ((getuid() == 0) || (geteuid() == 0)) {
        fprintf(stderr, "May not run as root, terrible things could happen\n");
        afs_usage();
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

    // Drive name and key
    printf("Enter drive name:\n");
    fgets(afs->name, sizeof(afs->name), stdin);
    afs->name[strlen(afs->name) - 1] = '\0';
    strcat(afs->name, ".afs");
    printf("Enter encryption key for %s:\n", afs->name);
    fgets(afs->key, sizeof(afs->key), stdin);
    afs->key[strlen(afs->key) - 1] = '\0';
    printf("\e[1;1H\e[2J");

    // Check mount
    if (parseArgv(argc, argv, MNT_OPTION)) { readRoot(); }
    // Check format
    else if (parseArgv(argc, argv, FORMAT_OPTION)) {
        afs_format();
        afs_file **files = malloc(sizeof(afs_file*) * afs->file_count);
        afs->files = files;
    }
    // Check expand
    else if (parseArgv(argc, argv, EXPAND_OPTION)) {
        readRoot();
        afs_expand();
    }
    else { afs_usage(); }

    fargv[0] = argv[0];
    fargv[1] = "-f";
    fargv[2] = "-o";
    fargv[3] = "big_writes";
    fargv[4] = "-s";
    fargv[5] = afs->name;

    mkdir(afs->name, 0755);
    printf("\n%s mounted.\nAlbumFS will continue to run in the foreground.\nUse Ctrl+C to safely unmount the filesystem.\n", afs->name);
    return fuse_main(fargc, (char **)fargv, &afs_oper, NULL);
    saveState();
    exit(0);
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
