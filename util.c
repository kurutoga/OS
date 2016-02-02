#include "type.h"

MOUNT *find_mount(int dev) {
    int i;
    for (i=0;i<NMOUNT;i++) {
        if (mount[i].dev==0)
            continue;
        if (mount[i].dev==dev) {
            return &(mount[i]);
        }
    }
    return;
}

int bget_block(int dev, int blk, char buf[], int size) {
    lseek64(dev, (long long)(blk*size), 0);
    read(dev, buf, size);
}

int bput_block(int dev, int blk, char buf[], int size) {
    lseek64(dev, (long long)(blk*size), 0);
    write(dev, buf, size);
}


int get_block(int dev, int blk, char buf[]) {
    int i,size;
    for(i=0;i<NMOUNT;i++) {
        if (mount[i].dev==0)
            continue;
        if (mount[i].dev==dev) {
            size=mount[i].blksize;
            break;
        }
    }
    lseek64(dev, (long long)blk*size, 0);
    read(dev, buf, size);
}

int put_block(int dev, int blk, char buf[]) {
    int i,size;
    size=0;
    for(i=0;i<NMOUNT;i++) {
        if (mount[i].dev==0)
            continue;
        if (mount[i].dev==dev) {
            size=mount[i].blksize;
            break;
        }
    }
    lseek64(dev, (long long)blk*size, 0);
    write(dev, buf, size);
}

void init_mount(MOUNT *mnt, char *fs, char *path) {

    int dev, i, j, cgrp, rgroups, count, cons, size;
    char buf[1024], *gbuf;

    dev=mnt->dev;
    lseek(dev, 1024, 0);
    read(dev, buf, 1024);

    sp=(SUPER *)buf;
    if (sp->s_magic==0xEF53) {
        printf("EXT2 .. Procedding...\n");
    } else {
        printf(RED"Sorry this is not an ext2 device! Cannot proceed!\n"RESET);
        return;
    }
    
    mnt->busy=false;
    mnt->blksize=1024 << (sp->s_log_block_size);
    size=mnt->blksize;
    mnt->isize=sp->s_inode_size;
    mnt->nblocks=sp->s_blocks_count;
    mnt->bfree=sp->s_free_blocks_count;
    mnt->ifree=sp->s_free_inodes_count;
    mnt->ninodes=sp->s_inodes_count;
    mnt->gblocks=sp->s_blocks_per_group;
    mnt->ginodes=sp->s_inodes_per_group;
    mnt->ngroups=(mnt->nblocks/mnt->gblocks);
    mnt->gdesc=(GRPINFO **)calloc(mnt->ngroups, sizeof(GRPINFO *));
    for (i=0;i<=mnt->ngroups;i++)
        mnt->gdesc[i]=(GRPINFO *)malloc(sizeof(GRPINFO));
    strcpy(mnt->name, fs);
    strcpy(mnt->path_name, path);
    printf(BOLDGREEN "\nMOUNTED %s at %s.\n\n" RESET, mnt->name, mnt->path_name);
    printf("inode size =\t\t%d\n", mnt->isize);
    printf("group count =\t\t%d\n", mnt->ngroups);
    printf("# of blocks =\t\t%d\n", mnt->nblocks);
    printf("# of inodes =\t\t%d\n", mnt->ninodes);
    printf("free inodes =\t\t%d\n", mnt->ifree);
    printf("free blocks =\t\t%d\n", mnt->bfree);
    printf("blocks/grp  =\t\t%d\n", mnt->gblocks);
    printf("inodes/grp  =\t\t%d\n\n" , mnt->ginodes);
    gbuf=malloc(size+1);
    cgrp=0;
    cons=(size==1024) ? 2 : 1;
    rgroups=mnt->ngroups;
    for (i=0;i<=(mnt->ngroups/(mnt->blksize/sizeof(GD)));i++) {
        lseek(dev, size*(i+cons), 0);
        read(dev, gbuf, (mnt->blksize));
        if (rgroups>((mnt->blksize)/sizeof(GD))) {
            count=(mnt->blksize)/sizeof(GD);
        } else
            count=rgroups;
        for (j=0;j<=count;j++) {
            gp=(GD *)gbuf+j;
            mnt->gdesc[cgrp]->gno=cgrp;
            mnt->gdesc[cgrp]->imap=gp->bg_inode_bitmap;
            mnt->gdesc[cgrp]->bmap=gp->bg_block_bitmap;
            mnt->gdesc[cgrp]->ifree=gp->bg_free_inodes_count;
            mnt->gdesc[cgrp]->bfree=gp->bg_free_blocks_count;
            mnt->gdesc[cgrp]->itable=gp->bg_inode_table;
            printf(BOLDWHITE"Group[%d] INODETABLE = BLK[%d]\n"RESET , cgrp, mnt->gdesc[cgrp]->itable);
            cgrp++;
        }
        rgroups-=((mnt->blksize)/sizeof(GD));
    }
    free(gbuf);

}
sync_mount_atime(MOUNT *mnt) {
/*
    int i, size, dev;
    char *buffer;

    dev=mnt->dev;
    size=sizeof(SUPER);
    buffer=malloc(size);
    lseek(dev, (long)size, 0);
    read(dev, buffer, size);

    sp=(SUPER *)buffer;
    sp->s_mtime=sp->s_wtime=time(0L);

    lseek(dev, (long)size, 0);
    write(dev, buffer, size);

    free(buffer);
    */
}

