/*************************************************************************
Cplant Release Version 2.0.1.10
Release Date: Nov 5, 2002 
#############################################################################
#
#     This Cplant(TM) source code is the property of Sandia National
#     Laboratories.
#
#     This Cplant(TM) source code is copyrighted by Sandia National
#     Laboratories.
#
#     The redistribution of this Cplant(TM) source code is subject to the
#     terms of the GNU Lesser General Public License
#     (see cit/LGPL or http://www.gnu.org/licenses/lgpl.html)
#
#     Cplant(TM) Copyright 1998, 1999, 2000, 2001, 2002 Sandia Corporation. 
#     Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
#     license for use of this work by or on behalf of the US Government.
#     Export of this program may require a license from the United States
#     Government.
#
#############################################################################
**************************************************************************/
/* $Id: pf_io.h,v 1.4 2001/02/16 05:29:51 lafisk Exp $ */
#include <sys/stat.h>
#if defined(__linux__)
#include <sys/vfs.h>
#endif
#if defined(__osf__)
#include <sys/mount.h>
#endif

#define SYS_fbci	350

//extern off_t pf_lseek(int, off_t, unsigned);
int pf_binval(int fd, void *p);

int pf_fstat(int fd, struct stat *buf);
int pf_access(const char* path, int mode);
int pf_chdir(const char* path);
int pf_chmod(const char* path, mode_t);
int pf_chown(const char* path, uid_t, gid_t);
int pf_close(int);
int pf_creat(const char *fname, mode_t mode);
int pf_dup( int fd );
int pf_dup2( int old, int new );
int pf_fcntl( int fd, int request, int arg );
int pf_fstatfs(int fd, struct statfs *buf );
int pf_fsync( int fd );
int pf_ftruncate(int fd, long length);
int pf_getdirentries( int fd, char *buf, int nbytes, long *basep);
int pf_ioctl(int fd, unsigned long request, char* ptr);
int pf_link(  const char *path1,  const char *path2 );
off_t pf_lseek( int fd, off_t offset, int whence );
int pf_lstat(const char *path, struct stat *sbuf);
int pf_mkdir( const char *path, mode_t mode);
int pf_open(const char *fname, int flags, mode_t mode);
#ifdef __osf__
int pf_read(int fd, void *buff, size_t nbytes);
#else
ssize_t pf_read(int fd, void *buff, size_t nbytes);
#endif
void *pf_mmap(void *start, size_t length, int pr, int flags, int fd, off_t offset);
int pf_munmap(void *start, size_t length);
int pf_rename( const char *path1, const char *path2 );
int pf_rmdir( const char *path);
int pf_stat( const char *path, struct stat *sbuf );
int pf_statfs( const char *path, struct statfs *sbuf );
int pf_symlink(const  char *path1, const char *path2 );
#ifdef __linux__
int pf_truncate(const char *path, __off_t length);
#else
int pf_truncate(const char *path, off_t length);
#endif
char * pf_ttyname( int fd );
int pf_unlink( const char *path );
#ifdef __osf__
int pf_write( int fd, const void *buff, size_t nbytes);
#else
ssize_t pf_write( int fd, const void *buff, size_t nbytes);
#endif
