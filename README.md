# CS-149-Program-Project

1) put myfs.c in fuse/examples directory
2) added myfs to Makefile.am in fuse/examples directory, github has example Makefile.am
3) config and remake fuse examples
4) make directory in fuse/example to mount filesytem  (example: fuse/example/testdir)
5) run myfs (example:  ./myfs testdir)
   type mount command to see lt-myfs on fuse/example/testdir
6) change directory to fuse/example/testdir
7) create/cp/rm files to see myfs work
   you see 'Directory~', 'Inode~', and files in /tmp/myfs directory
