#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp; 

#define BLKSIZE 1024

int fd;
int imap, bmap;
int ninodes, nblocks, nfreeInodes, nfreeBlocks;

int get_block(int fd, int blk, char buf[ ]) {
  lseek(fd, (long)blk*BLKSIZE, 0);
  read(fd, buf, BLKSIZE);
}
int put_block(int fd, int blk, char buf[ ]) {
  lseek(fd, (long)blk*BLKSIZE, 0);
  write(fd, buf, BLKSIZE);
}
int tst_bit(char *buf, int bit) {
  int i, j;
  i = bit/8; j=bit%8;
  if (buf[i] & (1 << j)) {
     return 1;
  }
  return 0;
}
int set_bit(char *buf, int bit) {
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] |= (1 << j);
}
int clr_bit(char *buf, int bit) {
  int i, j;
  i = bit/8; j=bit%8;
  buf[i] &= ~(1 << j);
}
int decFreeInodes(int dev) {
  char buf[BLKSIZE];

  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_blocks_count--;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_blocks_count--;
  put_block(dev, 2, buf);
}

int balloc(int dev) {
  int  i;
  char buf[BLKSIZE];

  get_block(dev, bmap, buf);

  for (i=0; i < ninodes; i++) {
    if (tst_bit(buf, i)==0) {
       set_bit(buf,i);
       decFreeInodes(dev);

       put_block(dev, bmap, buf);

       return i+1;
    }
  }
  printf("ialloc(): no more free inodes\n");
  return 0;
}
char *disk = "mydisk";

int main(int argc, char *argv[ ]) {
  int i, bno;
  char buf[BLKSIZE];

  if (argc > 1) {
    disk = argv[1];
  }
  fd = open(disk, O_RDWR);
  if (fd < 0) {
    printf("open %s failed\n", disk);
    exit(1);
  }
  get_block(fd, 1, buf);
  sp = (SUPER *)buf;

  ninodes = sp->s_inodes_count;
  nblocks = sp->s_blocks_count;
  nfreeInodes = sp->s_free_inodes_count;
  nfreeBlocks = sp->s_free_blocks_count;
  printf("ninodes=%d\nnblocks=%d\nnfreeInodes=%d\nnfreeBlocks=%d\n", 
	 ninodes, nblocks, nfreeInodes, nfreeBlocks);

  get_block(fd, 2, buf);
  gp = (GD *)buf;

  bmap = gp->bg_block_bitmap;
  printf("bmap = %d\n", bmap);
  getchar();
  for (i=0; i < 5; i++){  
    bno = balloc(fd);
    printf("allocated bno = %d\n", bno);
  }
}