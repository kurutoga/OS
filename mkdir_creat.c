int make_dir(char *pathname) {

    char *parent, *child;
    int pino, dev;
    MINODE *pip;

    child=basename(pathname);
    parent=dirname(strdup(pathname));

    dev=running->cwd->dev;

    pino=getino(&dev, parent);
    pip=iget(dev, pino);
    if (!S_ISDIR(pip->inode.i_mode))
        return 0;

    if (search(pip, child)!=0) {
        printf("pathname already exists in fs!\n");
        return 0;
    }
    mymakedir(pip, child);
    pip->inode.i_links_count+=1;
    touch_ino(&(pip->inode));
    pip->dirty=true;
    iput(pip);
}

mymakedir(MINODE *pip, char *name) {

    int ino, bno, i, dev;
    MINODE *mip;
    INODE *inP;
    MOUNT *mnt;
    DIR *direc;
    char *dbuf;

    mnt=(MOUNT *)find_mount(pip->dev);
    if(mnt==0)
        return-1;

    i=mnt->blksize;
    dev=mnt->dev;
    dbuf=malloc(i);
    ino=ialloc(mnt);
    bno=balloc(mnt);

    mip=iget(dev, ino);
    inP=&(mip->inode);
    
    inP->i_mode=0x41ED;
    inP->i_uid=running->uid;
    inP->i_gid=running->gid;
    inP->i_size=mnt->blksize;
    inP->i_links_count=2;
    inP->i_atime=inP->i_ctime=inP->i_mtime=time(0L);
    inP->i_block[0]=bno;
    for(i=1;i<15;i++)
        inP->i_block[i]=0;
    mip->dirty=1;
    iput(mip);

    bget_block(dev, inP->i_block[0], dbuf, mnt->blksize);
    direc=(DIR *)dbuf;
    direc->rec_len=12;
    direc->name_len=1;
    strcpy(direc->name, ".");
    direc->inode=ino;
    cp=dbuf+direc->rec_len;
    direc=(DIR *)cp;
    direc->rec_len=mnt->blksize;
    direc->name_len=2;
    direc->inode=pip->ino;
    strcpy(direc->name, "..");

    bput_block(pip->dev, inP->i_block[0], dbuf, mnt->blksize);

    enter_name(pip, ino, name);
    free(dbuf);
}

fill_dir(DIR *dp, int rl, int nl, int ino, char *name) {
    dp->rec_len=rl;
    dp->name_len=nl;
    dp->inode=ino;
    strcpy(dp->name, name);
}

