int touch (char *pathname) {
    MINODE *mip;
    int ino, dev;
    MOUNT *mnt;
    
    dev=running->cwd->dev;
    ino=getino(&dev, pathname);
    if(ino==0) {
        printf(RED "Invalid path\n" RESET);
        return -1;
    }
    mip=iget(dev, ino);
    mip->inode.i_mtime=time(0L);
    mip->dirty=true;
    mnt=(MOUNT *)find_mount(dev);
    sync_mount_atime(mnt);
    iput(mip);
}

int chown(char *cmd) {
    int newOwner, ino, dev;
    char path[1024], own[10];
    MINODE *mip;
    MOUNT *mnt;

    sscanf(cmd, "%s %s", own, path);
    newOwner=atoi(own);
    dev=running->cwd->dev;
    ino=getino(&dev, path);
    if(ino==0) {
        printf(RED "invalid path\n" RESET);
        return -1;
    }
    mip=iget(dev, ino);
    mip->inode.i_uid=newOwner;
    mip->dirty=true;
    mnt=(MOUNT *)find_mount(dev);
    sync_mount_atime(mnt);
    iput(mip);
    return;
}

int chgrp(char *cmd) {
    int newGroup, ino, dev;
    char path[1024], grp[10];
    MINODE *mip;
    MOUNT *mnt;

    sscanf(cmd, "%s %s", grp, path);
    newGroup=atoi(grp);
    dev=running->cwd->dev;
    ino = getino(&dev, path);
    if (ino==0) {
        printf(RED "invalid path\n" RESET);
        return -1;
    }
    mip = iget(dev, ino);
    mip->inode.i_gid=newGroup;
    mip->dirty=true;

    mnt=(MOUNT *)find_mount(dev);
    sync_mount_atime(mnt);
    iput(mip);
    return;
}

int chmod_file(char *cmd) {
    int newMod, ino, dev;
    char path[1024], mod[10];
    MINODE *mip;
    MOUNT *mnt;

    sscanf(cmd, "%s %s", mod, path);
    newMod=strtol(mod, NULL, 0);
    dev=running->cwd->dev;
    ino=getino(&dev, path);
    if (ino==0) {
        printf(RED "invalid path\n" RESET);
        return -1;
    }
    mip=iget(dev, ino);
    mip->inode.i_mode=newMod;
    mip->dirty=true;
    mnt=(MOUNT *)find_mount(dev);
    sync_mount_atime(mnt);
    iput(mip);
}

int clear (char *bogus) {
    printf("\033[2J\033[H");
}

int menu (char *bogus) {
    printf(YELLOW "***************************************\n" RESET);
    printf(BOLDWHITE"CMD\t\tUSAGE\n"RESET);
    printf(GREEN"ls\t\tls [pathname]\n");
    printf("cd\t\tcd [pathname]\n");
    printf("pwd\t\tpwd\n");
    printf("creat\t\tcreat pathname\n");
    printf("mkdir\t\tmkdir pathname\n");
    printf("rmdir\t\trmdir pathname\n");
    printf("link\t\tlink oldName newName\n");
    printf("symlink\t\tsymlink pathname link\n");
    printf("unlink\t\tunlink pathname\n");
    printf("touch\t\ttouch pathname\n");
    printf("chmod\t\tchmod newMode pathname\n");
    printf("chgrp\t\tchgrp newGrp pathname\n");
    printf("chown\t\tchown newOwn pathname\n");
    printf("open\t\topen filename\n");
    printf("close\t\tclose filename\n");
    printf("read\t\tread filename nbytes\n");
    printf("write\t\twrite filename buffer\n");
    printf("lseek\t\tlseek filename offset\n");
    printf("mount\t\tmount [device path]\n");
    printf("umount\t\tumount device_path\n");
    printf("sync\t\tsync\n");
    printf("pm\t\tpm\n");
    printf("pfd\t\tpfd\n" RESET);
    printf(YELLOW "***************************************\n" RESET);
}

int pm(char *bogus)
{
      int i;    MINODE *ip;
      printf("[dev, inode]  refCount\n");
      for (i=0; i<NMINODES; i++){
          ip = &minode[i]; 
          if (ip->refCount > 0){
              printf("[%d %d]    %d\n", ip->dev, ip->ino, ip->refCount);
          }
      }
}

int quit(char *bogus)
{
       int i;
       if (running->uid != 0) {
           printf(RED "not SUPER user\n" RESET);
           return(-1);
       }
       for (i=0; i<NMINODES; i++){
           if (minode[i].refCount){
                minode[i].refCount = 1;
                iput(&minode[i]);
                   if (minode[i].mounted)
                       minode[i].refCount = 1;
                   else
                       minode[i].refCount = 0; 
           }
       }
       for (i=NMOUNT-1; i>=0; i--){
           if (mount[i].busy){
                printf("umount %s\n",mount[i].name);
                umount(mount[i].name);
           }
       }
       exit(0);
} 

int sync(char *bogus)
{
    int i, save;
    MINODE *mip;
    printf(YELLOW "flush in-core inodes to disk .....\n" RESET);
    for (i=0; i<NMINODES; i++){
        mip = &minode[i];
        if (mip->refCount > 0){
            printf(WHITE "flush minode[%d] : [%d %d]\n" RESET, \
                    i, mip->dev,\
                    mip->ino); 
            save =  mip->refCount;
            mip->refCount = 1;
            iput(mip);
            mip->refCount = save;
        }
    }
}
