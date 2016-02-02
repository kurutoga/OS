int unlink (char *pathname) {

    int i,ino,dev,pdev;
    MINODE *mip, *pip;
    MOUNT *mnt;
    char *child, *parent;

    dev=running->cwd->dev;
    ino=getino(&dev, pathname);
    if (ino==0) {
        printf(RED"Invalid Path!\n"RESET);
        return -1;
    }
    mip=iget(dev, ino);
    mnt=(MOUNT *)find_mount(mip->dev);
    if (!S_ISREG(mip->inode.i_mode) && !S_ISLNK(mip->inode.i_mode)) {
        printf(RED"Cannot unlink this filetype!\n"RESET);
        return -1;
    }
    mip->inode.i_links_count-=1;
    if (mip->inode.i_links_count==0) {
        truncate(mip);
        idealloc(mnt, mip->ino);
    }
    iput(mip);
    child=basename(pathname);
    parent=dirname(strdup(pathname));
    pdev=running->cwd->dev;
    ino=getino(&pdev, parent);
    pip=iget(pdev, ino);

    rm_child(pip, child);
    mnt=(MOUNT *)find_mount(pip->dev);
    sync_mount_desc(mnt);
    iput(pip);        
}

int truncate(MINODE *mip) {

    int i,j,k,bno,size,fd,count;
    int *blkno, *indblkno, *doindblkno;
    char *buf, *indbuf, *dindbuf;
    MOUNT *mnt;
    INODE *inodeP;

    inodeP=&(mip->inode);
    fd=mip->dev;
    if (S_ISLNK(inodeP->i_mode)) {
        inodeP->i_block[0]=0;
        return;
    }
    mnt=(MOUNT *)find_mount(fd);
    size=mnt->blksize;   
    buf=malloc(size);
    indbuf=malloc(size);
    dindbuf=malloc(size);
    while(true) {
        for(i=0;i<12;i++) {
            if(inodeP->i_block[i]==0)
                continue;
            bno=inodeP->i_block[i];
            bdealloc(mnt, bno);
        }
        if(inodeP->i_block[12]!=0) {
            get_block(fd, inodeP->i_block[12], buf);
            for(i=0;i<size/4;i++) {
                blkno=(int *)buf+i;
                if (*blkno==0)
                    continue;
                bdealloc(mnt, *blkno);
            }
            bdealloc(mnt, inodeP->i_block[12]);
        }
        if (inodeP->i_block[13]!=0) {
            get_block(fd, inodeP->i_block[13], buf);
            for(i=0;i<size/4;i++) {
                blkno=(int *)buf+i;
                if (*blkno==0)
                    continue;
                get_block(fd, *blkno, indbuf);
                for(j=0;j<size/4;j++) {
                    indblkno=(int *)indbuf+j;
                    if (*indblkno==0)
                        continue;
                    bdealloc(mnt, *indblkno);
                }
                bdealloc(mnt, *blkno);
            }
            bdealloc(mnt, inodeP->i_block[13]);
        }
        if(inodeP->i_block[14]!=0) {
            get_block(fd, inodeP->i_block[14], buf);
            for (i=0;i<size/4;i++) {
                blkno=(int *)buf+i;
                if (*blkno==0)
                    continue;
                get_block(fd, *blkno, indbuf);
                for (j=0;j<size/4;j++) {
                    indblkno=(int *)indbuf+j;
                    if (*indblkno==0)
                        continue;
                    get_block(fd, *indblkno, dindbuf);
                    for(k=0;k<size/4;k++) {
                        doindblkno=(int *)dindbuf+k;
                        if (*doindblkno==0)
                            continue;
                        bdealloc(mnt, *doindblkno);
                    }
                    bdealloc(mnt, *indblkno);
                }
                bdealloc(mnt, *blkno);
            }
            bdealloc(mnt, inodeP->i_block[14]);
        }
        break;
    }
    touch_ino(inodeP);
    inodeP->i_size=0;
    FREE_3(buf, indbuf, dindbuf);
}