sync_mount_desc(MOUNT *mnt) {

    int i, j, dev, size, cgrp, bur, rgroups, count, cons;
    char *buffer, *spr;
    
    dev=mnt->dev;
    size=mnt->blksize;
    buffer=malloc(size);
    spr=malloc(size);
    lseek(dev, (long)OFFSET, 0);
    read(dev, spr, sizeof(SUPER));

    sp=(SUPER *)spr;
    sp->s_free_blocks_count=mnt->bfree;
    sp->s_free_inodes_count=mnt->ifree;

    lseek(dev, (long)OFFSET, 0);
    write(dev, spr, sizeof(SUPER));

    cgrp=0;
    cons=(mnt->blksize==1024) ? 2 : 1;
    rgroups=mnt->ngroups;
    bur=rgroups/(size/sizeof(GD));
    for (i=0;i<=bur;i++) {
        get_block(dev, (i+cons), buffer);
        if (rgroups>(size/sizeof(GD))) {
            count=size/sizeof(GD);
        } else
            count=rgroups;
        for (j=0;j<=count;j++) {
            gp=(GD *)buffer+j;
            gp->bg_free_inodes_count=mnt->gdesc[(i*bur)+j]->ifree;
            gp->bg_free_blocks_count=mnt->gdesc[(i*bur)+j]->bfree;
        }
        rgroups-=(size/sizeof(GD));
        put_block(dev, (i+cons), buffer);
    }

    free(spr);
    free(buffer);
 
}

get_inode_ptr(int fd, int ino, INODE *it) {

    int i, grp, gino, gin, size, isz, itab;
    MOUNT *mnt;
    char *buffer;

    mnt=(MOUNT *)find_mount(fd);
    gin=mnt->ginodes;
    if (ino<gin) {
        grp=0;
        gino=ino-1;
    } else {
        grp=(int)((ino-1)/gin);
        gino=(ino-1)%gin;
    }
    size=mnt->blksize;
    isz=mnt->isize;
    itab=mnt->gdesc[grp]->itable;
    lseek64(fd, (long long)((itab*size)+gino*isz), 0);
    buffer=(char *)it;
    read(fd, buffer, sizeof(INODE));
    it=(INODE *)buffer;

}

long ialloc(MOUNT *mnt) {
  int  dev, i, j, size;
  char *buffer;

  dev=mnt->dev;
  size=mnt->blksize;
  buffer=malloc(size+1);

  for (i=0;i<=mnt->ngroups;i++) {
    get_block(dev, mnt->gdesc[i]->imap, buffer);

      for(j=0;j<mnt->ginodes;j++) {
          if (!TST_BIT(buffer, j)) {
              SET_BIT(buffer, j);
              mnt->ifree-=1;
              mnt->gdesc[i]->ifree-=1;
              put_block(dev, mnt->gdesc[i]->imap, buffer);
              free(buffer);
              return ((i*mnt->ginodes)+j+1);
          }
      }
  }
  printf(RED"No more free blocks!\n"RESET);
  free(buffer);
  return 0;
}
      
