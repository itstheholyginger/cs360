#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>


typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode INODE;
typedef struct ext2_dir_entry_2 DIR; // need this for new version of e2fs

GD *gp;
SUPER *sp;
INODE *ip;
DIR *dp;

#define BLKSIZE 1024

// globals

int fd;

int get_block(int fd, int blk, char buf[])
{
    lseek(fd, (long)blk * BLKSIZE, 0);
    read(fd, buf, BLKSIZE);
}

int gd() {
    
    char buf[BLKSIZE];

    // get super block
    get_block(fd, 1, buf);
    sp = (SUPER *)buf;
}


char *disk = "mydisk";

int main(int argc, char const *argv[])
{
    if (argc > 1) {
        disk = argv[1];
    }
    fd = open(disk, O_RDONLY);
    if (fd < 0) {
        printf("open %s failed\n", disk);
        exit(1);
    }
    gd();
    return 0;
}
