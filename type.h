/* type.h file for CS360 FS */

#include <stdio.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>   // MAY NEED "ext2_fs.h"
#include <libgen.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/stat.h>

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

//ext2 MACROS
#define MAXGRPS             63          //2 GBS
#define OLD_REVISION        0
#define OFFSET              (1024)
#define BLKSIZE(s)          (OFFSET << s->s_log_block_size)
#define DEFAULT_INODE_SIZE  (128)
#define BLOCKS_PER_GRP(s)   ((s)->s_blocks_per_group)
#define INODESIZE(s)        (((s)->s_rev_level == OLD_REVISION) ? \
                            DEFAULT_INODE_SIZE : \
                            (s)->s_inode_size)
#define INODE_TABLE(g)      ((g)->bg_inode_table)
#define INODES_PER_BLOCK(s) (BLKSIZE(s)/INODESIZE(s))
#define INODES_PER_GRP(s)   (s->s_inodes_per_group)
#define INODE_OFFSET(s)     BLKSIZE(s)%INODESIZE(s)
#define NINODES(s)          (s->s_inodes_count)
#define NBLOCKS(s)          (s->s_blocks_count)
#define IMAP(g)             (g->bg_inode_bitmap)
#define BMAP(g)             (g->bg_block_bitmap)

//general MACROS
#define TST_BIT(O,b)        ((O[b/8] & ( 1 << b%8 )) ? \
                            1 : 0)
#define SET_BIT(O,b)        (O[b/8] |= ( 1 << (b%8) ))
#define CLR_BIT(O,b)        (O[b/8] &= (~( 1 << (b%8) )))

// free mem MACROS
#define FREE_1(A)           free(A)
#define FREE_2(A,B)         free(A);\
                            free(B)
#define FREE_3(A,B,C)       free(A);\
                            free(B);\
                            free(C)
#define FREE_4(A,B,C,D)     free(A);\
                            free(B);\
                            free(C);\
                            free(D)
#define FREE_5(A,B,C,D,E)   free(A);\
                            free(B);\
                            free(C);\
                            free(D);\
                            free(E)

// Block number of EXT2 FS on FD
#define SUPERBLOCK        1
#define GDBLOCK           2
#define ROOT_INODE        2

// Default dir and regular file modes
#define DIR_MODE    0040777 
#define FILE_MODE   0100644
#define SUPER_MAGIC  0xEF53
#define SUPER_USER        0

// Proc status
#define FREE              0
#define READY             1
#define RUNNING           2

// Table sizes
#define NMINODES        100
#define NMOUNT           10
#define NPROC            10
#define NFD              10
#define NOFT            100
 enum { false, true } bool;

#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */


     
// In-memory inodes structure
typedef struct minode{		
  INODE inode;               // disk inode
  int   dev, ino;
  int   refCount;
  int   dirty;
  int   mounted;
  struct mount *mountptr;
}MINODE;
typedef struct oft{
  int   mode;
  int   refCount;
  struct minode *ptr;
  int   offset;
}OFT;

// PROC structure
typedef struct proc{
  struct proc *nextProcPtr;
  int   uid;
  int   pid;
  int   gid;
  int   status;
  struct minode *cwd;
  OFT   *fd[NFD];
}PROC;
 
typedef struct grpinfo{
    int   bmap;
    int   imap;
    int   itable;
    int   gno;
    int   ifree;
    int   bfree;
}GRPINFO;

// Mount Table structure
typedef struct mount{
    int    dev, busy;
    int    nblocks,ninodes,ngroups,blksize;
    int    gblocks,ginodes,ifree,bfree,isize;
    struct grpinfo** gdesc;
    struct minode *mounted_inode;
    char   name[256]; 
    char   path_name[256];
}MOUNT;

//globals
MINODE minode[NMINODES];
MOUNT mount[NMOUNT];
PROC P[NPROC];
OFT ftable[NOFT];
char *sbuf;
char *gbuf;
PROC *running;
GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp; 
char  *cp;
MINODE *root;