long balloc(MOUNT *mnt) {
  int  dev, i, j, size;
  char *buffer;

  dev=mnt->dev;
  size=mnt->blksize;
  buffer=malloc(size+1);

  for (i=0;i<=mnt->ngroups;i++) {
    get_block(dev, mnt->gdesc[i]->bmap, buffer);

      for(j=0;j<mnt->gblocks;j++) {
          if (!TST_BIT(buffer, j)) {
              SET_BIT(buffer, j);
              mnt->bfree-=1;
              mnt->gdesc[i]->bfree-=1;
              put_block(dev, mnt->gdesc[i]->bmap, buffer);
              free(buffer);
              return ((i*mnt->gblocks)+j+1);
          }
      }
  }
  printf(RED"No more free blocks!\n"RESET);
  free(buffer);
  return 0;
}

int idealloc(MOUNT *mnt, int ino) {
   int  dev, i, size, gno, ioff;
  char *buffer;
  
  dev=mnt->dev;
  size=mnt->blksize;
  buffer=malloc(size);
  gno=(ino-1)/(mnt->gblocks);
  ioff=(ino-1)%(mnt->gblocks);
  lseek64(dev, (long long)(mnt->gdesc[gno]->imap)*size, 0);
  read(dev, buffer, size);
  if (TST_BIT(buffer, ioff)) {
      CLR_BIT(buffer, ioff);
      mnt->ifree+=1;
      mnt->gdesc[gno]->ifree+=1;
      lseek64(dev, (long long)(mnt->gdesc[gno]->imap)*size, 0);
      write(dev, buffer, size);
  }
  free(buffer);
  return;
}

bdealloc(MOUNT *mnt, int bno) {
  int  dev, i, size, gno, boff;
  char *buffer;
  
  dev=mnt->dev;
  size=mnt->blksize;
  buffer=malloc(size);
  gno=(bno-1)/(mnt->gblocks);
  boff=(bno-1)%(mnt->gblocks);
  get_block(dev, mnt->gdesc[gno]->bmap, buffer);
  if (TST_BIT(buffer, boff)) {
      CLR_BIT(buffer, boff);
      mnt->bfree+=1;
      mnt->gdesc[gno]->bfree+=1;
      put_block(dev, mnt->gdesc[gno]->bmap, buffer);
  }
  free(buffer);
  return;
}

write_inode(MINODE *mip, char *buf) {
    MOUNT *mnt;
    int grp, gino, ino;

    mnt=(MOUNT *)find_mount(mip->dev);
    if (mnt==0) {
        printf(RED"cannot update inode! device not found\n"RESET);
        return -1;
    }
    ino=mip->ino;

    grp=(ino-1)/mnt->ginodes;
    gino=(ino-1)%mnt->ginodes;

    lseek64(mnt->dev, (long long)(mnt->gdesc[grp]->itable)*(mnt->blksize)+gino*(mnt->isize), 0);
    write(mnt->dev, buf, sizeof(INODE));
}

int search(MINODE *mip, char *pname) {

    MOUNT *mnt;
    int i,dev,j,size,fd,m,inode;
    char *buf, *indirect;
    int *blkno;
    dev = mip->dev;

    mnt=(MOUNT *)find_mount(dev);
    if (mnt==0)
        return -1;

    size=mnt->blksize;
    if (!S_ISDIR(mip->inode.i_mode))
        return 0;
    buf=malloc(size);
    indirect=malloc(size);

    for (i=0;i<12;i++) {
        get_block(dev, mip->inode.i_block[i], buf);
        dp = (DIR *)buf;
        cp = buf;

        while(cp<buf+size && dp->rec_len>0) {
            if (strncmp(pname, dp->name, strlen(pname))==0) 
                if(strlen(pname)==dp->name_len) {
                        inode=dp->inode;
                        FREE_2(buf, indirect);
                        return inode;
                }
            cp+=dp->rec_len;
            dp=(DIR *)cp;
        }
    }
    if (mip->inode.i_block[12]==0) {
        FREE_2(buf, indirect);
        return 0;
    }
    get_block(dev, mip->inode.i_block[12], buf);
    for (i=0;i<size/4;i++) {
        blkno=(int *)buf+i;
        if (*blkno==0)
            break;
        get_block(mip->dev, *blkno, indirect);
        dp=(DIR *)indirect;
        cp = indirect;
        while (cp<indirect+size && dp->rec_len>0) {
            if (strncmp(dp->name, pname, strlen(pname))==0)
                if(strlen(pname)==dp->name_len) {
                    inode=dp->inode;
                    FREE_2(buf, indirect);
                    return inode;
                }
            cp+=dp->rec_len;
            dp=(DIR *)cp;
        }
    }
    FREE_2(buf, indirect);
    return 0;
}

