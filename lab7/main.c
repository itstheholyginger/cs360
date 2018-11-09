// #include "type.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

/***** type.h file for EXT2 FS *****/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <libgen.h>
#include <string.h>
#include <sys/stat.h>
// define shorter TYPES for convenience
typedef struct ext2_group_desc GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode INODE;
typedef struct ext2_dir_entry_2 DIRR;
#define BLKSIZE 1024
// Block number of EXT2 FS on FD
#define SUPERBLOCK 
#define GDBLOCK 2
#define ROOT_INODE 2
// Default dir and regular file modes
#define DIR_MODE 0x41ED
#define FILE_MODE 0x81AE
#define SUPER_MAGIC 0xEF53
#define SUPER_USER 0
// Proc status
#define FREE 0
#define BUSY 1
/// file system table sizes
#define NMINODE 100
#define NMTABLE 10
#define NPROC 2
#define NFD 10
#define NOFT 40

char gpath[256];
char *name[64]; // assume at most 64 components in pathnames
int n;

struct stat mystat, *sp;
char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";

int fd, dev;
int nblocks, ninodes, bmap, imap, inode_start, iblock;
char line[256], cmd[32], pathname[256];

char *disk = "mydisk";

int get_block(int dev, int blk, char *buf)
{
    lseek(dev, blk * BLKSIZE, SEEK_SET);
    int n = read(dev, buf, BLKSIZE);
    if (n < 0)
        printf("get_block(%d %d) error\n", dev, blk);
}

int put_block(int dev, int blk, char *buf)
{
    lseek(dev, blk * BLKSIZE, SEEK_SET);
    int n = write(dev, buf, BLKSIZE);
    if (n != BLKSIZE)
        printf("put_block[% d % d] error \n", dev, blk);
}

// In-memory inodes structure
typedef struct minode
{
    INODE INODE;
    int mptr;
    int mountptr;
    // disk inode
    int dev, ino;
    int refCount;
    // use count
    int dirty;
    // modified flag
    int mounted;
    // mounted flag
    struct mount *mntPtr;
    // mount table pointer
    // int lock;
    // ignored for simple FS
} MINODE;
MINODE minode[NMINODE];
MINODE *root;

typedef struct oft
{
    int mode;
    int refCount;
    MINODE *mptr;
    int offset;
} OFT;

// PROC structure
typedef struct proc
{
    struct Proc *next;
    int pid;
    int uid;
    int gid;
    int ppid;
    int status;
    struct minode *cwd;
    OFT *fd[NFD];
} PROC;
PROC proc[NPROC], *running;

// Mount Table structure
typedef struct mtable
{
    int dev;
    int ninodes;
    int nblocks;
    int free_blocks;
    int free_inodes;
    int bmap;
    int imap;
    int iblock;
    MINODE *mntDirPtr;
    char devName[64];
    char mntName[64];
} MTABLE;
MINODE *mialloc()
{ // allocate a FREE minode for use
    int i;
    for (i = 0; i < NMINODE; i++)
    {
        MINODE *mp = &minode[i];
        if (mp->refCount == 0)
        {
            mp->refCount = 1;
            return mp;
        }
    }
    printf("FS panic: out of minodes\n");
    return 0;
}
int midalloc(MINODE *mip)
{ // release a used minode
    mip->refCount = 0;
}

/*************** globals for Level-1 ********************/

int dbname(char *pathname, char *dname, char *bname);
/******* WRITE YOUR OWN util.c and others ***********
#include "util.c"
#include "cd_ls_pwd.c"
***************************************************/
int dbname(char *pathname, char *dname, char *bname)
{
    char temp[128]; // dirname(), basename() destroy original pathname
    strcpy(temp, pathname);
    strcpy(dname, dirname(temp));
    strcpy(temp, pathname);
    strcpy(bname, basename(temp));
}

// int get_block(int fd, int blk, char buf[]) {
//     lseek(fd, (long)blk * BLKSIZE, 0);
//     read(fd, buf, BLKSIZE);
// }

MINODE *iget(int dev, int ino)
{
    MINODE *mip;
    MTABLE *mp;
    INODE *ip;
    int i, block, offset;
    char buf[BLKSIZE];
    // serach in-memory minodes first
    for (i = 0; i < NMINODE; i++)
    {
        MINODE *mip = &minode[i];
        if (mip->refCount && (mip->dev == dev) && (mip->ino == ino))
        {
            mip->refCount++;
            return mip;
        }
    }
    // needed INODE=(dev,ino) not in
    mip = mialloc();
    mip->dev = dev;
    mip->ino = ino;
    block = (ino - 1) / 8 + iblock;
    offset = (ino - 1) % 8;
    get_block(dev, block, buf);
    ip = (INODE *)buf + offset;
    mip->INODE = *ip;
    // initialize minode
    mip->refCount = 1;
    mip->mounted = 0;
    mip->dirty = 0;
    mip->mountptr = 0;
    return mip;
}

