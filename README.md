# EXT2/3 Filesystem Driver
- Works on 64-bit machine!
- Has support for direct, indirect, double-indirect blocks
- So works on largest possible HD on ext2/3
- Most of the code inspiration from Dr. K.C. Wang.


Included commands:

 cd       - change directory
 
 pwd      - print working dir
 
 link     - regular link
 
 unlink   - unlink
 
 symlink  - symbolic link
 
 ls       - list files
 
 mkdir    - make dir
 
 creat    - create file
 
 open     - open file descriptor
 
 close    - close file descriptor
 
 pipe     - create a pipe for read/write share.
 
 mount    - mount an ext2/3 device
 
 umount   - un-mount a device that is mounter already
 
 read     - read file
 
 write    - write to file
 
 rm       - remove file
 
 rmdir    - remove directory
