#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <sys/stat.h>
/*
 * Write a C program, showblock, which displays the disk blocks of a file in an EXT2 file system.
 * The program runs as follows:
 *      showblock     DEVICE     PATHNAME
 *      ---------    ---------   --------
 * e.g. showblock    diskimage   /a/b/c/d
 *
 * It locates the file named PATHNAME and prints the disk blocks
 * (direct, indirect, double-indirect) of the file */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

typedef struct ext2_group_desc GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode INODE;
typedef struct ext2_dir_entry_2 DIR;

#define BLKSIZE 1024
#define BLKOFFSET(block) (BLKSIZE + block-1)*BLKSIZE


GD *gp;
SUPER *sp;
INODE *ip;
DIR *dp;

char* disk = "mydisk", char_buf[BLKSIZE], *name[128], *pathname = "/", dbuf[BLKSIZE];
int fd, inode_num;


int get_block(int fd, int blk, char buf[]) {
    lseek(fd, (long)blk * BLKSIZE, 0);
    read(fd, buf, BLKSIZE);
}

void get_inode(int fd, int ino, int inode_table, INODE *inode) {
    lseek(fd, BLKOFFSET(inode_table) + (ino - 1) * sizeof(INODE), 0);
    read(fd, inode, sizeof(INODE));
}

int does_exf2fs_exist() {
    printf("File Descriptor:\t%d\nt", fd);
    printf("char_buf:\t%d\n", char_buf);

    get_block(fd, 1, char_buf);
    sp = (SUPER *)char_buf;

    printf("valid ext2 fs: s_magic =\t%x\n", sp->s_magic);
    if(sp->s_magic != 0xEF53) {
        printf("This is not an ext2 file.....exiting\n");
        exit(1);
    }
    printf("\n******SUPERBLOCK******\n\tnblocks:\t%d\n\tinodes_per:\t%d\n\tfree_inodes:\t%d\n\tfree_total_blocks:\t%d\n", sp->s_blocks_count, sp->s_inodes_count, sp->s_inodes_per_group, sp->s_free_inodes_count, sp->s_free_blocks_count);
}
int InodesBeginBlock = 0;

int get_group_descriptor_get_inode_begin() {
    get_block(fd, 2,char_buf);
    gp = (SUPER *)char_buf;

    InodesBeginBlock = gp->bg_inode_table;
    printf("InodesBeginBlock:\t%d\n", InodesBeginBlock);
}

int parse_inode_from_beginning_block() {
    printf("InodeBeginBlock:\t%d\n", InodesBeginBlock);

    get_block(fd, InodesBeginBlock, char_buf);

    ip = (INODE *)char_buf + 1;

    printf("\tMode:\t%4x", ip->i_mode);
    printf("\tMid:\t%d\tgid=%d\n", ip->i_uid, ip->i_gid);
    printf("\tSize:\t%d\n", ip->i_size);
    printf("\tTime:\t%s", ctime(&ip->i_ctime));
    printf("\tLink:\t%d\n", ip->i_links_count);
    printf("\ti_block[0]:\t%d\n", ip->i_block[0]);
}

int i_count = 0, n = 0;

int tokenize() {
    printf("\nPathname:\t%s\n", pathname);
    name[0]= strtok(pathname, "/");
    printf("\tname[0]:\t%s\n", name[0]);

    while(name[i_count]) {
        i_count++;
        name[i_count] = strtok(NULL, "/");
        printf("\tname[%d]:\t%s\n", i_count, name[i_count]);
    }
    n = i_count;
    printf("\tn:\t%d\n", n);
}