int iput(MINODE *mip)
{
    INODE *ip;
    int i, block, offset;
    char buf[BLKSIZE];
    if (mip == 0)
        return;
    mip->refCount--;
    if (mip->refCount > 0)
        return;
    if (mip->dirty == 0)
        return;
    // dec refCount by 1
    // still has user
    // no need to write back
    // write INODE back to disk
    block = (mip->ino - 1) / 8 + iblock;
    offset = (mip->ino - 1) % 8;
    // get block containing this inode
    get_block(mip->dev, block, buf);
    ip = (INODE *)buf + offset;
    *ip = mip->INODE;
    put_block(mip->dev, block, buf);
    midalloc(mip);
}

char *name[64], buf[256]; // token string pointers
char gline[256];          // holds token strings, each pointed by a name[i]
int nname;
// number of token strings
int tokenize(char *pathname)
{
    char *s;
    strcpy(gline, pathname);
    nname = 0;
    s = strtok(gline, "/");
    while (s)
    {
        name[nname++] = s;
        s = strtok(0, "/");
    }
}
int search(MINODE *mip, char *name)
{
    int i;
    char *cp, temp[256], sbuf[BLKSIZE];
    DIRR *dp;
    for (i = 0; i < 12; i++)
    { // search DIR direct blocks only
        if (mip->INODE.i_block[i] == 0)
            return 0;
        get_block(mip->dev, mip->INODE.i_block[i], sbuf);
        dp = (DIRR *)sbuf;
        cp = sbuf;
        while (cp < sbuf + BLKSIZE)
        {
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;
            printf("%8d%8d%8u %s\n",
                   dp->inode, dp->rec_len, dp->name_len, temp);
            if (strcmp(name, temp) == 0)
            {
                printf("found %s : inumber = %d\n", name, dp->inode);
                return dp->inode;
            }
            cp += dp->rec_len;
            dp = (DIRR *)cp;
        }
    }
    return 0;
}

int getino(char *pathname)
{
    MINODE *mip;
    int i, ino;
    if (strcmp(pathname, "/") == 0)
    {
        return 2;
        // return root ino=2
    }
    if (pathname[0] == '/')
        mip = root;
    // if absolute pathname: start from root
    else
        mip = running->cwd;
    // if relative pathname: start from CWD
    mip->refCount++;
    // in order to iput(mip) later
    tokenize(pathname);
    for (i = 0; i < nname; i++)
    {
        // search for each component string
        if (!S_ISDIR(mip->INODE.i_mode))
        { // check DIR type
            printf("%s is not a directory\n", name[i]);
            iput(mip);
            return 0;
        }
        ino = search(mip, name[i]);
        if (!ino)
        {
            printf("no such component name %s\n", name[i]);
            iput(mip);
            return 0;
        }
        iput(mip);
        // release current minode
        mip = iget(dev, ino);
        // switch to new minode
    }
    iput(mip);
    return ino;
}

int init()
{
    int i;
    MINODE *mip;
    PROC *p;

    printf("init()\n");

    for (i = 0; i < NMINODE; i++)
    {
        mip = &minode[i];
        // set all entries to 0;
        mip->refCount = 0;
        mip->dirty = 0;
        mip->mounted = 0;
        mip->mountptr = 0;
    }
    for (i = 0; i < NPROC; i++)
    {
        p = &proc[i];
        p->pid = i;
        p->uid = i;
        p->cwd = 0;
    }
}

// load root INODE and set root pointer to it
int mount_root()
{
    char buf[BLKSIZE];
    SUPER *sp;
    GD *gp;
    MINODE mip;

    printf("mount_root()\n");
    /*
  (1). read super block in block #1;
  verify it is an EXT2 FS;
  record nblocks, ninodes as globals;
    */
    /* get super block of the rootdev */
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    /* check magic number */
    if (sp->s_magic != SUPER_MAGIC)
    {
        printf("super magic=%x : %s is not an EXT2 file system\n",
               sp->s_magic, disk);
        exit(0);
    }
    nblocks = sp->s_blocks_count;
    ninodes = sp->s_inodes_count;
    /*
    (2). get GD0 in Block #2:
    record bmap, imap, inodes_start as globals
    */
    get_block(dev, 2, buf);
    gp = (GD *)buf;
    bmap = gp->bg_block_bitmap;
    imap = gp->bg_inode_bitmap;
    inode_start = gp->bg_inode_table;
    /*
    (3). root = iget(dev, 2);       // get #2 INODE into minode[ ]
    printf("mounted root OK\n");
    */
    root = iget(dev, 2);
    printf("mounted root OK\n");
}

