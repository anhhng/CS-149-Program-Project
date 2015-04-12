
#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

int myfs_unlink (const char *path);
int write_inode(const unsigned int in, struct stat st);
int read_inode(const unsigned int in, struct stat *stp);
int myfs_link (const char *path1, const char *path2);
int myfs_chmod (const char *path, mode_t mode);
int myfs_chown (const char *path, uid_t uid, gid_t gid);
int myfs_utimens (const char *path, const struct timespec tv[2]);
int myfs_utime (const char *path, struct utimbuf *tb);
int myfs_truncate (const char *path, off_t off);
int myfs_ftruncate (const char *path, off_t off, struct fuse_file_info *fi);
int myfs_setattr (const char * path, const char *name, const char *value, size_t size, int flag);
int myfs_mknod (const char *filename, int mode, int dev);
static int myfs_getattr(const char *path, struct stat *stbuf);
static int myfs_open(const char *path, struct fuse_file_info *fi);
static int myfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int myfs_write (const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
static int myfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);

char filePath[80] = {0};
int inodeCnt = 1;

struct myfs_dir_entry {
	char          name[32];
	unsigned int  inum;
};

// read inode stat from inode file
int write_inode(const unsigned int in, struct stat st) {
	int fd;
	int rtn;

	fd = open("/tmp/myfs/Inodes~", O_CREAT|O_RDWR, 0666);
	if (fd < 0)
		return fd;
	rtn = lseek(fd, in * sizeof(st), SEEK_SET);
	if (rtn < 0) {
		close(fd);
		return (rtn);
	}
	rtn = write(fd, &st, sizeof(st));
	close(fd);
	if (rtn != sizeof(st))
		return (rtn);
	return(0);
}

// write inode stat from inode file
int read_inode(const unsigned int in, struct stat *stp) {
	int fd;
	int rtn;

	fd = open("/tmp/myfs/Inodes~", O_CREAT|O_RDWR, 0666);
	if (fd < 0)
		return fd;
	rtn = lseek(fd, in * sizeof(struct stat), SEEK_SET);
	if (rtn < 0) {
		close(fd);
		return(rtn);
	}
	rtn = read(fd, stp, sizeof(struct stat));
	close(fd);
	if (rtn == sizeof(struct stat))
		return(0);
	else
		return(rtn);
}


int myfs_link (const char *path1, const char *path2){
	// TBD
	return 0;
}

int myfs_chmod (const char *path, mode_t mode){
	// TBD
	return 0;
}

int myfs_chown (const char *path, uid_t uid, gid_t gid){
	// TBD
	return 0;
}

int myfs_utimens (const char *path, const struct timespec tv[2]){
	// TBD
	return 0;
}

int myfs_utime (const char *path, struct utimbuf *tb){
	// TBD
	return 0;
}

int myfs_truncate (const char *path, off_t off){
	// TBD
    return 0;
}

int myfs_ftruncate (const char *path, off_t off, struct fuse_file_info *fi){
	// TBD
	return 0;
}

int myfs_setattr (const char * path, const char *name, const char *value, size_t size, int flag){
	// TBD
	return 0;
}

int myfs_unlink (const char *path){
	int fd;
	struct myfs_dir_entry dir;
	struct stat st;
	int index = 0;
	int rtn;

	fd = open("/tmp/myfs/Directory~", O_RDWR);

	if (fd < 0)
			return -EIO;

	while (read(fd, &dir, sizeof(dir)) == sizeof(dir)) {
		if (strcmp(path+1, dir.name) == 0 && (dir.inum != 0)) {

			read_inode(dir.inum, &st);
			st.st_nlink = st.st_nlink - 1;
			write_inode(dir.inum, st);

			if (st.st_nlink != 0)
				return 0; // reference count not zero

			// free directory enter
			dir.inum = 0;
			rtn = lseek(fd, index * sizeof(dir), SEEK_SET);
			if (rtn < 0) {
				close(fd);
				return -EIO;
			}
			rtn = write(fd, &dir, sizeof(dir));
			close(fd);

			// found, remove file storage
			strcpy(filePath,"/tmp/myfs/");
			strcat(filePath, dir.name);
			unlink(filePath);

			if (rtn != sizeof(dir))
				return -EIO;
			else
				return 0;
		}
		index = index + 1;
	}
	close(fd);

	return -ENOENT;
}

