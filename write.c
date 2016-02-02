int write_file(char *cmd) {
    char file[1024],*buffer,*temp;
    int fd,ino,i,nbytes,dev;
    MINODE *mip;
    MOUNT *mnt;

    mnt=(MOUNT *)find_mount(running->cwd->dev);
    i=(mnt->blksize);
    buffer=(char *)malloc(i);
    temp=(char *)malloc(i);

    if (strcmp(cmd, "")==0) {
        printf("Enter filename/fd: ");
        fgets(cmd, 512, stdin);
    }
    sscanf(cmd, "%s", file);
    fd=strtol(file, NULL, 0);
    if (fd==0) {
        if (strcmp(file, "0")!=0) {
            dev=running->cwd->dev;
            ino=getino(&dev, file);
            for(i=0;i<NFD;i++) {
                if (running->fd[i]==0)
                    continue;
                if (running->fd[i]->ptr->ino==ino && running->fd[i]->refCount>0) {
                    fd=i;
                    break;
                }
            }
            if (fd!=i) {
                printf("FILE not found in Open File Table!\n");
                FREE_2(buffer, temp);
                return -1;
            }
        }
    }
    if (running->fd[fd]->mode==0) {
        printf("File opened in RD mode. Cannot continue\n");
        FREE_2(buffer, temp);
        return -1;
    }
    printf(BOLDBLUE "%s OPEN. Write Line: " RESET, file);
    fgets(temp, mnt->blkize, stdin);
    strncpy(buffer, temp, strlen(temp)-1);
    nbytes=strlen(buffer);
    free(temp);
    return (mywrite(fd, buffer, nbytes));
}

int mywrite(int fd, char buffer[], int nbytes) {

    long *tmp, blk, lblk, ilblk, dlblk, tlblk, size, start, bsize, fsize, *bno, *ibno, *dibno, remain, count, offset;
    char *wbuf, *buf, *inbuf, *dinbuf, *cq;
    OFT *oftp;
    MINODE *mip;
    MOUNT *mnt;

    if (fd<0 || fd>NFD) {
        printf("Invalid FD\n");
        return -1;
    }
    oftp=running->fd[fd];
    mip=oftp->ptr;
    mnt=(MOUNT *)find_mount(mip->dev);
    size=mnt->blksize;
    wbuf=malloc(size);
    buf=malloc(size);
    inbuf=malloc(size);
    dinbuf=malloc(size);

    bsize=size/4;
    fsize=(size/4)*(size/4);
    count=0;
    while (nbytes > 0) {

        lblk = oftp->offset/size;
        start = oftp->offset%size;

        if (lblk<12) {
            if (mip->inode.i_block[lblk]==0) 
                mip->inode.i_block[lblk]=balloc(mnt);
            *bno = mip->inode.i_block[lblk];
        } else if (lblk>=12 && lblk< bsize+12) {
            ilblk=lblk-12;
            if (mip->inode.i_block[12]==0)
                mip->inode.i_block[12]=balloc(mnt);
            get_block(mip->dev, mip->inode.i_block[12], buf);
            bno=(long *)buf+ilblk;
            if (*bno==0) {
                *bno=balloc(mnt);
                put_block(mip->dev, mip->inode.i_block[12], buf);
            }
        } else if (lblk>=(bsize+12) && lblk<(fsize+bsize+12)) {
            dlblk=(lblk-12)-(bsize);
            ilblk=dlblk/bsize;
            blk=dlblk%bsize;
            if (mip->inode.i_block[13]==0)
                mip->inode.i_block[13]=balloc(mnt);
            get_block(mip->dev, mip->inode.i_block[13], inbuf);
            ibno=(long *)inbuf+ilblk;
            if (*ibno==0) {
                *ibno=balloc(mnt);
                put_block(mip->dev, mip->inode.i_block[13], inbuf);
            }
            get_block(mip->dev, *ibno, buf);
            bno=(long *)buf+blk;
            if (*bno==0) {
                *bno=balloc(mnt);
                put_block(mip->dev, *ibno, buf);
            }
        } else {
            tlblk=(lblk-12)-(bsize)-(fsize);
            dlblk=tlblk/fsize;
            ilblk=(tlblk%fsize)/bsize;
            blk=(tlblk%fsize)%bsize;

            if (mip->inode.i_block[14]==0)
                mip->inode.i_block[14]=balloc(mnt);
            get_block(mip->dev, mip->inode.i_block[14], dinbuf);
            dibno=(long *)dinbuf+dlblk;
            if (*dibno==0) {
                *dibno=balloc(mnt);
                put_block(mip->dev, *dibno, dinbuf);
            }
            get_block(mip->dev, *dibno, inbuf);
            ibno=(long *)inbuf+ilblk;
            if (*ibno==0) {
                *ibno=balloc(mnt);
                put_block(mip->dev, *dibno, inbuf);
            }
            get_block(mip->dev, *ibno, buf);
            bno=(long *)buf+blk;
            if (*bno==0) {
                *bno=balloc(mnt);
                put_block(mip->dev, *ibno, buf);
            }
        }
        get_block(mip->dev, *bno, wbuf);
        cp = wbuf + start;
        remain = size-start;
        cq = buffer;
        if (remain < nbytes) {
            memcpy(cp, cq, remain);
            nbytes -= remain; 
            running->fd[fd]->offset += remain;
            if(running->fd[fd]->offset > mip->inode.i_size)
                mip->inode.i_size += remain;
            remain -= remain;
        } else {
            memcpy(cp, cq, nbytes);
            remain -= nbytes; 
            running->fd[fd]->offset += nbytes;
            if(running->fd[fd]->offset > mip->inode.i_size)
                mip->inode.i_size += nbytes;
            nbytes -= nbytes;
        }
        put_block(mip->dev, *bno, wbuf);
    }
    mip->dirty=true;
    printf("wrote %d char into file descriptor fd=%d\n", count, fd);
    FREE_4(wbuf,buf,inbuf,dinbuf);
    sync_mount_desc(mnt);
    return count;
}

