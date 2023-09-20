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
/* $Id: pf_io_fns.c,v 1.16 2001/11/25 01:43:57 pumatst Exp $ */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/unistd.h>

#include <errno.h>
#include "pf_io.h"

#ifdef linux
#define pf_syscall	syscall
/*
** syscall fails with _NR_mmap because it only works if
**   function has 5 or fewer arguments.  In case of mmap,
**   we'll invoke __mmap instead of using syscall.
*/
void *__mmap(void*, size_t, int, int, int, off_t);
#else
#define pf_syscall      ntv_sys
#endif

#if defined(__i386__)
/* see Fstat.i386.Note */
#include "alt-i386/types.h"
#include "alt-i386/stat.h"
#endif

#if defined(__osf__)
#include <sys/mount.h>
#include "alt-osf/linux-stat.h"
#include "alt-osf/linux-statfs.h"
#else
#include <sys/vfs.h>
#endif

#include "protocol_switch.h"

#define PF_ERRNO_NOFN ENOSYS

#ifdef __alpha__
typedef struct {
  unsigned int   st_dev;
  unsigned int   st_ino;
  unsigned int   st_mode;
  unsigned int   st_nlink;
  unsigned int   st_uid;
  unsigned int   st_gid;
  unsigned int   st_rdev;
  long           st_size;
  unsigned long  st_atime;
  unsigned long  st_mtime;
  unsigned long  st_ctime;
  unsigned int   st_blksize;
  int            st_blocks;
  unsigned int   st_flags;
  unsigned int   st_gen;
} kstat;
#endif

#ifdef __i386__
typedef struct {
  unsigned short st_dev;
  unsigned short __pad1;
  unsigned long st_ino;
  unsigned short st_mode;
  unsigned short st_nlink;
  unsigned short st_uid;
  unsigned short st_gid;
  unsigned short st_rdev;
  unsigned short __pad2;
  unsigned long  st_size;
  unsigned long  st_blksize;
  unsigned long  st_blocks;
  unsigned long  st_atime;
  unsigned long  __unused1;
  unsigned long  st_mtime;
  unsigned long  __unused2;
  unsigned long  st_ctime;
  unsigned long  __unused3;
  unsigned long  __unused4;
  unsigned long  __unused5;
} kstat;
#endif

#ifdef __ia64__
typedef struct {
  unsigned long	st_dev;
  unsigned long	st_ino;
  unsigned long	st_nlink;
  unsigned int	st_mode;
  unsigned int	st_uid;
  unsigned int	st_gid;
  unsigned int	__pad0;
  unsigned long	st_rdev;
  unsigned long	st_size;
  unsigned long	st_atime;
  unsigned long	__reserved0;	/* reserved for atime.nanoseconds */
  unsigned long	st_mtime;
  unsigned long	__reserved1;	/* reserved for mtime.nanoseconds */
  unsigned long	st_ctime;
  unsigned long	__reserved2;	/* reserved for ctime.nanoseconds */
  unsigned long	st_blksize;
  long		st_blocks;
  unsigned long	__unused[3];
} kstat;
#endif

io_ops_t io_ops_enfs =
{
  pf_open, pf_creat, pf_access, pf_chdir, pf_chmod, pf_chown, 
  pf_close, pf_dup, pf_dup2, pf_fcntl, pf_binval, pf_fstat, 
  pf_fstatfs, pf_fsync, pf_ftruncate, 
  pf_getdirentries, pf_ioctl, pf_link, pf_lseek, pf_lstat,
  pf_mkdir, pf_read, pf_mmap, pf_munmap, pf_rename, pf_rmdir, pf_stat, pf_statfs,
  pf_symlink, pf_truncate, pf_ttyname, pf_unlink, pf_write
};