int search(INODE *ip, char *name) {
    printf("\n*************************");
    printf("\nSearching for:\t%s", name);
    for (i_count = 0; i_count < 12; i_count++) {
        if (!ip->i_block[i_count]) {
            return 0;
        }
        get_block(fd, ip->i_block[i_count], dbuf);

        DIR *dp = (SUPER *)dbuf;
        char *cp = dbuf;

        while (cp < &dbuf[BLKSIZE]) {
            if (!strcmp(name, dp->name)) {
                printf("\n\tFound at INODE:\t%d\n", dp->inode);
                return dp->inode;
            }
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
        printf("\tNot Found\n");
        return 0;
    }
}

int showblock() {
    does_exf2fs_exist();

    get_group_descriptor_get_inode_begin();

    parse_inode_from_beginning_block();

    tokenize();

    for (i_count = 0; i_count < n; i_count++) {
        inode_num = search(ip, name[i_count]);
            if (!inode_num) {
                printf("\nCan't find name[%d]:\t'%s'\n", i_count, name[i_count]);
                exit(1);
            }
            int inodes_per_block = BLKSIZE/sizeof(INODE);

            get_block(fd, (((inode_num - 1) / inodes_per_block) + InodesBeginBlock), char_buf);
            ip = (INODE *)char_buf + ((inode_num - 1) % inodes_per_block);

            printf("\nFound Inode:\n");
            printf("\tinode:\t%d\n", inode_num);
            printf("\tInodesPerBlock:\t%d\n", inodes_per_block);
            printf("\tIn Block:\t%d\n", ((inode_num - 1) / inodes_per_block) + InodesBeginBlock);
            printf("\tOffset:\t%d\n", (inode_num - 1) % inodes_per_block);
            printf("\tMode:\t%4x ", ip->i_mode);
            printf("\tuid:\t%d.\tgid:\t%d", ip->i_uid, ip->i_gid);
            printf("\tSize:\t%d\n", ip->i_size);
            printf("\tTime:\t%s\n", ctime(&ip->i_ctime));
            printf("\tLink:\t%d\n", ip->i_links_count);
            printf("\ti_block[0]:\t%d", ip->i_block[0]);

            if (S_ISDIR(ip->i_mode)) {
                continue;
            } else {
                if (i_count == n - 1) {
                    continue;
                } else {
                    printf("\nname[%d], '%s', is not a DIR\n", i_count, name[i_count]);
                    exit(1);
                }
            }
    }

    int total_blocks[256], dist_block_one[256], disk_block_two[256];
    printf("\n****** The direct blocks of inode %d is: ******\n", inode_num);
    for(i_count = 0; i_count < 14; i_count++) {
        if (!ip->i_block[i_count]) {
            continue;
        } else {
            printf("i_block[%d]:\t%d\n", i_count, ip->i_block[i_count]);
        }
    }
    printf("\n****** Indirect Blocks ******\n");
    get_block(fd, ip->i_block[12], total_blocks);
    putchar(' ');

    for (i_count = 0; i_count < (sizeof(total_blocks) / sizeof(int)); i_count++) {
        if (total_blocks[i_count]) {
            printf("%d ", total_blocks[i_count]);
            if (!(i_count % 10) && i_count) {
                printf("\n ");
            }
        } else {
            continue;
        }
    }
    printf("\n\n****** Double Indirect Blocks ******\n");
    get_block(fd, ip->i_block[13], dist_block_one);
    putchar(' ');
    for(i_count = 0; i_count < (sizeof(dist_block_one) / sizeof(int)); i_count++) {
        if(dist_block_one[i_count]) {
            printf("%d ", dist_block_one[i_count]);
            printf("\n********************************\n");
            putchar(' ');
            get_block(fd, dist_block_one[i_count], disk_block_two);
            for (int i = 0; i< 256; i++) {
                if (dist_block_one[i]) {
                    printf("%d ", disk_block_two[i]);
                    if (!(i % 10) && i) {
                        printf("\n ");
                    }
                } else {
                    continue;
                }
            }
        }
    }

}


int main(int argc, char *argv[]) {
    printf("Hello, World!\n");
    if (argc > 1) {
        disk = argv[1];
    }
    if (argc > 2) {
        pathname = argv[2];
    }

    fd= open(disk, O_RDONLY);
    if (fd < 0) {
        printf("open %s failed\n", disk);
        exit(1);
    }
    printf("'%s' is in O_RDONLY mode\n", disk);
    showblock();

    return 0;
}