int myfs_mknod (const char *filename, int mode, int dev){
	(void) dev;
	struct myfs_dir_entry dir;
	struct timespec tm;
	int fd;
	int index;
	int rtn;
	struct stat st;

	if ((mode & S_IFMT) != S_IFREG)
		return -EPERM;

	fd = open("/tmp/myfs/Directory~", O_RDWR, 0666);
	if (fd < 0)
		return -EIO;

	// find not use directory entry
	index = 0;
	while (read(fd, &dir, sizeof(dir)) == sizeof(dir)) {
		if (strcmp(dir.name,".") == 0 || strcmp(dir.name,"..") == 0) {
			index++;
			continue;
		}
		if (dir.inum == 0)
			break;

		index++;
	}

	// create file to save contents
	strcpy(filePath,"/tmp/myfs");
	strcat(filePath, filename);
	rtn = open(filePath, O_CREAT|O_TRUNC|O_RDONLY, mode);
	if (rtn < 0) {
		close(fd);
		return -EIO;
	}
	close(rtn);

	// initialize stat
	st.st_mode = mode;
	st.st_nlink = 1;
	st.st_size = 0;
	st.st_uid = getuid();
	st.st_gid = getgid();
	clock_gettime(CLOCK_REALTIME, &tm);
	st.st_atim = tm;
	st.st_mtim = tm;
	st.st_ctim = tm;
	st.st_ino = inodeCnt++;

	// make new directory entry
	memset(&dir, 0, sizeof(dir));
	strcpy(dir.name, filename+1);
	dir.inum = st.st_ino;

	// write inode
	rtn = write_inode(st.st_ino, st);
	if (rtn != 0) {
		close(fd);
		return(rtn);
	}

	// create directory entry
	if (lseek(fd, index * sizeof(dir), SEEK_SET) < 0) {
		close(fd);
		return -EIO;
	}
	if (write(fd, &dir, sizeof(dir)) != sizeof(dir)) {
		close(fd);
		return -EIO;
	}

	close(fd);
	return 0;
}

int myfs_write (const char *path, const char *buf, size_t size, off_t offset,
												struct fuse_file_info *fi) {
	int fd;
	int cnt = 0;
	struct myfs_dir_entry dir;
	int found = 0;
	int rtn;
	struct stat st;

	fd = open("/tmp/myfs/Directory~", O_RDWR);

	if (fd < 0)
		return -EIO;

	// find directory entry
	while (read(fd, &dir, sizeof(dir)) == sizeof(dir)) {
		if (strcmp(path+1, dir.name) == 0 && (dir.inum != 0)) {
			found = 1;
			break;
		}
		cnt = cnt + 1;
	}

	if (found == 0) {
		close(fd);
		return -ENOENT;
	}
	close(fd);

	// update inode
	read_inode(dir.inum, &st);
	st.st_size = offset + size;
	clock_gettime(CLOCK_REALTIME, &st.st_atim);
	clock_gettime(CLOCK_REALTIME, &st.st_mtim);
	rtn = lseek(fd, cnt * sizeof(dir), SEEK_SET);
	rtn = write(fd, &dir, sizeof(dir));
	write_inode(dir.inum, st);


	// write file
	strcpy(filePath,"/tmp/myfs/");
	strcat(filePath, dir.name);
	fd = open(filePath, O_WRONLY);
	if (fd < 0)
		return -EIO;

	lseek(fd, offset, SEEK_SET);
	rtn = write(fd,buf, size);
	close(fd);
	return(rtn);
}

static int myfs_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;

	int fd;
	struct myfs_dir_entry dir;

	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else {
		fd = open("/tmp/myfs/Directory~", O_RDWR,0666);
		if (fd < 0)
			return -EIO;

		// find directory entry
		while (read(fd, &dir, sizeof(dir)) == sizeof(dir)) {
			if (strcmp(dir.name,path+1) == 0 && (dir.inum != 0)) {
				// read inode
				read_inode(dir.inum, stbuf);
				close(fd);
				return(0);
			}
		}
		res = -ENOENT;
	}

	close(fd);
	return res;
}

static int myfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	int fd_dir;
	struct myfs_dir_entry dir;
	struct stat st;

	if (strcmp(path, "/") != 0)
		return -ENOENT;

	fd_dir = open("/tmp/myfs/Directory~", O_RDWR, 0666);
	if (fd_dir < 0)
		return -EIO;

	while (read(fd_dir, &dir, sizeof(dir)) == sizeof(dir)) {
		if (dir.inum != 0) {
			// read stat from inode
			read_inode(dir.inum, &st);
			// give to fuse
			filler(buf, dir.name, &st, 0);
		}
	}

	if (fd_dir >= 0)
		close(fd_dir);

	return 0;
}