int
pf_close(int fd)
{
int retVal = 0;

    if (!validFd(fd)){
	errno = EBADF;
	return -1;
    }
    /* if there is more than one  reference to this file
            don't close it only decrement the reference count */
    if ( FD_ENTRY_REFCOUNT( fd ) > 1 ) {
        FD_ENTRY_REFCOUNT_DEC( fd );
        retVal = 0;

    } else {

        retVal = pf_syscall(__NR_close, 
                          (unsigned int)FD_ENTRY_HOST_FILE_INDEX(fd));

        destroyFdTableEntry( FD_ENTRY( fd ) );
        FD_ENTRY( fd ) = NULL;
    }

    return retVal;
}

int
pf_binval(int fd, void *p)
{
    if (!validFd(fd)){
	errno = EBADF;
	return -1;
    }
    return pf_syscall(__NR_ioctl, 
                          (unsigned int)FD_ENTRY_HOST_FILE_INDEX(fd), 
                      /* BLKFLSBUF */ 536875617, 0);
}

off_t
pf_lseek(int fd, off_t off, int whence)
{
    if (!validFd(fd)){
        errno = EBADF;
        return -1;
    }

    return pf_syscall(__NR_lseek, 
                      (unsigned int)FD_ENTRY_HOST_FILE_INDEX(fd), 
                      off, (unsigned) whence);
}

int
pf_open(const char *path, int flags, mode_t mode)
{
int hostfd, fd;

    if ((fd = availFd()) < 0){
      errno = EMFILE;
      return -1;
    }
    mode &= (~(_my_umask & 0000777));

    hostfd = pf_syscall(__NR_open, path, (unsigned) flags, mode);

    if (hostfd > 0)
    {
        FD_ENTRY( fd ) = createFdTableEntry();
 
        FD_CLOSE_ON_EXEC_FLAG( fd ) = FD_CLOEXEC;
        FD_FILE_STATUS_FLAGS( fd ) = flags;
        FD_ENTRY_REFCOUNT( fd ) = 1;
 
        FD_ENTRY_HOST_FILE_INDEX( fd ) = hostfd;
 
        FD_ENTRY_PROTOCOL(fd) = ENFS_IO_PROTO;
    }
    else{
        fd = -1;
    }
 
    return fd;
}

#ifdef __osf__
int
#else
ssize_t
#endif
pf_read(int fd, void *buf,  size_t nbytes)
{
    if (!validFd(fd)){
        errno = EBADF;
        return -1;
    }
    return pf_syscall(__NR_read, 
                       (unsigned int)FD_ENTRY_HOST_FILE_INDEX( fd ), 
                       buf, nbytes);
}
void *
pf_mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset)
{
void *retBuf;

    if (!validFd(fd)){
        errno = EBADF;
        return NULL;
    }

#ifdef linux
    retBuf = __mmap(start, length, prot, flags, 
             (unsigned int)FD_ENTRY_HOST_FILE_INDEX( fd ), offset);
#else
    retBuf = (void *)pf_syscall(__NR_mmap, start, length, prot, flags,
             (unsigned int)FD_ENTRY_HOST_FILE_INDEX( fd ), offset);
#endif

    return retBuf;
}
int
pf_munmap(void *start, size_t length)
{
    return pf_syscall(__NR_munmap, start, length);
}

#ifdef __osf__
int
#else
ssize_t 
#endif
pf_write(int fd, const void *buf, size_t nbytes)
{
    if (!validFd(fd)){
        errno = EBADF;
        return -1;
    }

    return pf_syscall(__NR_write, 
                      (unsigned int)FD_ENTRY_HOST_FILE_INDEX( fd ), 
                      buf, nbytes);
}

