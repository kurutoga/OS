int read_file(char *cmd) {

    int fd,i,nbytes,ino,count,dev;
    char *buffer, file[1024], nbt[12];
    OFT *oftp;
    MINODE *mip;
    MOUNT *mnt;
    
    mnt=(MOUNT *)find_mount(running->cwd->dev);
    i=mnt->blksize;
    buffer=malloc(i);
    memset(buffer, 0, i);
    bzero(file, 1024);
    bzero(nbt, 12);
    if (strcmp(cmd, "")==0) {
        printf("Enter filename/fd [nbytes=1]: ");
        fgets(cmd, 512, stdin);
    }
    sscanf(cmd, "%s %s", file, nbt);
    fd=strtol(file, NULL, 0);
    nbytes=strtol(nbt, NULL, 0);
    if (fd==0) {
        if (strcmp(file, "0")!=0) {
            dev=running->cwd->dev;
            ino = getino(&dev, file);
            mip = iget(dev, ino);
            for (i=0;i<NFD;i++) {
                oftp=running->fd[i];
                if (oftp==0)
                    continue;
                if (oftp->ptr->ino==ino && oftp->refCount>0) {
                    fd=i;
                    break;
                }
            }
            if (mip->ino!=ino) {
                printf(RED"File is not open!\n"RESET);
                free(buffer);
                return -1;
            }
            iput(mip);
        }
    }
    if (oftp->mode!=0 && oftp->mode!=2) {
        printf(RED"File is open in incompatible mode!\n"RESET);
        free(buffer);
        return -1;
    }
    count=(myread(fd, buffer, nbytes));
    buffer[count]=0;
    myprintf("READ BUFFER: %s\n", buffer);
    free(buffer);
    return count;
}

int myread(int fd, char buffer[], int nbytes) {

    int i, size, lblk, *bno, *ibno, *dbno, start, remain, avil, file_size, offset, count=0;
    OFT *oftp;
    char *cq, *buf, *inbuf, *dinbuf, *readbuf;
    MINODE *mip;
    MOUNT *mnt;

    cq=buffer;

    if (fd < 0 || fd >NFD) {
        printf(RED"Invalid FD\n"RESET);
        return -1;
    }
    oftp=running->fd[fd];
    mip=oftp->ptr;
 
    mnt=find_mount(mip->dev);

    size=mnt->blksize;
    buf=malloc(size+1);
    inbuf=malloc(size+1);
    dinbuf=malloc(size+1);
    readbuf=malloc(size+1);

    offset=oftp->offset;
    file_size=oftp->ptr->inode.i_size;
    avil=file_size-offset;
    printf("* fd=%d, size=%d avil=%d nbytes=%d\n", fd, file_size, avil, nbytes);
    while(nbytes && avil) {
        lblk=(oftp->offset)/size;
        start=(oftp->offset)%size;
        if (lblk<12) {
            bno =&(mip->inode.i_block[lblk]);
        } else if (lblk>=12 && lblk<((size/4)+12)) {
            bget_block(mip->dev, mip->inode.i_block[12], buf, size);
            bno = (int *)buf+(lblk-12);
        } else if (lblk>=((size/4)+12) && lblk<(((size/4)*(size/4))+(size/4)+12)) {
            bget_block(mip->dev, mip->inode.i_block[13], buf, size);
            lblk=((lblk-12) - (size/4));
            ibno = (int *)buf + (lblk/size);
            bget_block(mip->dev, *ibno, inbuf, size);
            lblk%=(size/4);
            bno=(int *)inbuf + lblk;
        } else {
            bget_block(mip->dev, mip->inode.i_block[14], buf, size);
            lblk=((lblk-12) - ((size/4)*(size/4)+(size/4)));
            dbno=(int *)buf + (lblk/((size/4)*(size/4)));
            bget_block(mip->dev, *dbno, dinbuf, size);
            lblk%=((size/4)*(size/4));
            ibno=(int *)buf + lblk/(size);
            bget_block(mip->dev, *ibno, inbuf, size);
            lblk%=(size/4);
            bno=(int *)inbuf + lblk;
        }

        bget_block(mip->dev, *bno, readbuf, size);
        cp = readbuf + start;
        remain = size - start;
        if(avail >= size && remain == size && nbytes >= size) {
        strncpy(cq, cp, size);
        running->fd[fd]->offset += size;
        count += size; avail -= size; nbytes -= size; remain -= size;
        } else if (remain <= avil && remain <= nbytes) {
            memcpy(cq, cp, remain);
            running->fd[fd]->offset += remain;
            count += remain; 
            avil -= remain; 
            nbytes -= remain; 
            remain -= remain;
        } else {
            memcpy(cq, cp, avil);
            running->fd[fd]->offset += avil;
            count += avil; 
            avil -= avil; 
            nbytes -= avil; 
            remain -= avil;
        }
    }
        printf("myread : read %d char from file descriptor %d\n", count, fd);
        FREE_4(buf, inbuf, dinbuf, readbuf);
        return count;
}

int cat_file(char *pathname) {

    char *mybuf, file[3];
    int n, fd, size;
    MOUNT *mnt;

    strcat(pathname, " RD");
    fd = open_file(pathname);
    mnt=(MOUNT *)find_mount(running->cwd->dev);
    size=mnt->blksize;
    mybuf=malloc(size+1);

    while(n=myread(fd, mybuf, size)) {
        mybuf[n]=0;
        myprintf("%s", mybuf);
    }
    sprintf(file, "%d", fd);
    close_file(file);
    free(mybuf);
}