static int myfs_open(const char *path, struct fuse_file_info *fi)
{
	int fd;
	struct myfs_dir_entry dir;

	fd = open("/tmp/myfs/Directory~", O_RDWR);

	if (fd < 0)
			return -EIO;

	// read directory to find file
	while (read(fd, &dir, sizeof(dir)) == sizeof(dir)) {
		if (strcmp(path+1, dir.name) == 0 && (dir.inum != 0)) {
			close(fd);
			// found
			return 0;
		}
	}
	close(fd);

	return -ENOENT;
}

static int myfs_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	int fd;
	int rtn = 0;
	char myPath[80] = {0};
	static char iobuf[4096];

	strcat(myPath,"/tmp/myfs");
	strcat(myPath, path);
	fd = open(myPath, O_RDONLY);
	if (fd < 0)
		return -EIO;

	// seek to position
	if (lseek(fd, offset, SEEK_SET) == ENXIO) {
		close(fd);
		return 0;
	}

	// read content
	rtn = read(fd, &iobuf, size);
	if (rtn < 0) {
		close(fd);
		return(rtn);
	}

	// copy to user space
	memcpy(buf, iobuf, rtn);

	close(fd);

	return rtn;
}

static struct fuse_operations myfs_oper = {
	.getattr	= myfs_getattr,
	.readdir	= myfs_readdir,
	.open		= myfs_open,
	.read		= myfs_read,
	.write      = myfs_write,
	.mknod      = myfs_mknod,
	.setxattr	= myfs_setattr,
	.chmod      = myfs_chmod,
	.chown      = myfs_chown,
	.utimens    = myfs_utimens,
	.utime      = myfs_utime,
	.truncate	= myfs_truncate,
	.ftruncate  = myfs_ftruncate,
	.unlink		= myfs_unlink,
	.link		= myfs_link
};

int main(int argc, char *argv[])
{
	int fd_dir;
	struct myfs_dir_entry dir;
	int rtn;
	struct stat st;

	// create directory and inode files
	fd_dir = open("/tmp/myfs/Directory~", O_RDWR, 0666);
	if (fd_dir < 0) {
		rtn = mkdir("/tmp/myfs",0777);

		// truncate inodes file
		fd_dir = open("/tmp/myfs/Inodes~", O_CREAT|O_RDWR|O_TRUNC, 0666);
		if (fd_dir < 0)
			return -EIO;
		close(fd_dir);

		// create directory file
		fd_dir = open("/tmp/myfs/Directory~", O_CREAT|O_RDWR, 0666);
		if (fd_dir < 0)
			return -EIO;

		// make . directory entry
		memset(&dir, 0, sizeof(dir));
		strcpy(dir.name, ".");
		st.st_mode = S_IFDIR | 0755;
		st.st_nlink = 1;
		st.st_size = 0;
		st.st_ino = inodeCnt++;
		st.st_uid = getuid();
		st.st_gid = getgid();
		clock_gettime(CLOCK_REALTIME, &st.st_atim);
		clock_gettime(CLOCK_REALTIME, &st.st_mtim);
		clock_gettime(CLOCK_REALTIME, &st.st_ctim);
		dir.inum = st.st_ino;
		write(fd_dir, &dir, sizeof(dir));

		rtn = write_inode(st.st_ino, st);
		if (rtn != 0) {
			close(fd_dir);
			return(rtn);
		}

		// make .. directory entry
		strcpy(dir.name, "..");
		st.st_mode = S_IFDIR | 0755;
		st.st_nlink = 1;
		st.st_size = 0;
		st.st_ino = inodeCnt++;
		st.st_uid = getuid();
		st.st_gid = getgid();
		clock_gettime(CLOCK_REALTIME, &st.st_atim);
		clock_gettime(CLOCK_REALTIME, &st.st_mtim);
		clock_gettime(CLOCK_REALTIME, &st.st_ctim);
		dir.inum = st.st_ino;
		write(fd_dir, &dir, sizeof(dir));

		rtn = write_inode(st.st_ino, st);
		if (rtn != 0) {
			close(fd_dir);
			return(rtn);
		}
	}

	return fuse_main(argc, argv, &myfs_oper, NULL);
}
