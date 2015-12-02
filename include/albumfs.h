#ifndef ALBUMFS_H
#define ALBUMFS_H

#define FUSE_USE_VERSION 30


/* System Header Files */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <fuse.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <libgen.h>
#include <fcntl.h>
#include <math.h>
#include <sys/types.h>


/* Preprocessor Macros */

#define _GNU_SOURCE
#define VERSION "0.1.2"
#define MAX_FILENAME 64
#define MAX_PATH 512
#define MINIMUM_PNG 2
#define HELP_OPTION "-help"
#define DEBUG_OPTION "-debug"
#define FORMAT_OPTION "-format"
#define EXPAND_OPTION "-expand"
#define MNT_OPTION "-mount"


extern int8_t afs_dbg;
int8_t afs_dbg;


/* Function Prototypes */

//albumfs.c
void afs_usage();
int parseArgv(int argc, char *argv[], char *option);
//afs.c
int wipeFile(char *path);
int findFile(char *path);
int createFile(char *path);
void afs_expand();
void afs_format();
void writeRoot();
void readRoot();
void saveState();

#endif
