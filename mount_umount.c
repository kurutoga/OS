int mount_dev(char *cmd) {

    int i, fs, ino, j, dev;
    char filesys[256], mount_point[256];
    MINODE *mip;
    if (strcmp(cmd, "")==0) {
        printf("Enter filesys mountpoint [ENTER for display]\n");
        fgets(cmd, 512, stdin);
    }
    if (strcmp(cmd, "\n")==0) {
        for(i=0;i<NMOUNT;i++) {
            if (mount[i].dev==0)
                continue;
            printf(MAGENTA "%s is mounted at %s\n" RESET, mount[i].name, mount[i].path_name);
        }
        return 0;
    }
    sscanf(cmd, "%s %s", filesys, mount_point);
    if ((strcmp(filesys, "\n")==0 || strcmp(mount_point, "")==0)) {
        printf(RED"Invalid Syntax!\n"RESET);
        return -1;
    }
    for (i=0;i<NMOUNT;i++) {
        if (mount[i].dev==0)
            continue;
        if (strcmp(filesys, mount[i].name)==0) {
            printf(RED"%s is already mounted at %s\n"RESET, filesys, mount[i].path_name);
            return -1;
        }
    }
    for (i=0;i<NMOUNT;i++) {
        if (mount[i].dev==0) {
            fs = open(filesys, O_RDWR);
            if (fs<=0) {
                printf(RED"Cannot open filesys!\n"RESET);
                return -1;
            }
            dev=root->dev;
            ino=getino(&dev, mount_point);
            if (ino==0) {
                printf(RED"Invalid Mount Point!\n"RESET);
                return -1;
            }
            mip=iget(dev, ino);
            if (!(S_ISDIR(mip->inode.i_mode))) {
                printf(RED"Mount point is not a directory!\n"RESET);
                return -1;
            }
            for (j=0;j<NPROC;j++) {
                if (P[j].cwd==mip) {
                    printf(RED"Mount point is busy!\n"RESET);
                    return -1;
                }
            }
            mount[i].dev=fs;
            init_mount(&mount[i], filesys, mount_point);
            mount[i].mounted_inode=mip;
            mip->mounted=true;
            mip->mountptr=&mount[i];
            break;
        }
    }
    return 0;
}

int umount_dev(char *cmd) {
    int i, g;
    MINODE *mip;

    for (i=0;i<NMOUNT;i++) {
        if(mount[i].dev==0)
            continue;
        if (strcmp(mount[i].name, cmd)==0) {
            if(mount[i].busy==true) {
                printf(RED"Mounted fs is busy!\n"RESET);
                return -1;
            }
            mip=mount[i].mounted_inode;
            if(mip->mounted==false) {
                printf(RED"Filesystem is not mounted!\n"RESET);
                return -1;
            }
            sync_mount_desc(&mount[i]);
            for (g=0;g<=mount[i].ngroups;g++) {
                free(mount[i].gdesc[g]);
            }
            free(mount[i].gdesc);
            mount[i].dev=0;
            mount[i].busy=false;
            mip->mounted=false;
            iput(mip);
            break;
        }
    }
    return 0;
}