int enter_name(MINODE *pip, int myino, char *myname) {
    
    int i, size, dev, need_length, remain_length, ideal_length, bno;
    int *blkno;
    MOUNT *mnt; 
    char *buffer, *indbuf;

    mnt=(MOUNT *)find_mount(pip->dev);
    if (mnt==0)
        return -1;
    size=mnt->blksize;
    dev=mnt->dev;
    buffer=malloc(size);
    indbuf=malloc(size);
    for (i=0;i<12;i++) {
        if(pip->inode.i_block[i]==0) {
            bno=balloc(mnt);
            pip->inode.i_size+=size;
            get_block(dev, bno, buffer);
            dp=(DIR *)buffer;
            cp=buffer;
            fill_dir(dp, size, strlen(myname), myino, myname);
            put_block(pip->dev, bno, buffer);
            pip->inode.i_block[i]=bno;
            FREE_2(buffer, indbuf);
            return;
        }
            
        get_block(pip->dev, pip->inode.i_block[i], buffer);
        dp=(DIR *)buffer;
        cp=buffer;
        
        while(cp + dp->rec_len < buffer + size) {
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
        ideal_length=4*((8 + dp->name_len + 3)/4);
        need_length=4*((8 + strlen(myname) + 3)/4);
        remain_length=dp->rec_len-ideal_length;
        if(remain_length>=need_length) {
            dp->rec_len=ideal_length;
            cp +=dp->rec_len;
            dp = (DIR *)cp;
            fill_dir(dp, remain_length, strlen(myname), myino, myname);
            put_block(pip->dev, pip->inode.i_block[i], buffer);
            FREE_2(buffer, indbuf);
            return;
        }
    }
    get_block(pip->dev, pip->inode.i_block[12], buffer);
    if (pip->inode.i_block[12]==0) {
        bno=balloc(mnt);
        *blkno=bno;
        put_block(pip->dev, pip->inode.i_block[12], buffer);
        get_block(pip->dev, *blkno, buffer);
        dp=(DIR *)buffer;
        fill_dir(dp, size, strlen(myname), myino, myname);
        put_block(pip->dev, *blkno, buffer);
        pip->inode.i_size+=size;
        FREE_2(buffer, indbuf);
        return;
    }
    for(i=0;i<size/4;i++) {
        blkno=(int *)buffer+i;
        if(*blkno==0) {
            bno=balloc(mnt);
            *blkno=bno;
            put_block(pip->dev, pip->inode.i_block[12], buffer);
            get_block(pip->dev, *blkno, buffer);
            dp=(DIR *)buffer;
            fill_dir(dp, size, strlen(myname), myino, myname);
            put_block(pip->dev, *blkno, buffer);
            pip->inode.i_size+=size;
            FREE_2(buffer, indbuf);
            return;
        }

        get_block(pip->dev, *blkno, indbuf);
        dp=(DIR *)indbuf;
        cp=indbuf;
        while(cp + dp->rec_len < buffer+size) {
            cp+=dp->rec_len;
            dp=(DIR *)cp;
        }
        ideal_length=4*((8 + dp->name_len + 3)/4);
        need_length=4*((8 + strlen(myname) + 3)/4);
        remain_length=dp->rec_len-ideal_length;
        if(remain_length>=need_length) {
            dp->rec_len=ideal_length;
            cp += dp->rec_len;
            dp = (DIR *)cp;
            fill_dir(dp, remain_length, strlen(myname), myino, myname);
            put_block(pip->dev, pip->inode.i_block[i], indbuf);
            FREE_2(buffer, indbuf);
            return;
        }
    }
    printf(RED"DISK FULL! Cannot Write More Entries...!\n"RESET);
    FREE_2(buffer, indbuf);
    return 0;
}

touch_ino(INODE *inodePtr) {
    inodePtr->i_ctime=time(0);
    inodePtr->i_mtime=time(0);
}

int creat_file(char *pathname) {
    char *parent, *child;
    int pino,dev;
    MINODE *pip;

    child=basename(pathname);
    parent=dirname(strdup(pathname));
    
    dev=running->cwd->dev;
    pino=getino(&dev, parent);
    pip=iget(dev, pino);

    if (!S_ISDIR(pip->inode.i_mode))
        return 0;

    if (search(pip, child)!=0) {
        printf(RED"pathname already exists in fs!\n"RESET);
        return 0;
    }

    my_creat(pip, child);
    touch_ino(&(pip->inode));
    pip->dirty=true;
    iput(pip);

}

int my_creat(MINODE *pip, char *name) {

    int ino, bno, i;
    MINODE *mip;
    INODE *inP;
    DIR *direc;
    MOUNT *mnt;

    mnt=(MOUNT *)find_mount(pip->dev);
    ino=ialloc(mnt);

    mip=iget(pip->dev, ino);
    inP=&(mip->inode);
    
    inP->i_mode=0x81A4;
    inP->i_uid=running->uid;
    inP->i_gid=running->gid;
    inP->i_size=0;
    inP->i_links_count=1;
    inP->i_atime=inP->i_ctime=inP->i_mtime=time(0L);
    inP->i_block[0]=0;
    for(i=1;i<15;i++)
        inP->i_block[i]=0;
    mip->dirty=1;
    iput(mip);

    enter_name(pip, ino, name);
}
