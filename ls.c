int ls (char *pathname) {
    int ino, dev;
    MINODE *mip;
    if (strcmp(pathname, "")==0) {
        mip=running->cwd;
        list_dir(mip);
        return;
    }
    dev=running->cwd->dev;
    ino = getino(&dev, pathname);
    mip=iget(dev, ino);
    if (S_ISREG(mip->inode.i_mode))
        list_file(mip, basename(pathname));
    else
        list_dir(mip);
    iput(mip);
}

int list_file(MINODE *mip, char *name) {
    
    int i; 
    char *t1 = "xwrxwrxwr-------";
    char t2 = '-';
    char *time;

    ip=&mip->inode;  
    printf("%-4d", mip->ino);
    if ((ip->i_mode & 0xF0000)== 0x8000)
		printf(YELLOW "%c" RESET, '-');
	if ((ip->i_mode & 0xF0000)== 0x4000)
		printf(YELLOW "%c" RESET, 'd');
	if ((ip->i_mode & 0xF0000)== 0xA000)
		printf(YELLOW"%c"RESET, 'l');

	for (i=8;i>=0;i--) {
		if (ip->i_mode & (1<<i))
			printf(YELLOW"%c"RESET, t1[i]);
		else
			printf(YELLOW"%c" RESET, t2);
	}
    time=ctime((const time_t *)&ip->i_ctime);
    time[strlen(time)-1]='\0';
    printf(" %-4d%-4d%-4d%-10d", ip->i_links_count, ip->i_uid, ip->i_gid, ip->i_size ); 
    printf(" %s ", time);
    if(S_ISDIR(ip->i_mode))
        printf(BOLDRED " %s " RESET, name);
    else if (S_ISLNK(ip->i_mode))
        printf(BLUE " %s " RESET, name);
    else if (S_ISREG(ip->i_mode)) {
        if (strncmp(name, ".", 1)==0)
            printf(BOLDBLUE " %s " RESET, name);
        else 
            printf(BOLDGREEN " %s " RESET, name);
    } else
        printf(" %s ", name);
    if (S_ISLNK(ip->i_mode))
        printf(BOLDGREEN " -> %s\n" RESET, (char *)ip->i_block);
    printf("\n");
    return;
}

int list_dir(MINODE *mip) {

    int i,j,*blkno,*indirect,size;
    char *lbuf, *name;
    MINODE *cip;
    MOUNT *mnt;
    ip = &(mip->inode);
    
    mnt=(MOUNT *)find_mount(mip->dev);
    size=mnt->blksize;
    name=malloc(size);
    lbuf=malloc(size);

    for (i=0;i<12;i++) {
        if (ip->i_block[i]==0) {
            FREE_2(name, lbuf);
            return;
        }
        get_block(mip->dev, mip->inode.i_block[i], lbuf);
        dp=(DIR *)lbuf;
        cp=lbuf;

        while(cp<(lbuf+size) && dp->rec_len>0) {
            cip=iget(mip->dev, dp->inode);
            strncpy(name, dp->name, dp->name_len);
            name[dp->name_len]=0;
            list_file(cip, name);
            iput(cip);
            cp+=dp->rec_len;
            dp=(DIR *)cp;
        }
    }
    if (mip->inode.i_block[12]==0) {
        free(name);
        return;
    }
    get_block(mip->dev, mip->inode.i_block[12], lbuf);
    for (i=0;i<size/4;i++) {
        blkno=(int *)lbuf+i;
        if(*blkno=0) {
            FREE_2(name, lbuf);
            return;
        }
        get_block(mip->dev, *blkno, lbuf);
        dp=(DIR *)lbuf;
        cp=lbuf;

        while(cp<lbuf+size && dp->rec_len>0) {
            cip=iget(mip->dev, mip->ino);
            strncpy(name, dp->name, dp->name_len);
            name[dp->name_len]=0;
            list_file(cip, name);
            iput(cip);
            cp+=dp->rec_len;
            dp=(DIR *)cp;
        }
    }
    FREE_2(name, lbuf);
}