int
pf_fstat(int fd, struct stat *buf)
{  
  int rc;
  kstat mstat;

  if (!validFd(fd)){
      errno = EBADF;
      return -1;
  }

  rc = pf_syscall(__NR_fstat, 
             (unsigned int)FD_ENTRY_HOST_FILE_INDEX( fd ) , &mstat);

  if (rc == 0) {
    buf->st_dev = mstat.st_dev;
    buf->st_ino = mstat.st_ino;
    buf->st_mode = mstat.st_mode;
    buf->st_nlink = mstat.st_nlink;
    buf->st_uid = mstat.st_uid;
    buf->st_gid = mstat.st_gid;
    buf->st_rdev = mstat.st_rdev;
    buf->st_size = mstat.st_size;
    buf->st_atime = mstat.st_atime;
    buf->st_mtime = mstat.st_mtime;
    buf->st_ctime = mstat.st_ctime;
    buf->st_blksize = mstat.st_blksize;
    buf->st_blocks = mstat.st_blocks;
#ifdef __alpha__
    buf->st_flags = mstat.st_flags;
    buf->st_gen = mstat.st_gen;
#endif
  }
  return rc;
}

int
pf_access(const char *path, int amode)
{
  return pf_syscall(__NR_access, path, amode);
}

int
pf_chdir( const char *path )
{
    return pf_syscall(__NR_chdir, path);
} 

int
pf_chmod(const char *path, mode_t mode )
{
    mode &= (~(_my_umask & 0000777));

    return pf_syscall(__NR_chmod, path, mode);
}

int 
pf_chown( const char *path, uid_t owner, gid_t group)
{
    return pf_syscall(__NR_chown, path, owner, group);
}

int
pf_creat(const char *fname, mode_t mode)
{
    return pf_syscall(__NR_open, fname, O_CREAT | O_WRONLY | O_TRUNC, 
                                                               (int) mode);
}

int
pf_dup( int fd )
{
int new_fd;

  if ((new_fd = availFd()) < 0){
      errno = EMFILE;
      return -1;
  }

  return fcntlDup(fd, new_fd);
}

int
pf_dup2( int old, int new )
{
  return fcntlDup(old, new);
}

int 
pf_fcntl( int fd, int request, int arg )
{
int rc;

    rc = 0;

    if (!validFd(fd)){
	errno = EBADF;
	return -1;
    }

    switch (request)
    {
	case F_DUPFD:
	    rc = fcntlDup(fd, arg);
	    break;

        case F_GETFD:
	    rc = FD_CLOSE_ON_EXEC_FLAG( fd );
            break;

        case F_SETFD:
            FD_CLOSE_ON_EXEC_FLAG( fd ) = arg;
            break;

        case F_SETFL:
            FD_FILE_STATUS_FLAGS( fd ) = (arg & ~O_ACCMODE);
            break;

        case F_GETFL:
            rc = FD_FILE_STATUS_FLAGS( fd );
            break;

        case F_GETOWN:
        case F_SETOWN:
            rc = pf_syscall(__NR_fcntl, 
                     (unsigned int) FD_ENTRY_HOST_FILE_INDEX(fd),
                     (unsigned int) request, (unsigned long) arg);
            break;

        case F_GETLK:
        case F_SETLK:
        case F_SETLKW:
            errno = EINVAL;   /* sorry we can't do this */
            rc = -1;
            break;
 
        default:
            errno = EINVAL;
            rc = -1;
    }
    return rc;
}

int
pf_fstatfs(int fd, struct statfs *buf )
{
    if (!validFd(fd)){
	errno = EBADF;
	return -1;
    }

    return pf_syscall(__NR_fstatfs, 
        (unsigned int) FD_ENTRY_HOST_FILE_INDEX(fd), buf); 
}  

int
pf_fsync( int fd )
{
    if (!validFd(fd)){
	errno = EBADF;
	return -1;
    }

    return pf_syscall(__NR_fsync, 
               (unsigned int) FD_ENTRY_HOST_FILE_INDEX(fd));
}

int
pf_ftruncate(int fd, long length)
{
  if (!validFd(fd)){
      errno = EBADF;
      return -1;
  }

  return pf_syscall(__NR_ftruncate, 
          (unsigned int) FD_ENTRY_HOST_FILE_INDEX(fd), 
          (unsigned long) length);
}

