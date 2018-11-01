#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>

#define BLKSIZE 1024

typedef struct ext2_group_desc GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode INODE;
typedef struct ext2_dir_entry_2 DIR;

GD *gp;
SUPER *sp;
INODE *ip;
DIR *dp;

int fd;
int iblock;

int get_block(int fd, int blk, char buf[ ]) {
    lseek(fd, (long)blk * BLKSIZE, 0);
    read(fd, buf, BLKSIZE);
}
int inode()
{
    char buf[BLKSIZE];

    get_block(fd, 2, buf);
    gp = (GD *)buf;

    iblock = gp->bg_inode_table;
    printf("inode_block=%d\n", iblock);

    get_block(fd, iblock, buf);
    ip = (INODE *)buf + 1;

    printf("mode=0x%4x\n", ip->i_mode);
    printf("uid=%d  gid=%d\n", ip->i_uid, ip->i_gid);
    printf("size=%d\n", ip->i_size);
    printf("time=%s", ctime(&ip->i_ctime));
    printf("link=%d\n", ip->i_links_count);
    printf("i_block[0]=%d\n", ip->i_block[0]);
}
char *disk = "mydisk";
int main(int argc, char *argv[ ]) { 
    if (argc > 1)
        disk = argv[1];

    fd = open(disk, O_RDONLY);
    if (fd < 0) {
        printf("open %s failed\n", disk);
        exit(1);
    }
    inode();
}
