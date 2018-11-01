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

int block_size, inode_table, inode_size, first_block, inodes_per_block;
int inode_bitmap, block_bitmap, block_offset, inode_count, inode_offset;

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

    // get gd block
    get_block(fd, 2, buf);
    gp = (GD *)buf;


    // check EXT 2 FS magic number
    printf("\n\n********** group descriptor **********\n");
    printf("s_inodes_count \t\t=\t%d\n", gp->bg_block_bitmap);
    block_bitmap = gp->bg_block_bitmap;
    printf("bg_inode_bitmap \t=\t%d\n", gp->bg_inode_bitmap);
    inode_bitmap = gp->bg_inode_bitmap;
    printf("bg_inode_table \t\t=\t%d\n", gp->bg_inode_table);
    inode_table = gp->bg_inode_table;
    printf("bg_free_blocks_count \t=\t%d\n", gp->bg_free_blocks_count);
    printf("bg_free_inodes_count \t=\t%d\n", gp->bg_free_inodes_count);
    printf("bg_used_dirs_count \t=\t%d\n", gp->bg_used_dirs_count);
    putchar('\n');
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