int cp_file(char *cmd) {
    
    int fd, gd, n;
    char src[1024], dst[1024], *buffer, temp[10];
    MOUNT *mnt;

    sscanf(cmd, "%s %s", src, dst);
    if (strcmp(src, "")==0 || strcmp(dst, "")==0) {
        printf("Invalid Args!\n");
        return -1;
    }
    strcat(src, " 0");
    strcat(dst, " 2");
    fd = open_file(src);
    if (fd<0) {
        printf("Cannot open source for read!\n");
        return -1;
    }
    gd = open_file(dst);
    if (gd<0) {
        printf("Cannot open dest for write!\n");
        return -1;
    }
    mnt=(MOUNT *)find_mount(runnind->fd[fd]->ptr->dev);

    buffer=malloc(mnt->blksize);

    while (n=myread(fd, buffer, mnt->blksize)) {
        mywrite(gd, buffer, n);
    }
    sprintf(temp, "%d", fd);
    close_file(temp);
    sprintf(temp, "%d", gd);
    close_file(temp);
    FREE_1(buffer);
}

int mv(char *cmd) {

    int ino, dino, dev, cdev, isOnSameDev;
    MINODE *mip, *dip;
    char src[1024], dst[1024], *d_parent, *d_child;

    sscanf(cmd, "%s %s", src, dst);

    if (strcmp(src, "")==0 || strcmp(dst, "")==0) {
        printf("Invalid Args!\n");
        return -1;
    }
    
    d_child=basename(dst);
    d_parent=dirname(strdup(dst));

    dev=running->cwd->dev;
    printf("D1=%d\n", dev);
    dino=getino(&dev, d_parent);
    if (dino==0) {
        printf("Destination folder doesn't exist!\n");
        return -1;
    }
    dip=iget(dev, dino);
    printf("D2=%d\n", dev);
    if (dip->mounted==true)
        printf("ADAD");
    cdev=running->cwd->dev;
    ino=getino(&cdev, src);
    if (ino==0) {
        printf("Source Doesn't Exist!\n");
        return -1;
    }
    mip=iget(cdev, ino);
    if (mip==0) {
        printf("Failed to allocate memory\n");
        return -1;
    }
    isOnSameDev = (dip->dev==mip->dev ? 1: 0);
    iput(mip);
    iput(dip);
    if (isOnSameDev) {
        link(cmd);
        unlink(src);
    } else {
        cp_file(cmd);
        unlink(src);
    }
    return 0;
}
