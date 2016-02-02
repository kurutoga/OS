int cd (char *pathname) {
    int ino,dev,i;
    MINODE *mip;
    if (strlen(pathname)==0) {
        printf("CD // /n");
        running->cwd=root;
        return;
    }
    dev=running->cwd->dev;
    ino = getino(&dev, pathname);
    mip = iget(dev, ino);
    if (S_ISDIR(mip->inode.i_mode)) {
        iput(running->cwd);
        printf("Found DIR. Changing to INODE=%d on FD[%d]\n", mip->ino, mip->dev);
        if (dev!=root->dev)
            printf(BOLDYELLOW "[MOUNTED FS]\n" RESET);
        running->cwd=mip;
        pwd("");
    } else {
        printf("Not a DIR!\n");
        iput(mip);
    }
}

int pwd(char *pathname) {
        char *path;
        path=malloc(1024);
        get_cwd(path);
        printf("CWD=%s\n", path);
        free(path);
}

get_cwd(char *path) {

    int parentIno, currentIno, i=0,m,dev,ino;
    MINODE *mip;
    int *blkno, *indblkno;
    char *temp, *chunk, *mount_pt;

    temp=malloc(OFFSET/2);
    chunk=malloc(OFFSET/4);
    mount_pt=malloc(OFFSET/2);
    memset(temp, 0, OFFSET/2);
    memset(chunk, 0, OFFSET/4);
    memset(mount_pt, 0, OFFSET/2);

    mip=iget(running->cwd->dev, running->cwd->ino);
    parentIno=0;
    currentIno=running->cwd->ino;
    strcpy(temp, "/");
    strcpy(path, "");
    while(true) {
        findino(mip, &currentIno, &parentIno);
        if (currentIno==parentIno) {
          if (mip->dev==root->dev)
              break;
          for(m=0;m<NMOUNT;m++) {

              if (mount[m].dev==0)
                  continue;
              if (mount[m].dev==mip->dev) {
                  strcpy(temp, mount_pt);
                  strcat(mount_pt, mount[m].path_name);
                  strcat(mount_pt, path);
                  strcpy(path, mount_pt);
                  bzero(chunk, 128);
                  break;
              }
          }
          break;
        }
        iput(mip);
        mip=iget(running->cwd->dev, parentIno);
        findmyname(mip, currentIno, chunk);
        strcat(temp, chunk);
        strcat(temp, path);
        strcpy(path, temp);
        strcpy(temp, "/");
        bzero(chunk, 64);
    }
    strcat(path, "/");
    iput(mip);
    FREE_3(temp, chunk, mount_pt);
}
