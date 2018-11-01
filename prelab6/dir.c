#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <time.h>
#include <string.h>

#define BLKSIZE 1024

typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp; 

int fd;
int iblock;

int search(INODE *ip2, char *name);

int get_block(int fd, int blk, char buf[ ]) {
  lseek(fd,(long)blk*BLKSIZE, 0);
   read(fd, buf, BLKSIZE);
}

int dirFunction() {
  char buf[BLKSIZE];
  int size, count = 0;

  get_block(fd, 2, buf);
  gp = (GD *)buf;
  
  iblock = gp->bg_inode_table;
  printf("inode_block=%d\n", iblock);

  get_block(fd, iblock, buf);
  
  ip = (INODE *)buf + 1;
  size = ip->i_size;

  int ret = search(ip, "file1");
  printf("ret=%d\n", ret);

  get_block(fd, ip->i_block[0], buf);
  
  dp = (DIR *)buf;
  char nameval[BLKSIZE + 1];
  
  while(count < size && dp->inode) {
  	memcpy(nameval, dp->name, dp->name_len);
	nameval[dp->name_len] = '\0';
  	printf("path_name: %s \t --> \t inode_number: %u\n", nameval, dp->inode);
        dp = (void *)dp + dp->rec_len;
	count+=dp->rec_len;	
  }
}

int search(INODE *ip2, char *name) {
  int size, count = 0;
  char buf[BLKSIZE];
  size = ip->i_size;

  get_block(fd, ip->i_block[0], buf);

  dp = (DIR *)buf;
  char nameval[BLKSIZE + 1];
  while(count < size && dp->inode) {
        memcpy(nameval, dp->name, dp->name_len);
        nameval[dp->name_len] = '\0';
	if(!strcmp(nameval, name)) {
		return dp->inode;	
	}
        dp = (void *)dp + dp->rec_len;
        count+=dp->rec_len;
  }
  return 0;
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

  dirFunction();
}