int
pf_getdirentries( int fd, char *buf, int nbytes, long *basep)
{
  errno = PF_ERRNO_NOFN;
  return -1;
}  

int pf_ioctl(int fd, unsigned long request, char* ptr)
{
  if (!validFd(fd)){
      errno = EBADF;
      return -1;
  }

  return pf_syscall(__NR_ioctl, 
             (unsigned int) FD_ENTRY_HOST_FILE_INDEX(fd), 
	     (unsigned int) request, (unsigned long) ptr);
}

int
pf_link(  const char *path1,  const char *path2 )
{
#if 1
  return pf_syscall(__NR_link, path1, path2);
#else
  errno = PF_ERRNO_NOFN;
  return -1;
#endif
}  

int pf_lstat(const char *path, struct stat *buf) 
{
  int rc;
  kstat mstat;

  rc =  pf_syscall(__NR_lstat, (char*) path, mstat);

  if (rc == 0) {
    buf->st_dev = mstat.st_dev;
    buf->st_ino = mstat.st_ino;
    buf->st_mode = mstat.st_mode;
    buf->st_nlink = mstat.st_nlink;
    buf->st_uid = mstat.st_uid;
    buf->st_gid = mstat.st_gid;
    buf->st_rdev = mstat.st_rdev;
    buf->st_size = mstat.st_size;
    buf->st_atime = mstat.st_atime;
    buf->st_mtime = mstat.st_mtime;
    buf->st_ctime = mstat.st_ctime;
    buf->st_blksize = mstat.st_blksize;
    buf->st_blocks = mstat.st_blocks;
#ifdef __alpha__
    buf->st_flags = mstat.st_flags;
    buf->st_gen = mstat.st_gen;
#endif
  }
  return rc;
}

int
pf_mkdir( const char *path, mode_t mode)
{
  mode &= (~(_my_umask & 0000777));

  return pf_syscall(__NR_mkdir, path, (int) mode);
}

int
pf_rename( const char *path1, const char *path2 )
{
#if 1
  return pf_syscall(__NR_rename, path1, path2);
#else
  errno = PF_ERRNO_NOFN;
  return -1;
#endif
}

int
pf_rmdir( const char *path)
{
  return pf_syscall(__NR_rmdir, path);
}

int
pf_stat( const char *path, struct stat *buf )
{
  int rc;
  kstat mstat;

  rc = pf_syscall(__NR_stat, (char*) path, &mstat);

  if (rc == 0) {
    buf->st_dev = mstat.st_dev;
    buf->st_ino = mstat.st_ino;
    buf->st_mode = mstat.st_mode;
    buf->st_nlink = mstat.st_nlink;
    buf->st_uid = mstat.st_uid;
    buf->st_gid = mstat.st_gid;
    buf->st_rdev = mstat.st_rdev;
    buf->st_size = mstat.st_size;
    buf->st_atime = mstat.st_atime;
    buf->st_mtime = mstat.st_mtime;
    buf->st_ctime = mstat.st_ctime;
    buf->st_blksize = mstat.st_blksize;
    buf->st_blocks = mstat.st_blocks;
#ifdef __alpha__
    buf->st_flags = mstat.st_flags;
    buf->st_gen = mstat.st_gen;
#endif
  }
  return rc;
}

int
pf_statfs( const char *path, struct statfs *sbuf )
{
  return pf_syscall(__NR_statfs, path, sbuf);
}  

int
pf_symlink(const  char *path1, const char *path2 )
{
#if 1
  return pf_syscall(__NR_symlink, path1, path2);
#else
  errno = PF_ERRNO_NOFN;
  return -1;
#endif
}  

int
pf_truncate(const char *path, off_t length)
{
  return pf_syscall(__NR_truncate, path, (unsigned long) length);
}

char *
pf_ttyname( int fd )
{
  errno = PF_ERRNO_NOFN;
  return NULL;
}  

int
pf_unlink( const char *path )
{
  return pf_syscall(__NR_unlink, path);
}  