MINODE *iget(int dev, int ino) {

    int i;
    for (i=0;i<NMINODES;i++) {
        if (minode[i].dev==dev && minode[i].ino==ino) {
            minode[i].refCount+=1;
            return(minode+i);
        } else if (minode[i].refCount==0) {
            minode[i].dev=dev;
            minode[i].ino=ino;
            minode[i].mounted=false;
            get_inode_ptr(dev, ino, &minode[i].inode);
            minode[i].refCount+=1;
            return(minode+i);
        }
    }

    printf(RED"memory full!! try later?\n"RESET);
    return 0;

}

int iput(MINODE *mip) {

    mip->refCount-=1;
    if (mip->refCount>0 || mip->dirty==0)
        return;
    if (mip->dirty==1) {
        write_inode(mip, (char *)&(mip->inode));
        mip->dirty=false;
    }
    return;
}

int getino(int *dev, char *pathname) {

    MINODE *mip;
    MOUNT *mptr;
    char cwd[1024];
    char *names[64], *mount_name;
    int i,j,inumber,count=0, pino, ino;

    if (pathname[0]!='/') {
        mip=iget(*dev, running->cwd->ino);
        strcpy(cwd, "/");
        strcat(cwd, pathname);
    } else {
        mip=iget(root->dev, root->ino);
        strcpy(cwd, pathname);
    }
    names[count]=(char *)strtok(&cwd[1], "/");
    while(names[count++]!=NULL) 
        names[count]=(char *)strtok(NULL, "/");

    for (i=0;i<count-1;i++) {
        inumber = search(mip, names[i]);
        if (inumber==0) {
            iput(mip);
            return 0;
        }
        if (strcmp(names[i], "..")==0) {
            if (inumber==2 && search(mip, ".")==2 && mip->dev!=root->dev) {
                for(j=0;j<NMOUNT;j++) {
                    if (mount[j].dev==0)
                        continue;
                    if (mount[j].dev==*dev) {
                        *dev=root->dev;
                        mount_name=dirname(strdup(mount[j].path_name));
                        inumber=search(root, mount_name);
                        break;
                    }
                }

            }
        }
        iput(mip);
        mip=iget(*dev, inumber);
        if (mip!=root && mip->mounted==true) {
            for (j=0;j<NMOUNT;j++) {
                if (mount[j].dev==0)
                    continue;
                if (strcmp(mount[j].path_name, names[i])==0) {
                    *dev = mount[j].dev;
                    inumber = 2;
                    iput(mip);
                    mip=iget(*dev, inumber);
                    break;
                }

            }
        } 
    }        
    
    iput(mip);
    return inumber;
}

int findmyname(MINODE *parent, int myino, char *myname) {
    
    int i,j, size;
    char *buf, *indirect;
    int *blkno;
    MOUNT *mnt;
    INODE *inode = &(parent->inode);

    if (!S_ISDIR(inode->i_mode))
        return 0;

    mnt=(MOUNT *)find_mount(parent->dev);
    size=mnt->blksize;
    buf=malloc(size);
    indirect=malloc(size);

    for (i=0;i<12;i++) {
        get_block(parent->dev, inode->i_block[i], buf);
        dp = (DIR *)buf;
        cp = buf;

        while(cp<buf+size && dp->rec_len>0) {
            if(!(strncmp(dp->name, ".", 1)==0 || strncmp(dp->name, "..", 2)==0) && dp->inode==myino) {
                strncpy(myname, dp->name, dp->name_len);
                FREE_2(buf, indirect);
                return;
            }
            cp+=dp->rec_len;
            dp=(DIR *)cp;
        }
    }
    if(inode->i_block[12]==0) {
        FREE_2(buf, indirect);
        return 0;
    }
    get_block(parent->dev, inode->i_block[12], buf);
    for (i=0;i<size/4;i++) {
        blkno=(int *)buf+i;
        if (*blkno==0)
            break;
        get_block(parent->dev, *blkno, indirect);
        dp=(DIR *)indirect;
        cp = indirect;
        while (cp<indirect+size && dp->rec_len>0) {
            if (dp->inode==myino) {
                strncpy(myname, dp->name,  dp->name_len);
                FREE_2(buf, indirect);
                return;
            }
            cp+=dp->rec_len;
            dp=(DIR *)cp;
        }
    }
    FREE_2(buf, indirect);
    return 0;
}

int findino (MINODE *mip, int *myino, int *parentino) {
    *myino=search(mip, ".");
    *parentino=search(mip, "..");
    return 0;
}
