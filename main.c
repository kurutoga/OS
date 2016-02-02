#include "print.c"
#include "util.c"
#include "ls.c"
#include "cd_pwd.c"
#include "mkdir_creat.c"
#include "rmdir.c"
#include "link.c"
#include "unlink.c"
#include "open_close.c"
#include "mount_umount.c"
#include "read.c"
#include "write.c"
#include "misc.c"

init() {
    int i,j;
    for (i=0;i<NPROC;i++) {
        P[i].pid=i;
        P[i].cwd=0;
        P[i].fd[NFD]=0;
        if (i==0 || i==1)
            P[i].uid=i;
    }
    P[0].nextProcPtr=(PROC *)&(P[1]);
    P[1].nextProcPtr=(PROC *)&(P[0]);
    running=&P[0];
    for (i=0;i<NMINODES;i++) {
        minode[i].refCount=0;
        minode[i].dev=0;
        minode[i].ino=0;
             root=0;
    }
    for (i=0;i<NFD;i++) {
        running->fd[i]=0;
    }
    for (i=0;i<NMOUNT;i++) {
        mount[i].dev=0;
    }
}

mount_root(char *fs) {
    int fd=0;
    printf("Opening FS..\n");
    fd=open(fs, O_RDWR);
    minode[0].dev=fd;
    mount[0].dev=minode[0].dev;
    init_mount(&mount[0], fs, "/");
    printf("Open SUCCESS.. Mounting root.\n");
    root=iget(fd, 2);
    mount[0].mounted_inode=root;
    root->mounted=true;
    running->cwd=iget(fd, 2);
    P[1].cwd=iget(fd, 2);
    printf("MOUNT COMPLETE. ENTER COMMAND\n\n");
}
char line[1024], cname[32], pathname[1024], dname[1024], cwd[1024];
char *command[32] = { "ls", "cd", "pwd", "mkdir", "creat", "rmdir", "link", "symlink", "readlink", \
                     "unlink", "touch", "chmod", "chown", "chgrp", "clear", "menu", "open", "close",\
                     "pfd", "lseek", "quit", "read", "cat", "write", "cp", "mv", "q", "exit", "mount", "umount", "sync", "pm", };
int (*funcPtr[32])(char*) = { ls, cd, pwd, make_dir, creat_file, rmdir, link, symlink, readlink, \
                              unlink, touch, chmod_file, chown, chgrp, clear, menu, open_file, close_file,\
                              pfd_file, lseek_file, quit, read_file, cat_file, write_file, cp_file, mv, quit, quit, mount_dev, umount_dev, sync, pm };

int main(int argc, char *argv[], char *env[]) {

    int i;
    i=sizeof(SUPER);
    sbuf=malloc(i+1);
    i=4096;
    gbuf=malloc(i+1);
    init();
    mount_root(argv[1]);
    while(true) {
        bzero(line, 1024);
        bzero(cname, 32);
        bzero(pathname, 1024);
        bzero(dname, 1024);
        printf("\n\nP%d running \n", running->pid);
        get_cwd(cwd);
        printf(GREEN"%s > "RESET, cwd);
        fgets(line, 1024, stdin);
        line[strlen(line)-1]=0;
        if(line[0]==0) continue;

        sscanf(line, "%s %s %s", cname, pathname, dname);
        if(strcmp(dname, "")!=0) {
            strcat(pathname, " ");
            strcat(pathname, dname);
        }
        for (i=0;i<32;i++) 
            if(strcmp(cname, command[i])==0) 
                    funcPtr[i](pathname);
    } 
    FREE_2(sbuf, gbuf);
}
