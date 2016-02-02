int open_file(char *pathname) {

    int md, ino, dev, i,j;
    char mode[6], path[1024];
    MINODE *mip;
    OFT *file, *oftp;

    sscanf(pathname, "%s %s", path, mode);
    md=strtol(mode, NULL, 0);

    if(md==0) {
          if(strcmp(mode, "RD")==0)
              md=0;
          else if (strcmp(mode, "WR")==0)
              md=1;
          else if (strcmp(mode, "RW")==0)
              md=2;
          else if (strcmp(mode, "APPEND")==0)
              md=3;
          else if (strcmp(mode, "0")==0)
              md=0;
          else
              md=-1;
    }
    if (md==-1||md>3) {
        printf(RED"invalid mode!\n"RESET);
        return -1;
    }
    if (path[0]!='/')
        dev=running->cwd->dev;
    else
        dev=root->dev;
    ino = getino(&dev, path);
    if (ino <= 0) {
        if (md==2) {
        creat_file(path);
        ino = getino(&dev, path);
        } else {
            printf(RED"File Doesn't Exist!\n"RESET);
        }
    }
    mip = iget(dev, ino);
    if(!S_ISREG(mip->inode.i_mode)) {
        printf(RED"Not a file!\n"RESET);
        return -1;
    }
    for(i=0;i<NFD;i++) {
        file = running->fd[i];
        if (!file) 
            continue;
        if(file->ptr==mip) {
            if(file->mode!=0) {
                printf(RED"File already open in incompatible mode!\n"RESET);
                return -1;
            }
        }
    }
    for(i=0;i<NOFT;i++) {
        if (ftable[i].refCount==0) {
            ftable[i].mode=md;
            ftable[i].ptr=(MINODE *)mip;
            oftp = &(ftable[i]);
            for(j=0;j<NFD;j++) {
                if (running->fd[j]==0) {
                   running->fd[j]=(OFT *)&(ftable[i]); 
                   break;
                }
            }
        break;
         }
    }
    oftp->mode=md;
    oftp->refCount=1;
    oftp->ptr=(MINODE *)mip;

    switch(md) {
        case 0:
            oftp->offset=0;
            break;
        case 1:
            trucate(mip);
            mip->dirty=true;
            oftp->offset=0;
            break;
        case 2:
            mip->dirty=true;
            oftp->offset=0;
            break;
        case 3:
            mip->dirty=true;
            oftp->offset=mip->inode.i_size;
            break;
        default:
            printf(RED"invalid mode!\n"RESET);
            return -1;
    }
    touch(path);
    return i;
}

int trucate (MINODE *mip) {
    truncate(mip);
    mip->dirty=true;
    printf("File %d truncated ... \n", mip->ino);
 }

int close_file(char *cmd) {
    OFT *oftp;
    MINODE *mip;
    int fd, ino, i, dev;
    fd=strtol(cmd,NULL, 0);

    if (fd < 0 || fd > NFD) {
        printf(RED"Invalid File Descriptor!\n"RESET);
        return -1;
    }
    if (fd==0) {
        if (strcmp(cmd, "0")!=0) {
            dev=running->cwd->dev;
            ino=getino(&dev, cmd);
            if (ino==0) {
                printf(RED"Invalid filename!\n"RESET);
                return -1;
            }
            for(i=0;i<NFD;i++) {
                if (running->fd[i]==0)
                    continue;
                if (running->fd[i]->ptr->ino==ino && running->fd[i]->refCount>0) {
                    fd=i;
                    break;
                }
            }
        }
    }
    if ((running->fd[fd])==0) {
        printf(RED"File Does Not Exist!"RESET);
        return -1;
    }
    oftp=running->fd[fd];
    running->fd[fd]=0;
    oftp->refCount--;
    if (oftp->refCount > 0) return 0;
    
    mip = oftp->ptr;
    iput(mip);
    printf("Closed %d FD\n", fd);

    return 0;
}

int lseek_file(char *cmd) {
    OFT *oftp;
    MINODE *mip;
    char fil[2], pos[20];
    int originalPos, fd, ino, i, dev;
    long position;

    sscanf(cmd, "%s %s", fil, pos);
    fd=strtol(fil, NULL, 0);
    if (fd==0) {
        dev=running->cwd->dev;
        if (strcmp(fil, "0")!=0) {
          ino=getino(&dev, fil);
          if (ino==0) {
              printf(RED"Invalid filename!\n"RESET);
              return -1;
          }
          for(i=0;i<NFD;i++) {

              if (running->fd[i]==0)
                  continue;
              if (running->fd[i]->ptr->ino==ino && running->fd[i]->refCount>0) {
                  fd=i;
                  break;
              }
          }
       }
    }
    position=strtol(pos, NULL, 0);
    if (fd<0 || fd > NFD) {
        printf(RED"Invalid file descriptor!\n"RESET);
        return -1;
    }
    if (running->fd[fd]==0) {
        printf(RED"Invalid File!\n"RESET);
        return -1;
    }
    oftp=running->fd[fd];
    mip=oftp->ptr;
    if (position < 0 || (mip->inode.i_size)<position) {
        printf(RED"position is out of bounds!\n"RESET);
        return -1;
    }
    originalPos=oftp->offset;
    oftp->offset=position;
    mip->dirty=true;
    return originalPos;
}

int pfd_file(char *cmd) {

    int i,offset, dev, ino, fd;

    printf(BOLDWHITE "fd\tmode\toffset\tINODE\n" RESET);
    for(i=0;i<NFD;i++) {
        fd=i;
        
        if(running->fd[i]) {
            offset=running->fd[i]->offset;
            dev=running->fd[i]->ptr->dev;
            ino=running->fd[i]->ptr->ino;

            printf(YELLOW "%-4d\t" RESET, fd);
            switch(running->fd[i]->mode) {
                case 0:
                    printf(MAGENTA "READ\t" RESET);
                    break;
                case 1:
                    printf(CYAN "WRITE\t" RESET);
                    break;
                case 2:
                    printf(BLUE "RW\t" RESET);
                    break;
                case 3:
                    printf(WHITE "APPEND\t" RESET);
                    break;
                default:
                    break;
            }
            printf(YELLOW "%d\t[%d, %d]\n" RESET, offset, dev, ino);
        }
    }
}



