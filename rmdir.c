int rmdir(char *pathname) {

    char temp[1024];
    char *parent, *child;
    int i, ino, dev, cdev;
    MINODE *pip, *mip;
    MOUNT *mnt;

    strcpy(temp, pathname);
    child=basename(pathname);
    parent=dirname(strdup(pathname));

    dev = running->cwd->dev;
    ino = getino(&dev, pathname);
    mip = iget(dev, ino);
    if(mip->inode.i_gid!=0 && mip->inode.i_uid!=running->cwd->inode.i_uid) {
        iput(mip);
        return -1;
    }
    if(!S_ISDIR(mip->inode.i_mode) || mip->mounted>0 || mip->refCount>1 || not_empty(mip)) {
        iput(mip);
        return -1;
    }
    mnt=(MOUNT *)find_mount(mip->dev);
    for(i=0;i<12;i++) {
        if(mip->inode.i_block[i]==0)
            continue;
        bdealloc(mnt, mip->inode.i_block[i]);
    }
    idealloc(mnt, mip->ino);
    iput(mip);
    dev=running->cwd->dev;
    ino = getino(&dev, parent);
    pip=iget(dev, ino);
 
    rm_child(pip, child);
    
    pip->inode.i_links_count-=1;
    touch_ino((INODE *)&pip->inode);
    pip->dirty=true;
    sync_mount_desc(mnt);
    iput(pip);
    return 0;
}

int rm_child(MINODE *pip, char *name) {
   
    int i,j,fd, size, remain_length,del_length,nxt, *blkno, *nextbno;
    char *buf, *indirect, *temp, *last_dir, *next_dir;
    DIR *nextdp,td;
    MOUNT *mnt;

    fd = pip->dev;
    mnt=(MOUNT *)find_mount(fd);
    if (!S_ISDIR(pip->inode.i_mode))
        return 0;
    size=mnt->blksize;
    buf=malloc(size+1);
    indirect=malloc(size+1);
    temp=malloc(size+1);

    remain_length=size;
    for (i=0;i<12;i++) {
        get_block(pip->dev, pip->inode.i_block[i], buf);
        dp = (DIR *)buf;
        cp = buf;
        while(cp<buf+size && dp->rec_len>0) {
            remain_length-=dp->rec_len;
            if (strncmp(name, dp->name, strlen(name))==0)
                if(strlen(name)==dp->name_len) {
                    del_length=dp->rec_len;
                    if (remain_length!=size-dp->rec_len) {
                        if (remain_length==0) {
                                dp=(DIR *)last_dir;
                                dp->rec_len+=del_length;
                                put_block(pip->dev, pip->inode.i_block[i], buf);
                                break;
                        }
                        next_dir=(buf+(size-remain_length));
                        nextdp=(DIR *)(next_dir);
                        while(true) {
                            next_dir+=nextdp->rec_len;
                            fill_dir(dp, nextdp->rec_len, nextdp->name_len, nextdp->inode, nextdp->name);
                            if(dp->rec_len>dp->name_len) {
                                dp->rec_len+=del_length;
                                break;
                            }
                            cp+=dp->rec_len;
                            dp=(DIR *)cp;
                            nextdp=(DIR *)(next_dir);
                        }
                        put_block(pip->dev, pip->inode.i_block[i], buf);
                        break;
                    } else {
                        
                        bdealloc(mnt, pip->inode.i_block[i]);
                        pip->inode.i_size-=size;
                        while(pip->inode.i_block[i+1]!=0 && i<12) {
                            pip->inode.i_block[i]=pip->inode.i_block[i+1];
                            i++;
                        }
                        if(i!=12 ||pip->inode.i_block[12]==0) {
                            pip->dirty=true;
                            FREE_3(buf, indirect, temp);
                            return;
                        }
                        get_block(pip->dev, pip->inode.i_block[12], buf);
                        blkno=(int *)buf;
                        pip->inode.i_block[11]=*blkno;
                        nextbno=(int *)buf+1;
                        for (i=0;i<(size/4)-1;i++) {
                            *blkno=*nextbno;
                            if(*nextbno==0)
                                break;
                        }
                        pip->dirty=true;
                        FREE_3(buf, indirect, temp);
                        return;
                    }
                }
            last_dir=cp;               
            cp+=dp->rec_len;
            dp=(DIR *)cp;
        }
    }
} 

int not_empty(MINODE *mip) {

    int i;
    int *blkno, *indblkno, *dindblkno;
    char *buf, *indbuf, *dindbuf;
    MOUNT *mnt;

    if (mip->inode.i_links_count>2) {
        printf(RED"Not empty!\n"RESET);
        return true;
    }
    mnt=(MOUNT *)find_mount(mip->dev);
    i=mnt->blksize;
    buf=malloc(i+1);
    indbuf=malloc(i+1);
    dindbuf=malloc(i+1);
   
    for (i=0;i<12;i++) {
        if(mip->inode.i_block[i]==0)
            FREE_3(buf, indbuf, dindbuf);
            return false;
        get_block(mip->dev, mip->inode.i_block[i], buf);
        dp=(DIR *)(buf);
        cp=buf;
    
        while(cp<buf+(mnt->blksize) && dp->rec_len>0) {
            if (!(strcmp(dp->name, ".")==0 || strcmp(dp->name, "..")==0)) {
                printf(RED"dir not empty. cannot delete\n"RESET);
                FREE_3(buf, indbuf, dindbuf);
                return true;
            }
            cp+=dp->rec_len;
            dp=(DIR *)cp;
        }
    }
    FREE_3(buf, indbuf, dindbuf);
    return false;
}
