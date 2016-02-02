int link(char *cmd) {
    char src[1024], dst[1024], *d_parent, *d_child;
    MINODE *sip, *d_pip;
    int sino, d_pino, dev;

    sscanf(cmd, "%s %s", src, dst);
    if (strlen(src)==0 || strlen(dst)==0) {
        printf(RED "Invalid Syntax!\n" RESET);
        return -1;
    }
    dev=running->cwd->dev;
    sino=getino(&dev, src);
    sip=iget(dev, sino);

    if (!S_ISREG(sip->inode.i_mode) && !S_ISLNK(sip->inode.i_mode))
        return -1;
    d_pino=getino(&dev, dst);
    if(d_pino!=0) {
        printf(RED "Path already exists!\n" RESET);
        return -1;
    }
    
    d_child=basename(dst);
    d_parent=dirname(strdup(dst));

    d_pino=getino(&dev, d_parent);
    d_pip=iget(dev, d_pino);

    if (!S_ISDIR(d_pip->inode.i_mode)) {
        printf(RED "Invalid Path (NOT DIR)\n" RESET);
        return -1;
    }
    
    enter_name(d_pip, sino, d_child);
    d_pip->inode.i_links_count+=1;

    d_pip->dirty=true;
    sip->dirty=true;
    iput(d_pip);
    iput(sip);

    return;
}

int symlink(char *cmd) {

    int sino, dino, dev, sdev;
    MINODE *sip, *dip;
    char src[64], dst[1024], *block;
    sscanf(cmd, "%s %s", src, dst);

    sdev=running->cwd->dev;
    sino=getino(&sdev, src);
    if(sino==0) {
        printf(RED "path doesn't exist!\n" RESET);
        return -1;
    }
    sip=iget(sdev, sino);
    if(!S_ISDIR(sip->inode.i_mode) && !S_ISREG(sip->inode.i_mode)) {
        printf(RED "path is not a valid dir/file\n" RESET);
        iput(sip);
        return -1;
    }
    creat_file(dst);
    dev=running->cwd->dev;
    dino=getino(&dev, dst);
    dip=iget(dev, dino);
    dip->inode.i_mode=0xA000;
    block=(char *)dip->inode.i_block;
    strcpy(block, src);
    dip->dirty=true;
    iput(sip);
    iput(dip);
    return;
}

int readlink(char *pathname) {

    MINODE *mip;
    int ino, content, dev;

    dev=running->cwd->dev;
    ino=getino(&dev, pathname);
    mip=iget(dev, ino);
    if (!S_ISLNK(mip->inode.i_mode)) {
        printf(RED "Not a Symbolic Link\n" RESET);
        iput(mip);
        return;
    }
    content=mip->inode.i_block[0];
    printf("LINK: %s\n", (char *)&content);
    iput(mip);
}