int ls(char *pathname)
{
    int ino, n;
    MINODE *mip;
    char dname[64], bname[64], temp[64];
    struct stat path_stat;
    printf("pathname:\t%s\n", pathname);
    getchar();
    stat(pathname, &path_stat);

    if (S_ISDIR(path_stat.st_mode))
    {
        ls_dir(pathname);
    }
    else
    {
        ls_file(pathname);
    }
}

ls_dir(char *dirname)
{
    DIR *dir;
    struct dirent *pDirent;
    dir = opendir(dirname);
    if (!dir) {
        printf("Cannot open directory");
        return -1;
    }
    while(pDirent = readdir(dir)) {

        ls(pDirent->d_name);
    }
}

ls_file(char *filename)
{
    printf("ls filename");
    struct stat fstat, *sp;
    int r, i;
    char ftime[64];
    sp = &fstat;
    if ((r = lstat(filename, &fstat)) < 0)
    {
        printf("can’t stat %s\n", filename);
        exit(1);
    }

    if ((sp->st_mode & 0xF000) == 0x8000) // if (S_ISREG())
        printf("%c", '-');
    if ((sp->st_mode & 0xF000) == 0x4000) // if (S_ISDIR())
        printf("%c", 'd');
    if ((sp->st_mode & 0xF000) == 0xA000) // if (S_ISLNK())
        printf("%c", 'l');
    for (i = 8; i >= 0; i--)
    {
        if (sp->st_mode & (1 << i)) // print r|w|x
            printf("%c", t1[i]);
        else
            printf("%c", t2[i]);
        // or print -
    }
    printf("%4d ", sp->st_nlink); // link count
    printf("%4d ", sp->st_gid);
    // gid
    printf("%4d ", sp->st_uid);
    // uid
    printf("%8d ", sp->st_size);
    // file size
    // print time
    strcpy(ftime, ctime(&sp->st_ctime)); // print time in calendar form
    ftime[strlen(ftime) - 1] = 0;
    // kill \n at end
    printf("%s ", ftime);
    // print name
    printf("%s", basename(filename)); // print file basename
    // print -> linkname if symbolic file
    if ((sp->st_mode & 0xF000) == 0xA000)
    {
        // use readlink() to read linkname
        readlink(filename, buf, 1028);
        printf(" -> %s", buf); // print linked name
    }
    printf("\n");
}


int main(int argc, char *argv[])
{
    int ino;
    char buf[BLKSIZE];
    if (argc > 1)
        disk = argv[1];

    fd = open(disk, O_RDWR);
    if (fd < 0)
    {
        printf("open %s failed\n", disk);
        exit(1);
    }
    dev = fd;

    init();
    mount_root();
    printf("root refCount = %d\n", root->refCount);

    printf("creating P0 as running process\n");
    running = &proc[0];
    running->status = BUSY;
    running->cwd = iget(dev, 2);
    // set proc[1]'s cwd to root also
    printf("root refCount = %d\n", root->refCount);

    while (1)
    {
        printf("input command : [ls|cd|pwd|quit] ");
        fgets(line, 128, stdin);
        line[strlen(line) - 1] = 0;

        if (line[0] == 0)
            continue;
        pathname[0] = 0;

        sscanf(line, "%s %s", cmd, pathname);
        printf("cmd=%s pathname=%s\n", cmd, pathname);

        if (strcmp(cmd, "ls") == 0)
        {
            printf("command was ls\npathname:\t%s\n");
            ls(pathname);
        }

        // if (strcmp(cmd, "cd")==0)
        // chdir(pathname);

        // if (strcmp(cmd, "pwd")==0)
        // pwd(running->cwd);

        if (!strcmp(cmd, "quit"))
            printf("quitting.....\n");
        quit();
    }
}

// int print(MINODE *mip)
// {
//     int blk;
//     char buf[1024], temp[256];
//     int i;
//     DIR *dp;
//     char *cp;

//     INODE *ip = &mip->INODE;
//     for (i = 0; i < 12; i++)
//     {
//         if (ip->i_block[i] == 0)
//             return 0;
//         get_block(dev, ip->i_block[i], buf);

//         dp = (DIR *)buf;
//         cp = buf;

//         while (cp < buf + 1024)
//         {
//             strncpy(temp, dp->name, dp->name_len);
//             temp[dp->name_len] = 0; // removing end of str delimiter '/0', why tho?
//             // make dp->name a string in temp[ ]
//             printf("%4d %4d %4d %4s\n", dp->inode, dp->rec_len, dp->name_len, temp);

//             cp += dp->rec_len;
//             dp = (DIR *)cp;
//         }
//     }
// }

int quit()
{
    MINODE *mip;
    for (int i = 0; i < NMINODE; i++)
    {
        mip = &minode[i];
        if (mip->refCount > 0)
            iput(mip);
    }
    exit(0);
}
