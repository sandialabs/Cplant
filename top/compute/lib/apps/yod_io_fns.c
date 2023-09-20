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
/* $Id: yod_io_fns.c,v 1.24 2002/02/12 18:51:14 pumatst Exp $ io_ops for both yod and fyod */

#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include "protocol_switch.h"
#include "puma_unistd.h"
#include "puma.h"
#include "errno.h"
#include "fdTable.h"
#include "shrink_path.h"

#include "portals/ppid.h"
#include "puma_io.h"

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

#include "fyod_map.h"

extern int _fyod_map[FYOD_MAP_SZ];

#ifdef __linux__
#define PRINTF(args...)
#endif
 
#define PATH_IS_FYOD( S )  strncmp( (S) , "/raid_", 6) == 0 && \
	isdigit( S[6] ) && isdigit( S[7] ) && isdigit( S[8] ) && \
	S[9] == '/'

#define ERRNO_SWITCH EFAULT


extern UINT16 _fyod_nid;
static int _fyod_dbg=0;
extern int Dbglevel;

int yod_switch( int cmdID, const char *full_path, hostCmd_t *cmd, UINT32 *rpc_nid, UINT32 *rpc_pid, UINT32 *rpc_portal  );

int fstat_yod(int fd, struct stat *buf);
int fstat_yod2(int linux_version, int fd, struct stat *buf);
int access_yod(const char* path, int mode);
int chdir_yod(const char* path);
int chmod_yod(const char* path, mode_t);
int chown_yod(const char* path, uid_t, gid_t);
int close_yod(int);
static int closeHost( int fd );
int creat_yod(const char *fname, mode_t mode);
int dup_yod( int fd );
int dup2_yod( int old, int new );
int fcntl_yod( int fd, int request, int arg );
static int fcntlHost( int fd, int request, int arg );
int binval_yod(int fd, void *p);
int fstatfs_yod(int fd, struct statfs *buf );
int fsync_yod( int fd );
int ftruncate_yod(int fd, long length);
int getdirentries_yod( int fd, char *buf, size_t nbytes, off_t *basep);
int ioctl_yod(int fd, unsigned long request, char* ptr);
int link_yod(  const char *path1,  const char *path2 );
off_t lseek_yod( int fd, off_t offset, int whence );
int lstat_yod(const char *path, struct stat *sbuf);
int lstat_yod2( int linux_version, const char *path, struct stat *sbuf );
int mkdir_yod( const char *path, mode_t mode);
int open_yod(const char *fname, int flags, mode_t mode);
#ifdef __osf__
int read_yod(int fd, void *buff, size_t nbytes);
#else
ssize_t read_yod(int fd, void *buff, size_t nbytes);
#endif
void *mmap_yod(void *start, size_t len, int pr, int flags, int fd, off_t offset);
int munmap_yod(void *st, size_t length);
int rename_yod( const char *path1, const char *path2 );
int rmdir_yod( const char *path);
int stat_yod( const char *path, struct stat *sbuf );
int stat_yod2( int linux_version, const char *path, struct stat *sbuf );
int statfs_yod( const char *path, struct statfs *sbuf );
int symlink_yod(const  char *path1, const char *path2 );
int truncate_yod(const char *path, off_t length);
char * ttyname_yod( int fd );
int unlink_yod( const char *path );
#ifdef __osf__
int write_yod( int fd, const void *buff, size_t nbytes);
#else
ssize_t write_yod( int fd, const void *buff, size_t nbytes);
#endif

io_ops_t io_ops_yod =
{
  open_yod, creat_yod, access_yod, chdir_yod, chmod_yod, chown_yod, 
  close_yod, dup_yod, dup2_yod, fcntl_yod, binval_yod, fstat_yod, 
  fstatfs_yod, fsync_yod, ftruncate_yod, 
  getdirentries_yod, ioctl_yod, link_yod, lseek_yod, lstat_yod,
  mkdir_yod, read_yod, mmap_yod, munmap_yod, rename_yod, rmdir_yod, stat_yod, statfs_yod,
  symlink_yod, truncate_yod, ttyname_yod, unlink_yod, write_yod
};

int
binval_yod(int fd, void*p) 
{
  errno = ENOSYS;
  return -1;
}

#if defined(__linux__)

int
fstat_yod(int fd, struct stat *buf)
{
  return fstat_yod2(0, fd, buf);
}

int
fstat_yod2(int linux_stat_version __attribute__ ((unused)), int fd,
       struct stat *buf )
{
#undef CMD
#define CMD     (cmd.info.fstatCmd)

  
hostCmd_t	cmd;
hostReply_t	ack;


    if ( ! validFd( fd ) ) {
	errno = EBADF;
	return(-1);
    }

    CMD.hostFileIndex= FD_ENTRY_HOST_FILE_INDEX( fd );

    cmd.uid = getuid();
    cmd.gid = getgid();
    cmd.euid = geteuid();
    cmd.egid = getegid();

    if ( _host_rpc(&cmd, &ack, 
	     CMD_FSTAT, FD_ENTRY_SRVR_NID( fd ),
	    FD_ENTRY_SRVR_PID( fd ), FD_ENTRY_SRVR_PTL( fd ),
	    NULL, 0,(char *) buf, sizeof(struct stat)) != 0) {

	/* could not send! Use errno set by _host_rpc() */
	return -1;
    }

    if ( ack.retVal == -1 ) {
	errno =  ack.hostErrno;
    }

    return( ack.retVal );

}  

#else

/*
** needed to fix incompatibility between Linux and OSF stat structures
*/

int
fstat_yod(int fd, struct stat *sbuf)
{
#undef CMD
#define CMD     (cmd.info.fstatCmd)

  
hostCmd_t	  cmd;
hostReply_t	  ack;
struct linux_stat linux_stat;


    if ( ! validFd( fd ) ) {
	errno = EBADF;
	return(-1);
    }

    CMD.hostFileIndex= FD_ENTRY_HOST_FILE_INDEX( fd );

    cmd.uid = getuid();
    cmd.gid = getgid();
    cmd.euid = geteuid();
    cmd.egid = getegid();

    if ( _host_rpc(&cmd, &ack, 
	    CMD_FSTAT, FD_ENTRY_SRVR_NID( fd ),
	    FD_ENTRY_SRVR_PID( fd ), FD_ENTRY_SRVR_PTL( fd ),
	    NULL, 0,(char *)&linux_stat, sizeof(struct linux_stat)) != 0) {

	/* could not send! Use errno set by _host_rpc() */
	return -1;
    }

    sbuf->st_dev     = linux_stat.st_dev; 
    sbuf->st_ino     = linux_stat.st_ino;
    sbuf->st_mode    = linux_stat.st_mode;
    sbuf->st_nlink   = linux_stat.st_nlink;
    sbuf->st_uid     = linux_stat.st_uid;
    sbuf->st_gid     = linux_stat.st_gid;
    sbuf->st_rdev    = linux_stat.st_rdev;
    sbuf->st_size    = linux_stat.st_size;
    sbuf->st_atime   = linux_stat.st_atime;
    sbuf->st_mtime   = linux_stat.st_mtime;
    sbuf->st_ctime   = linux_stat.st_ctime;
    sbuf->st_blksize = linux_stat.st_blksize;
    sbuf->st_blocks  = linux_stat.st_blocks;
    sbuf->st_flags   = linux_stat.st_flags;
    sbuf->st_gen     = linux_stat.st_gen;

    if ( ack.retVal == -1 ) {
	errno =  ack.hostErrno;
    }

    return( ack.retVal );

}  
#endif 


int
access_yod(const char *path, int amode)
{
#undef CMD
#define CMD     (cmd.info.accessCmd)

char 	full_path[MAXPATHLEN];
char 	*new_fname;

hostCmd_t	cmd;
hostReply_t 	ack;

int rpc_nid, rpc_pid, rpc_portal;

    MAKE_FULL_PATH(full_path, path, -1);

    new_fname = convert_pound(full_path, _my_rank);

    if ( new_fname == NULL )   {
	errno= ENAMETOOLONG;
	return(-1);
    }

    strcpy(full_path, new_fname);
    CMD.fnameLen = strlen(full_path) + 1;   /* + 1 for '\0' at eos */
    CMD.amode = amode;

    if ( yod_switch(CMD_ACCESS, full_path, &cmd, &rpc_nid, &rpc_pid, 
                                                           &rpc_portal) <0) {
      errno = ERRNO_SWITCH;
      return -1;
    }

    if ( _host_rpc(&cmd, &ack, 
	  CMD_ACCESS, rpc_nid, rpc_pid, rpc_portal,
	    full_path, CMD.fnameLen, NULL, 0) != 0 ) {

	/* could not send! Use errno set by _host_rpc() */
	return -1;
    }

    if ( ack.retVal == -1 ) {
	errno = ack.hostErrno;
    }

    return( ack.retVal );

}  /* end of access_yod() */


int
chdir_yod( const char *path )
{

#undef CMD
#define CMD     (cmd.info.chdirCmd)

hostCmd_t	cmd;
hostReply_t	ack;
char		full_path[MAXPATHLEN];
char		*test_path;

int rpc_nid, rpc_pid, rpc_portal;

    MAKE_FULL_PATH(full_path, path, -1);
    test_path = shrink_path(full_path);

    if (test_path == NULL)   {
	errno= ENAMETOOLONG;
	return(-1);
    }

    CMD.fnameLen = strlen(test_path) + 1;   /* + 1 for '\0' at eos */

    if (yod_switch( CMD_CHDIR, full_path, &cmd, &rpc_nid, &rpc_pid, 
                                                          &rpc_portal) <0 ) {
      errno = ERRNO_SWITCH;
      return -1;
    }

    if ( _host_rpc(&cmd, &ack, 
	  CMD_CHDIR, rpc_nid, rpc_pid, rpc_portal,
	    test_path, CMD.fnameLen, NULL, 0) != 0 ) {

	/* could not send! Use errno set by _host_rpc() */
	return -1;
    }

    if ( ack.retVal == -1 )   {
        errno = ack.hostErrno;
    } else {
        strcpy(_CLcwd, test_path);
    }

    return( ack.retVal );

}  /* end of chdir_yod() */

int
chmod_yod(const char *path, mode_t mode )
{

#undef CMD
#define CMD     (cmd.info.chmodCmd)

char 		full_path[MAXPATHLEN];
char 		*new_fname;

hostCmd_t	cmd;
hostReply_t 	ack;

int rpc_nid, rpc_pid, rpc_portal;


    MAKE_FULL_PATH(full_path, path, -1);

    new_fname = convert_pound(full_path, _my_rank );

    if ( new_fname == NULL )   {
	errno= ENAMETOOLONG;
	return(-1);
    }

    strcpy(full_path, new_fname);
    CMD.fnameLen = strlen(full_path) + 1;   /* + 1 for '\0' at eos */

    CMD.mode = mode;

    if (yod_switch(CMD_CHMOD, full_path, &cmd, &rpc_nid, &rpc_pid, 
                                                         &rpc_portal ) < 0) {
      errno = ERRNO_SWITCH;
      return -1;
    }

    if ( _host_rpc(&cmd, &ack, 
	 CMD_CHMOD, rpc_nid, rpc_pid, rpc_portal,
	    full_path, CMD.fnameLen, NULL, 0) != 0 ) {

	/* could not send! Use errno set by _host_rpc() */
	return -1;
    }

    if ( ack.retVal == -1 ) {
	errno = ack.hostErrno;
    }

    return( ack.retVal );

}  /* end of chmod_yod() */

int 
chown_yod( const char *path, uid_t owner, gid_t group)
{

#undef CMD
#define CMD     (cmd.info.chownCmd)

char 		full_path[MAXPATHLEN];
char 		*new_fname;

hostCmd_t	cmd;
hostReply_t 	ack;

int rpc_nid, rpc_pid, rpc_portal;

    MAKE_FULL_PATH(full_path, path, -1);

    new_fname = convert_pound(full_path, _my_rank );

    if ( new_fname == NULL )   {
	errno= ENAMETOOLONG;
	return(-1);
    }

    strcpy(full_path, new_fname);
    CMD.fnameLen = strlen(full_path) + 1;   /* + 1 for '\0' at eos */

    CMD.owner = owner;
    CMD.group = group;

    if (yod_switch( CMD_CHOWN, full_path, &cmd, &rpc_nid, &rpc_pid, 
                                                          &rpc_portal) <0) {
      errno = ERRNO_SWITCH;
      return -1;
    }

    if ( _host_rpc(&cmd, &ack, 
	 CMD_CHOWN, rpc_nid, rpc_pid, rpc_portal,
	    full_path, CMD.fnameLen, NULL, 0) != 0 ) {

	/* could not send! Use errno set by _host_rpc() */
	return -1;
    }

    if ( ack.retVal == -1) {
	errno = ack.hostErrno;
    }

    return( ack.retVal );

}  /* end of chown_yod() */


int close_yod( int fd )
{
int retVal = 0;

    if ( ! validFd( fd ) ) {
	errno= EBADF;
	return(-1);
    }

    /* if there is more than one  reference to this file
	    don't close it only decrement the reference count */ 
    if ( FD_ENTRY_REFCOUNT( fd ) > 1 ) {
	FD_ENTRY_REFCOUNT_DEC( fd ); 		
	retVal = 0;
    } else {
	/* 
	** only tell the host to close files
	** other than stdio, note that we will be able
	** to reuse this these fd numbers 
	*/
	if ( fd >= 3 ) {
	    retVal  = closeHost( fd );
	}
	destroyFdTableEntry( FD_ENTRY( fd ) );
	FD_ENTRY( fd ) = NULL;
    }
    return( retVal );
}

static int 
closeHost( int fd )
{
#undef CMD
#define CMD     (cmd.info.closeCmd)

    hostCmd_t 	cmd;
    hostReply_t	ack;

    CMD.hostFileIndex = FD_ENTRY_HOST_FILE_INDEX( fd );
    CMD.rank = _my_rank;

/* Now do the _real_ close */

    cmd.uid = getuid();
    cmd.gid = getgid();
    cmd.euid = geteuid();
    cmd.egid = getegid();

    if ( _host_rpc( &cmd, &ack, 
	 CMD_CLOSE, FD_ENTRY_SRVR_NID( fd ),
	    FD_ENTRY_SRVR_PID( fd ), FD_ENTRY_SRVR_PTL( fd ),
	    NULL, 0, NULL, 0) != 0 ) { 
	return ( -1 );
    }

    if ( ack.retVal == -1 ) {
	PCT_DUMP(9981, 0, 0, 0, 0, 0, 0);
	errno = ack.hostErrno;
    }


    return( ack.retVal );
}

int
creat_yod(const char *fname, mode_t mode)
{
  return(open(fname, O_WRONLY | O_CREAT | O_TRUNC, mode));
}

int
dup_yod( int fd )
{
  return( fcntl( fd, F_DUPFD, 0 ) ); 
}

int
dup2_yod( int old, int new )
{
    return( fcntl( old, F_DUPFD, new ) ); 
}

int 
fcntl_yod( int fd, int request, int arg )
{
int retVal = 0;

    switch( request )
    {
	case F_DUPFD:
	    retVal = fcntlDup( fd, arg );  
	    break;

	case F_GETFD:
	    retVal = FD_CLOSE_ON_EXEC_FLAG( fd );
	    break;

	case F_SETFD:
	    FD_CLOSE_ON_EXEC_FLAG( fd ) = arg;
	    break;

	case F_SETFL:
	    /* 
	    ** note that you cannot change the access mode 
	    ** of an open file 
	    */
	    FD_FILE_STATUS_FLAGS( fd ) = (arg & ~O_ACCMODE);
	    break;

	case F_GETFL:
	    retVal = FD_FILE_STATUS_FLAGS( fd );
	    break;

	case F_GETOWN:
	case F_SETOWN:
	    return( fcntlHost( fd, request, arg ) );
	    break;

	case F_GETLK: 
	case F_SETLK: 
	case F_SETLKW:
	    /* 
	    ** these are part of the OS for the paragon 
	    ** but are not implemented in puma 
	    */
	    errno = EINVAL; 
	    retVal = -1;
	    break;

	default:
	    errno = EINVAL; 
	    retVal = -1;
    }
    return( retVal ); 
}

static int 
fcntlHost( int fd, int request, int arg )
{

#undef CMD
#define CMD     (cmd.info.fcntlCmd)

hostCmd_t	cmd;
hostReply_t	ack;

CMD.hostFileIndex = FD_ENTRY_HOST_FILE_INDEX( fd );
CMD.request = request;
CMD.arg = arg;

    cmd.uid = getuid();
    cmd.gid = getgid();
    cmd.euid = geteuid();
    cmd.egid = getegid();

    if ( _host_rpc(&cmd, &ack, 
	 CMD_FCNTL,FD_ENTRY_SRVR_NID( fd ),
	    FD_ENTRY_SRVR_PID( fd ), FD_ENTRY_SRVR_PTL( fd ),
	    NULL, 0, NULL, 0) != 0 ) {

	return( -1 );
    }

    if ( ack.retVal == -1 ) {
	errno = ack.hostErrno;
    }

    return( ack.retVal );
}

#ifdef __linux__

int
fstatfs_yod(int fd, struct statfs *buf )
{
#undef CMD
#define CMD     (cmd.info.fstatfsCmd)

  
hostCmd_t	cmd;
hostReply_t	ack;
	
    if ( ! validFd( fd ) ) {
	errno = EBADF;
	return(-1);
    }
										   
    CMD.hostFileIndex= FD_ENTRY_HOST_FILE_INDEX( fd );

    cmd.uid = getuid();
    cmd.gid = getgid();
    cmd.euid = geteuid();
    cmd.egid = getegid();

    if ( _host_rpc(&cmd, &ack, 
	  CMD_FSTATFS, FD_ENTRY_SRVR_NID( fd ),
	    FD_ENTRY_SRVR_PID( fd ), FD_ENTRY_SRVR_PTL( fd ),
	    NULL, 0,(char *) buf, sizeof(struct statfs)) != 0) {

	/* could not send! Use errno set by _host_rpc() */
	return -1;
    }
    if ( ack.retVal == -1 ) {
	errno =  ack.hostErrno;
    }

    return( ack.retVal );

}  

#else

/*
** needed to fix incompatibility between Linux and OSF statfs structures
*/

int
fstatfs_yod(int fd, struct statfs *buf )
{
#undef CMD
#define CMD     (cmd.info.fstatfsCmd)

  
hostCmd_t	    cmd;
hostReply_t	    ack;
struct linux_statfs linux_statfs;
	
    if ( ! validFd( fd ) ) {
	errno = EBADF;
	return(-1);
    }
										   
    CMD.hostFileIndex= FD_ENTRY_HOST_FILE_INDEX( fd );

    cmd.uid = getuid();
    cmd.gid = getgid();
    cmd.euid = geteuid();
    cmd.egid = getegid();

    if ( _host_rpc(&cmd, &ack, 
	 CMD_FSTATFS, FD_ENTRY_SRVR_NID( fd ),
	    FD_ENTRY_SRVR_PID( fd ), FD_ENTRY_SRVR_PTL( fd ),
	    NULL, 0,(char *)&linux_statfs, sizeof(struct linux_statfs)) != 0) {

	/* could not send! Use errno set by _host_rpc() */
	return -1;
    }
    buf->f_type   = linux_statfs.f_type;
    buf->f_bsize  = linux_statfs.f_bsize;
    buf->f_blocks = linux_statfs.f_blocks;
    buf->f_bfree  = linux_statfs.f_bfree;
    buf->f_bavail = linux_statfs.f_bavail;
    buf->f_files  = linux_statfs.f_files;
    buf->f_ffree  = linux_statfs.f_ffree;
    memcpy( &buf->f_fsid, &linux_statfs.f_fsid, sizeof(buf->f_fsid) );

    if ( ack.retVal == -1 ) {
	errno =  ack.hostErrno;
    }

    return( ack.retVal );

}  
#endif

int
fsync_yod( int fd )
{
#undef CMD
#define CMD     (cmd.info.fsyncCmd)
 
hostCmd_t 	cmd;
hostReply_t	ack;


    if ( ! validFd( fd ) ) {
	errno = EBADF;
	return(-1);
    }

    CMD.hostFileIndex = FD_ENTRY_HOST_FILE_INDEX( fd );

    cmd.uid = getuid();
    cmd.gid = getgid();
    cmd.euid = geteuid();
    cmd.egid = getegid();

    if ( _host_rpc(&cmd, &ack, 
		CMD_FSYNC, FD_ENTRY_SRVR_NID( fd ),
		FD_ENTRY_SRVR_PID( fd ), FD_ENTRY_SRVR_PTL( fd ),
		NULL, 0, NULL, 0) != 0 ) {

	return( -1 );
    }
    if ( ack.retVal == -1 ) {
	errno = ack.hostErrno;
    }

    return( ack.retVal );
}

int
ftruncate_yod(int fd, long length)
{
#undef CMD
#define CMD     (cmd.info.ftruncateCmd)

hostCmd_t	cmd;
hostReply_t	ack;

    if ( ! validFd( fd ) ) {
        errno = EBADF;
        return(-1);
    }

    CMD.hostFileIndex = FD_ENTRY_HOST_FILE_INDEX(fd);
    CMD.length        = length;

    cmd.uid = getuid();
    cmd.gid = getgid();
    cmd.euid = geteuid();
    cmd.egid = getegid();

    if ( _host_rpc(&cmd, &ack, 
	 CMD_FTRUNCATE, FD_ENTRY_SRVR_NID( fd ),
	    FD_ENTRY_SRVR_PID( fd ), FD_ENTRY_SRVR_PTL( fd ),
	    NULL, 0, NULL, 0) != 0) {

	/* could not send! Use errno set by _host_rpc() */
	return -1;
    }
    if ( ack.retVal == -1 ) {
	errno =  ack.hostErrno;
    }

    return( ack.retVal );

}  /* end of truncate_yod() */

int
getdirentries_yod( int fd, char *buf, size_t nbytes, off_t *basep)
{
#undef CMD
#undef ACK
#define CMD     (cmd.info.getdirentriesCmd)
#define ACK     (ack.info.getdirentriesAck)

hostCmd_t	cmd;
hostReply_t	ack;

    CMD.hostFileIndex = FD_ENTRY_HOST_FILE_INDEX( fd ); 
    CMD.nbytes = nbytes;
    CMD.basep  = *basep;

    cmd.uid = getuid();
    cmd.gid = getgid();
    cmd.euid = geteuid();
    cmd.egid = getegid();

    if ( _host_rpc(&cmd, &ack,
	CMD_GETDIRENTRIES, FD_ENTRY_SRVR_NID( fd),
	FD_ENTRY_SRVR_PID( fd ), FD_ENTRY_SRVR_PTL( fd ),
	NULL, 0 , buf, nbytes ) != 0) {

	/* could not send! Use errno set by _host_rpc() */
	return -1;
    }
    if ( ack.retVal == -1 ) {
	errno = ack.hostErrno;
    }
    else{
        *basep = ACK.basep;
    }


    return( ack.retVal );
				   
}  

int ioctl_yod(int fd, unsigned long request, char* ptr)
{
  errno = ENOSYS;
  return -1;
}

int
link_yod(  const char *path1,  const char *path2 )
{
#undef CMD
#define CMD     (cmd.info.linkCmd)

char 	full_path1[MAXPATHLEN];
char 	full_path2[MAXPATHLEN];
char 	buff[MAXPATHLEN*2+2];
char 	*new_fname1;
char 	*new_fname2;

hostCmd_t	cmd;
hostReply_t ack;

int rpc_nid, rpc_pid, rpc_portal;

    MAKE_FULL_PATH(full_path1, path1, -1);
    new_fname1 = convert_pound(full_path1, _my_rank );

    if ( new_fname1 == NULL )   {
	errno= ENAMETOOLONG;
	return(-1);
    }

    strcpy( buff, new_fname1 );

    /* +1 for '\0' at eos */
    CMD.fnameLen1 = strlen(new_fname1) + 1;


    MAKE_FULL_PATH(full_path2, path2, -1);
    new_fname2 = convert_pound(full_path2, _my_rank );


    if ( new_fname2 == NULL )   {
	errno= ENAMETOOLONG;
	return(-1);
    }

    strcpy( buff + strlen(buff) + 1 , new_fname2);

    /* +1 for '\0' at eos */
    CMD.fnameLen2 = strlen(new_fname2) + 1;

    if (yod_switch(CMD_LINK, full_path1, &cmd, &rpc_nid, &rpc_pid, 
                                                         &rpc_portal) <0) {
      errno = ERRNO_SWITCH;
      return -1;
    }

    if ( _host_rpc(&cmd, &ack, 
	 CMD_LINK, rpc_nid, rpc_pid, rpc_portal,
	    buff, CMD.fnameLen1 + CMD.fnameLen2, NULL, 0) != 0 ) {

	/* could not send! Use errno set by _host_rpc() */
	return -1;
    }

    if ( ack.retVal == -1 ) {	
	errno = ack.hostErrno;
    }
    return( ack.retVal );

}  

off_t
lseek_yod( int fd, off_t offset, int whence )
{
#undef CMD
#define CMD     (cmd.info.lseekCmd)
 
hostCmd_t 	cmd;
hostReply_t	ack;

    if ( ! validFd( fd ) ) {
	errno = EBADF;
	return(-1);
    }

    CMD.hostFileIndex = FD_ENTRY_HOST_FILE_INDEX( fd );
    CMD.offset = offset;
    CMD.whence = whence;
    CMD.curPos = FD_ENTRY_CURPOS( fd );

    cmd.uid = getuid();
    cmd.gid = getgid();
    cmd.euid = geteuid();
    cmd.egid = getegid();

    if ( _host_rpc(&cmd, &ack, 
	    CMD_LSEEK, FD_ENTRY_SRVR_NID( fd ),
	    FD_ENTRY_SRVR_PID( fd ), FD_ENTRY_SRVR_PTL( fd ),
	    NULL, 0, NULL, 0) != 0 ) {
	return( -1 );
    }

    if ( ack.retVal == -1 ) {
	errno = ack.hostErrno;
    } else {
    	FD_ENTRY_CURPOS( fd ) = ack.retVal;
    }	

    return( ack.retVal );
}

#if defined(__linux__)
/* kind of nasty: 1) library calls lxstat
                  2) which calls this w/ the normal lstat args
                  3) which calls lstat_yod2 w/ linux lstat/lxstat args...
*/
int lstat_yod(const char *path, struct stat *sbuf) 
{
  return lstat_yod2( 0, path, sbuf);
}

int
lstat_yod2( int linux_version __attribute__ ((unused)), const char *path,
           struct stat *sbuf )
{
#undef CMD
#define CMD     (cmd.info.lstatCmd)

  
char    	full_path[MAXPATHLEN];
char    	*new_fname;

hostCmd_t	cmd;
hostReply_t	ack;

    MAKE_FULL_PATH(full_path, path, -1);

    new_fname = convert_pound(full_path, _my_rank );
	
    if ( new_fname == NULL ) {
	errno= ENAMETOOLONG;
	return(-1);
    }

    strcpy(full_path, new_fname);
    CMD.fnameLen = strlen(full_path) + 1;   /* + 1 for '\0' at eos */

    cmd.uid = getuid();
    cmd.gid = getgid();
    cmd.euid = geteuid();
    cmd.egid = getegid();

    if ( _host_rpc(&cmd, &ack, 
	   CMD_LSTAT, YOD_NID, YOD_PID, YOD_PTL, 
	full_path, CMD.fnameLen, (char *) sbuf, sizeof(struct stat)) != 0) {

	/* could not send! Use errno set by _host_rpc() */
	return -1;
    }
    if ( ack.retVal == -1 ) {
	errno = ack.hostErrno;
    }


    return( ack.retVal );

}  

#else

int lstat_yod(const char *path, struct stat *sbuf) 
{
  return lstat_yod2( 0, path, sbuf);
}

int
lstat_yod2( int linux_version, const char *path, struct stat *sbuf )
{
#undef CMD
#define CMD     (cmd.info.lstatCmd)

  
char    	full_path[MAXPATHLEN];
char    	*new_fname;

hostCmd_t	  cmd;
hostReply_t	  ack;
struct linux_stat linux_stat;

    MAKE_FULL_PATH(full_path, path, -1);

    new_fname = convert_pound(full_path, _my_rank );
	
    if ( new_fname == NULL ) {
	errno= ENAMETOOLONG;
	return(-1);
    }

    strcpy(full_path, new_fname);
    CMD.fnameLen = strlen(full_path) + 1;   /* + 1 for '\0' at eos */

    cmd.uid = getuid();
    cmd.gid = getgid();
    cmd.euid = geteuid();
    cmd.egid = getegid();

    if ( _host_rpc(&cmd, &ack, 
	CMD_LSTAT, YOD_NID, YOD_PID, YOD_PTL, 
	full_path, CMD.fnameLen, (char *)&linux_stat, sizeof(struct linux_stat)) != 0) {

	/* could not send! Use errno set by _host_rpc() */
	return -1;
    }
    sbuf->st_dev     = linux_stat.st_dev; 
    sbuf->st_ino     = linux_stat.st_ino;
    sbuf->st_mode    = linux_stat.st_mode;
    sbuf->st_nlink   = linux_stat.st_nlink;
    sbuf->st_uid     = linux_stat.st_uid;
    sbuf->st_gid     = linux_stat.st_gid;
    sbuf->st_rdev    = linux_stat.st_rdev;
    sbuf->st_size    = linux_stat.st_size;
    sbuf->st_atime   = linux_stat.st_atime;
    sbuf->st_mtime   = linux_stat.st_mtime;
    sbuf->st_ctime   = linux_stat.st_ctime;
    sbuf->st_blksize = linux_stat.st_blksize;
    sbuf->st_blocks  = linux_stat.st_blocks;
    sbuf->st_flags   = linux_stat.st_flags;
    sbuf->st_gen     = linux_stat.st_gen;

    if ( ack.retVal == -1 ) {
	errno = ack.hostErrno;
    }


    return( ack.retVal );

}  

#endif

int
mkdir_yod( const char *path, mode_t mode)
{
#undef CMD
#define CMD     (cmd.info.mkdirCmd)

char 	full_path[MAXPATHLEN];
char 	*new_fname;

hostCmd_t	cmd;
hostReply_t 	ack;

int rpc_nid, rpc_pid, rpc_portal;

    MAKE_FULL_PATH(full_path, path, -1);

    new_fname = convert_pound(full_path, _my_rank );

    if ( new_fname == NULL )   {
	errno= ENAMETOOLONG;
	return(-1);
    }

    strcpy(full_path, new_fname);
    CMD.fnameLen = strlen(full_path) + 1;   /* + 1 for '\0' at eos */

    CMD.mode = mode & (~(_my_umask & 0000777));

    if (yod_switch( CMD_MKDIR, full_path, &cmd, &rpc_nid, &rpc_pid, 
                                                          &rpc_portal ) <0) {
      errno = ERRNO_SWITCH;
      return -1;
    }

    if ( _host_rpc(&cmd, &ack, 
	 CMD_MKDIR, rpc_nid, rpc_pid, rpc_portal,
	    full_path, CMD.fnameLen, NULL, 0) != 0 ) {

	/* could not send! Use errno set by _host_rpc() */
	return -1;
    }

    if ( ack.retVal == -1 ) {
	    errno = ack.hostErrno;
    }

    return( ack.retVal );

}  /* end of mkdir_yod() */


int
open_yod(const char *fname, int flags, mode_t mode)
{
#undef CMD
#undef ACK
#define CMD     (cmd.info.openCmd)
#define ACK     (ack.info.openAck)
#define FYODCMD     (cmd.info.openCmd.filetype.fyod)
#define INVALID 0xffff
#define vdebug(type, var) fprintf(stderr, #var"=%"#type"\n", var)


int 			fd, path_len, retry=0;
char 			full_path[MAXPATHLEN];
char 			*new_fname;
hostCmd_t		cmd;
hostReply_t 		ack;
int rpc_nid;
int rpc_pid;
int rpc_portal;

    if ( _fyod_dbg != 0 ) {
	printf ("open: Open entered Version July  99 \n");
	printf ("open: Open file limit = %d\n", _NFILE);
    }

    if ((fd = availFd()) < 0){
      errno = EMFILE;
      return -1;
    }

    MAKE_FULL_PATH(full_path, fname, -1);
    new_fname = convert_pound(full_path, _my_rank );

    if ( new_fname == NULL ) {	
      errno= ENAMETOOLONG;
      return -1;
    } 

    strcpy(full_path, new_fname);
    path_len = strlen(full_path) + 1;	/* + 1 for '\0' at eos */

    /*
     * Setup the command for some type of yod
     */
again:
    CMD.fnameLen = path_len;
    CMD.mode = mode & (~(_my_umask & 0000777));
    CMD.flags = flags;

    if (yod_switch( CMD_OPEN, full_path, &cmd, &rpc_nid, &rpc_pid, 
                                                         &rpc_portal ) < 0) {
      errno = ERRNO_SWITCH;
      return -1;
    }
    if (  _host_rpc(&cmd, &ack, 
	CMD_OPEN, rpc_nid, rpc_pid, rpc_portal,
	    full_path, CMD.fnameLen, NULL, 0) != 0 ) {
	/* could not send! Use errno set by _host_rpc() */
	return( -1);
    }

    if (ack.retVal > 0) 
	{
	FD_ENTRY( fd ) = createFdTableEntry();

	FD_CLOSE_ON_EXEC_FLAG( fd ) = FD_CLOEXEC; 
	FD_FILE_STATUS_FLAGS( fd ) = flags; 
	FD_ENTRY_REFCOUNT( fd ) = 1;

	/* Fill in the server that will handle requests for this file */
	FD_ENTRY_SRVR_NID( fd ) = ACK.srvrNid;
	FD_ENTRY_SRVR_PID( fd ) = ACK.srvrPid;
	FD_ENTRY_SRVR_PTL( fd ) = ACK.srvrPtl;

	FD_ENTRY_HOST_FILE_INDEX( fd ) = ack.retVal;
	FD_ENTRY_CURPOS( fd ) = ACK.curPos;
	FD_ENTRY_IS_TTY( fd ) = ACK.isattyFlag;

	FD_ENTRY_PROTOCOL(fd) = YOD_IO_PROTO;
    } 
    else {

      /*  Did fyod suggest "try this again"?  */

      if ( (retry <= 5) && (ack.hostErrno == EAGAIN) ) {
	retry++;
        goto again;
      }

      fd = -1;	
      errno = ack.hostErrno;
    }

    return( fd );
}  /* end of open_yod() */

#ifdef __osf__
int
#else
ssize_t
#endif
read_yod(int fd, void *buff, size_t nbytes)
{
#undef CMD
#undef ACK
#define CMD     (cmd.info.readCmd)
#define ACK     (ack.info.readAck)

hostCmd_t	cmd;
hostReply_t	ack;
		 
    if ( ! validFd( fd ) ) {
	errno= EBADF;
	return(-1);
    }

    /* add 1 to the flags so we can use FREAD to extract value */ 
    if ( ! ( ( FD_FILE_STATUS_FLAGS(fd) + 1 ) & FREAD ) ) {
	errno= EBADF;
	return(-1);
    }

    /* Setup the command for yod */
    CMD.nbytes = nbytes;
    CMD.curPos = FD_ENTRY_CURPOS( fd ); 
    CMD.hostFileIndex =  FD_ENTRY_HOST_FILE_INDEX( fd );
	CMD.rank = _my_rank;
	CMD.nnodes = _my_nnodes;

    /* at this point the host does not care about access mode, so mask it */  
    CMD.fileStatusFlags =  FD_FILE_STATUS_FLAGS( fd ) & ~O_ACCMODE;

    cmd.uid = getuid();
    cmd.gid = getgid();
    cmd.euid = geteuid();
    cmd.egid = getegid();

    if ( _host_rpc(&cmd, &ack, 
	CMD_READ, FD_ENTRY_SRVR_NID( fd ),
	FD_ENTRY_SRVR_PID( fd ), FD_ENTRY_SRVR_PTL( fd ),
	NULL, 0, buff, nbytes) != 0 ) {

	/* could not send! Use errno set by _host_rpc() */
	return -1;
    }

    FD_ENTRY_CURPOS( fd ) = ACK.curPos;

    if ( ack.retVal == -1 ) {
	errno = ack.hostErrno;
    }

    return( ack.retVal );
}

/*
** Limited mmap functionality.  If start address is not specified, and
** length is reasonable, and memory is read-only, we'll malloc the memory
** and read in the file for you.
** 
*/
#define  MMAP_ERR(addr, err) \
    if (addr) free(addr);    \
    errno = err;             \
    return (void *)MAP_FAILED;

void *
mmap_yod(void *start, size_t length, int prot, int flags, int fd, off_t offset)
{
int rc, curpos;
void *st;

    st = NULL;

    if (start){
	MMAP_ERR(st, EINVAL);
    }
    if (prot != PROT_READ){
	MMAP_ERR(st, EINVAL);
    }

    st = (void *)calloc(length, 1);

    if (!st){
	MMAP_ERR(st, ENOMEM);
    }    

    curpos = lseek(fd, 0, SEEK_CUR);

    if (curpos < 0){
	MMAP_ERR(st, errno);
    }

    rc = lseek(fd, offset, SEEK_SET);

    if (rc < 0){
	MMAP_ERR(st, errno);
    }
    rc = read(fd, st, length);

    if (rc < 0){
	MMAP_ERR(st, errno);
    }

    rc = lseek(fd, curpos, SEEK_SET);

    if (rc < 0){
	MMAP_ERR(st, errno);
    }

    return st;
}
int munmap_yod(void *st, size_t length)
{
    free(st);

    return 0;
}

int
rename_yod( const char *path1, const char *path2 )
{
#undef CMD
#define CMD     (cmd.info.renameCmd)

char 	full_path1[MAXPATHLEN];
char 	full_path2[MAXPATHLEN];
char 	buff[MAXPATHLEN*2+2];
char 	*new_fname1;
char 	*new_fname2;

hostCmd_t	cmd;
hostReply_t 	ack;

int rpc_nid, rpc_pid, rpc_portal;

    MAKE_FULL_PATH(full_path1, path1, -1);
    new_fname1 = convert_pound(full_path1, _my_rank );

    if ( new_fname1 == NULL )   {
	errno= ENAMETOOLONG;
	return(-1);
    }

    strcpy( buff, new_fname1 );
    /* +1 for '\0' at eos */
    CMD.fnameLen1 = strlen(new_fname1) + 1;


    MAKE_FULL_PATH(full_path2, path2, -1);
    new_fname2 = convert_pound(full_path2, _my_rank );


    if ( new_fname2 == NULL ) {
	errno= ENAMETOOLONG;
	return(-1);
    }


    strcpy( buff + strlen(buff) + 1 , new_fname2);

    /* +1 for '\0' at eos */
    CMD.fnameLen2 = strlen(new_fname2) + 1;

    if (yod_switch(CMD_RENAME, full_path1, &cmd, &rpc_nid, &rpc_pid, 
                                                           &rpc_portal ) <0) {
      errno = ERRNO_SWITCH;
      return -1;
    }

    if ( _host_rpc(&cmd, &ack, 
	CMD_RENAME, rpc_nid, rpc_pid, rpc_portal,
	buff, CMD.fnameLen1 + CMD.fnameLen2, NULL, 0) != 0 ) {

	/* could not send! Use errno set by _host_rpc() */
	return -1;
    }

    if ( ack.retVal == -1 ) {	
	errno = ack.hostErrno;
    }
    return( ack.retVal );

}

int
rmdir_yod( const char *path)
{
#undef CMD
#define CMD     (cmd.info.rmdirCmd)

CHAR 	full_path[MAXPATHLEN];
CHAR 	*new_fname;

hostCmd_t	cmd;
hostReply_t 	ack;

int rpc_nid, rpc_pid, rpc_portal;

    MAKE_FULL_PATH(full_path, path, -1);

    new_fname = convert_pound(full_path, _my_rank );

    if ( new_fname == NULL )   {
	errno= ENAMETOOLONG;
	return(-1);
    }

    strcpy(full_path, new_fname);
    CMD.fnameLen = strlen(full_path) + 1;   /* + 1 for '\0' at eos */

    if (yod_switch(CMD_RMDIR, full_path, &cmd, &rpc_nid, &rpc_pid, 
                                                         &rpc_portal ) < 0) {
      errno = ERRNO_SWITCH;
      return -1;
    } 

    if ( _host_rpc(&cmd, &ack, 
	 CMD_RMDIR, rpc_nid, rpc_pid, rpc_portal,
	full_path, CMD.fnameLen, NULL, 0) != 0 ) {
	/* could not send! Use errno set by _host_rpc() */
	return -1;
    }

    if ( ack.retVal == -1 ) {
	errno = ack.hostErrno;
    }

    return( ack.retVal );

}  /* end of rmdir_yod() */

#if defined(__linux__)
int
stat_yod( const char *path, struct stat *sbuf )
{
  return stat_yod2(0, path, sbuf);
}

int
stat_yod2( int linux_version __attribute__ ((unused)), const char *path,
         struct stat *sbuf )
{
#undef CMD
#define CMD     (cmd.info.statCmd)

char    	full_path[MAXPATHLEN];
char    	*new_fname;

hostCmd_t	cmd;
hostReply_t	ack;

int rpc_nid, rpc_pid, rpc_portal;

    MAKE_FULL_PATH(full_path, path, -1);

    new_fname = convert_pound(full_path, _my_rank );
	
    if ( new_fname == NULL ) {
	errno= ENAMETOOLONG;
	return(-1);
    }

    strcpy(full_path, new_fname);
    CMD.fnameLen = strlen(full_path) + 1;   /* + 1 for '\0' at eos */

    if (yod_switch(CMD_STAT, full_path, &cmd, &rpc_nid, &rpc_pid, 
                                                        &rpc_portal) < 0) {
      errno = ERRNO_SWITCH;
      return -1;
    }

    if ( _host_rpc(&cmd, &ack, 
	 CMD_STAT, rpc_nid, rpc_pid, rpc_portal, 
	full_path, CMD.fnameLen, (char *) sbuf, sizeof(struct stat)) != 0) {

	/* could not send! Use errno set by _host_rpc() */
	return -1;
    }

    if ( ack.retVal == -1 ) {
	errno = ack.hostErrno;
    }

    return( ack.retVal );
				   
}  

#else

int
stat_yod( const char *path, struct stat *sbuf )
{
#undef CMD
#define CMD     (cmd.info.statCmd)

char    	full_path[MAXPATHLEN];
char    	*new_fname;

hostCmd_t	  cmd;
hostReply_t	  ack;
struct linux_stat linux_stat;

int rpc_nid;
int rpc_pid;
int rpc_portal;

    MAKE_FULL_PATH(full_path, path, -1);

    new_fname = convert_pound(full_path, _my_rank );
	
    if ( new_fname == NULL ) {
	errno= ENAMETOOLONG;
	return(-1);
    }

    strcpy(full_path, new_fname);
    CMD.fnameLen = strlen(full_path) + 1;   /* + 1 for '\0' at eos */

    if (yod_switch(CMD_STAT, full_path, &cmd, &rpc_nid, &rpc_pid, 
                                                        &rpc_portal) < 0) {
      errno = ERRNO_SWITCH;
      return -1;
    }

    if ( _host_rpc(&cmd, &ack, 
	 CMD_STAT, rpc_nid, rpc_pid, rpc_portal, 
	full_path, CMD.fnameLen, (char *)&linux_stat, sizeof(struct linux_stat)) != 0) {

	/* could not send! Use errno set by _host_rpc() */
	return -1;
    }

    sbuf->st_dev     = linux_stat.st_dev; 
    sbuf->st_ino     = linux_stat.st_ino;
    sbuf->st_mode    = linux_stat.st_mode;
    sbuf->st_nlink   = linux_stat.st_nlink;
    sbuf->st_uid     = linux_stat.st_uid;
    sbuf->st_gid     = linux_stat.st_gid;
    sbuf->st_rdev    = linux_stat.st_rdev;
    sbuf->st_size    = linux_stat.st_size;
    sbuf->st_atime   = linux_stat.st_atime;
    sbuf->st_mtime   = linux_stat.st_mtime;
    sbuf->st_ctime   = linux_stat.st_ctime;
    sbuf->st_blksize = linux_stat.st_blksize;
    sbuf->st_blocks  = linux_stat.st_blocks;
    sbuf->st_flags   = linux_stat.st_flags;
    sbuf->st_gen     = linux_stat.st_gen;

    if ( ack.retVal == -1 ) {
	errno = ack.hostErrno;
    }

    return( ack.retVal );
}  

#endif

#if defined(__linux__)

int
statfs_yod( const char *path, struct statfs *sbuf )
{
#undef CMD
#define CMD     (cmd.info.statfsCmd)

  
char    	full_path[MAXPATHLEN];
char    	*new_fname;

hostCmd_t	cmd;
hostReply_t	ack;

    MAKE_FULL_PATH(full_path, path, -1);

    new_fname = convert_pound(full_path, _my_rank );
	
    if ( new_fname == NULL ) {
	errno= ENAMETOOLONG;
	return(-1);
    }

    strcpy(full_path, new_fname);
    CMD.fnameLen = strlen(full_path) + 1;   /* + 1 for '\0' at eos */

    cmd.uid = getuid();
    cmd.gid = getgid();
    cmd.euid = geteuid();
    cmd.egid = getegid();

    if ( _host_rpc(&cmd, &ack, 
	CMD_STATFS, YOD_NID, YOD_PID, YOD_PTL, 
	full_path, CMD.fnameLen, (char *) sbuf, sizeof(struct statfs)) != 0) {

	/* could not send! Use errno set by _host_rpc() */
	return -1;
    }
    if ( ack.retVal == -1 ) {
	errno = ack.hostErrno;
    }


    return( ack.retVal );
				   
}  


#else

int
statfs_yod( const char *path, struct statfs *sbuf )
{
#undef CMD
#define CMD     (cmd.info.statfsCmd)

  
char    	     full_path[MAXPATHLEN];
char    	    *new_fname;

hostCmd_t	     cmd;
hostReply_t	     ack;

struct linux_statfs  linux_statfs;


    MAKE_FULL_PATH(full_path, path, -1);

    new_fname = convert_pound(full_path, _my_rank );
	
    if ( new_fname == NULL ) {
	errno= ENAMETOOLONG;
	return(-1);
    }

    strcpy(full_path, new_fname);
    CMD.fnameLen = strlen(full_path) + 1;   /* + 1 for '\0' at eos */

    cmd.uid = getuid();
    cmd.gid = getgid();
    cmd.euid = geteuid();
    cmd.egid = getegid();

    if ( _host_rpc(&cmd, &ack, 
	CMD_STATFS, YOD_NID, YOD_PID, YOD_PTL, 
	full_path, CMD.fnameLen, (char *)&linux_statfs, sizeof(struct linux_statfs)) != 0) {

	/* could not send! Use errno set by _host_rpc() */
	return -1;
    }
    sbuf->f_type   = linux_statfs.f_type;
    sbuf->f_bsize  = linux_statfs.f_bsize;
    sbuf->f_blocks = linux_statfs.f_blocks;
    sbuf->f_bfree  = linux_statfs.f_bfree;
    sbuf->f_bavail = linux_statfs.f_bavail;
    sbuf->f_files  = linux_statfs.f_files;
    sbuf->f_ffree  = linux_statfs.f_ffree;
    memcpy( &sbuf->f_fsid, &linux_statfs.f_fsid, sizeof(sbuf->f_fsid) );

    if ( ack.retVal == -1 ) {
	errno = ack.hostErrno;
    }


    return( ack.retVal );
				   
}  

#endif

int
symlink_yod(const  char *path1, const char *path2 )
{
#undef CMD
#define CMD     (cmd.info.linkCmd)

char 	full_path2[MAXPATHLEN];
char 	buff[MAXPATHLEN*2+2];
char 	*new_fname1;
char 	*new_fname2;

hostCmd_t	cmd;
hostReply_t ack;

int rpc_nid, rpc_pid, rpc_portal;

    /* Don't call MAKE_FULL_PATH for path1, it should be whatever string
     * the user specifies */
    new_fname1 = convert_pound(path1, _my_rank );

    if ( new_fname1 == NULL )   {
	errno= ENAMETOOLONG;
	return(-1);
    }

    strcpy( buff, new_fname1 );
    /* +1 for '\0' at eos */
    CMD.fnameLen1 = strlen(new_fname1) + 1;

    MAKE_FULL_PATH(full_path2, path2, -1);
    new_fname2 = convert_pound(full_path2, _my_rank );

    if ( new_fname2 == NULL )   {
	errno= ENAMETOOLONG;
	return(-1);
    }

    strcpy( buff + strlen(buff) + 1 , new_fname2);

    /* +1 for '\0' at eos */
    CMD.fnameLen2 = strlen(new_fname2) + 1;

    if (yod_switch(CMD_SYMLINK, path1, &cmd, &rpc_nid, &rpc_pid, 
                                                       &rpc_portal ) < 0) {
      errno = ERRNO_SWITCH;
      return -1;
    }

    if ( _host_rpc(&cmd, &ack, 
	  CMD_SYMLINK, rpc_nid, rpc_pid, rpc_portal,
	    buff, CMD.fnameLen1 + CMD.fnameLen2, NULL, 0) != 0 ) {

	/* could not send! Use errno set by _host_rpc() */
	return -1;
    }

    if ( ack.retVal == -1 ) {	
	errno = ack.hostErrno;
    }
    return( ack.retVal );

}  

int
truncate_yod(const char *path, off_t length)
{
#undef CMD
#define CMD     (cmd.info.truncateCmd)

char	full_path[MAXPATHLEN];
char	*new_fname;
  
hostCmd_t	cmd;
hostReply_t	ack;

int rpc_nid, rpc_pid, rpc_portal;

    MAKE_FULL_PATH(full_path, path, -1);

    new_fname = convert_pound(full_path, _my_rank);

    if( new_fname == NULL) {
	errno = ENAMETOOLONG;
	return(-1);
    }

    strcpy(full_path, new_fname);
    CMD.fnameLen = strlen(full_path) + 1;   /* + 1 for '\0' at eos */
    CMD.length = length;

    if (yod_switch( CMD_TRUNCATE, full_path, &cmd, 
                                  &rpc_nid, &rpc_pid, &rpc_portal) < 0) {
      errno = ERRNO_SWITCH;
      return -1;
    }

    if ( _host_rpc(&cmd, &ack, 
	 CMD_TRUNCATE, rpc_nid, rpc_pid, rpc_portal, 
	    full_path, CMD.fnameLen, NULL, 0) != 0) {

	/* could not send! Use errno set by _host_rpc() */
	return -1;
    }
    if ( ack.retVal == -1 ) {
	errno =  ack.hostErrno;
    }

    return( ack.retVal );

}  /* end of truncate_yod() */

char *
ttyname_yod( int fd )
{
#undef CMD
#define CMD     (cmd.info.ttynameCmd)

    static char name[MAXPATHLEN];
    char *retval = NULL; 

    hostCmd_t	cmd;
    hostReply_t	ack;

    if ( ! validFd( fd ) ) {
	errno = EBADF;
	return( NULL );
    }

    CMD.hostFileIndex = FD_ENTRY_HOST_FILE_INDEX( fd );

    cmd.uid = getuid();
    cmd.gid = getgid();
    cmd.euid = geteuid();
    cmd.egid = getegid();

    if ( _host_rpc(&cmd, &ack, 
	CMD_TTYNAME, FD_ENTRY_SRVR_NID( fd ),
	FD_ENTRY_SRVR_PID( fd ), FD_ENTRY_SRVR_PTL( fd ),
	NULL, 0, name, MAXPATHLEN ) != 0) {

	/* could not send! Use errno set by _host_rpc() */
	return( NULL );
    }

    if ( ack.retVal ) {
	retval = name;	
    } else {
	errno =  ack.hostErrno;
    }

    return( retval );

}  

int
unlink_yod( const char *path )
{
#undef CMD
#define CMD     (cmd.info.unlinkCmd)

char 	full_path[MAXPATHLEN];
char 	*new_fname;

int rpc_nid, rpc_pid, rpc_portal;

    hostCmd_t	cmd;
    hostReply_t ack;

    MAKE_FULL_PATH(full_path, path, -1);

    new_fname = convert_pound(full_path, _my_rank );

    if ( new_fname == NULL )   {
	errno= ENAMETOOLONG;
	return(-1);
    }

    strcpy(full_path, new_fname);
    CMD.fnameLen = strlen(full_path) + 1;   /* + 1 for '\0' at eos */

    if (yod_switch(CMD_UNLINK, full_path, &cmd, &rpc_nid, &rpc_pid, 
                                                          &rpc_portal ) < 0) {
      errno = ERRNO_SWITCH;
      return -1;
    }

    if ( _host_rpc(&cmd, &ack, 
	 CMD_UNLINK, rpc_nid, rpc_pid, rpc_portal,
	full_path, CMD.fnameLen, NULL, 0) != 0 ) {

	/* could not send! Use errno set by _host_rpc() */
	return -1;
    }

    if ( ack.retVal == -1 ) {
	errno = ack.hostErrno;
    }

    return( ack.retVal );
}  

#ifdef __osf__
int
#else
ssize_t 
#endif
write_yod( int fd, const void *buff, size_t nbytes)
{
#undef CMD
#undef ACK
#define CMD     (cmd.info.writeCmd)
#define ACK     (ack.info.writeAck)


    hostCmd_t	cmd;
    hostReply_t	ack;

/*	PCT_DUMP(1001, Dbglevel, 0, 0, 0, 0, 0); */

    if ( ! validFd( fd ) ) {
	errno= EBADF;
	return(-1);
    }
    /* add 1 to the flags so we can use FWRITe to extract value */ 
    if ( ! ( ( FD_FILE_STATUS_FLAGS(fd) + 1 ) & FWRITE ) ) {
	errno= EBADF;
	return(-1);
    }

    /* Setup the command for yod */
    CMD.nbytes = nbytes;
    CMD.curPos = FD_ENTRY_CURPOS( fd ); 
    CMD.hostFileIndex =  FD_ENTRY_HOST_FILE_INDEX( fd );
	CMD.rank = _my_rank;
	CMD.nnodes = _my_nnodes;

    /* at this point the host does not care about access modes mask */
    CMD.fileStatusFlags = FD_FILE_STATUS_FLAGS( fd ) & ~O_ACCMODE;


	if ( Dbglevel > 3 )
	{
		PCT_DUMP(1011, FD_ENTRY_SRVR_NID(fd), 0, 0, 0, 0, 0);
		PCT_DUMP(1012, FD_ENTRY_SRVR_PID(fd), 0, 0, 0, 0, 0);
		PCT_DUMP(1013, FD_ENTRY_SRVR_PTL(fd), 0, 0, 0, 0, 0);
	}

    cmd.uid = getuid();
    cmd.gid = getgid();
    cmd.euid = geteuid();
    cmd.egid = getegid();

    if ( _host_rpc(&cmd, &ack, 
	CMD_WRITE, FD_ENTRY_SRVR_NID( fd ), 
	FD_ENTRY_SRVR_PID( fd ), FD_ENTRY_SRVR_PTL( fd ), 
	buff, nbytes, NULL, 0) != 0 ) {

	/* could not send! Use errno set by _host_rpc() */
	return -1;
    }

    if ( ack.retVal == -1 ) {
	PCT_DUMP(9998, 0, 0, 0, 0, 0, 0);
	errno= ack.hostErrno;
    }

    FD_ENTRY_CURPOS( fd ) = ACK.curPos;

    return(ack.retVal);

}  /* end of write() */


/* used by yod/fyod-common io routines */
int 
yod_switch( int cmdID, const char *full_path, hostCmd_t *cmd, UINT32 *rpc_nid, UINT32 *rpc_pid, UINT32 *rpc_portal  )
{
    int unit, raid, subraid, rc;

    cmd->uid = getuid();
    cmd->gid = getgid();
    cmd->euid = geteuid();
    cmd->egid = getegid();

    if ( PATH_IS_FYOD(full_path) ) {

        rc=sscanf( full_path, "/raid_%3d", &unit);
        raid = unit / FYOD_FACTOR;
        subraid = unit % MAX_DISKS_PER_FYOD_NODE;

#ifdef __linux__
        PRINTF("subraid= %d\n", subraid);
        PRINTF("raid= %d\n", raid);
        PRINTF("_fyod_map[%d] = %d\n",MAX_DISKS_PER_FYOD_NODE*raid+subraid, _fyod_map[MAX_DISKS_PER_FYOD_NODE*raid+subraid]);
#endif

        *rpc_nid = (UINT32) _fyod_map[MAX_DISKS_PER_FYOD_NODE*raid+subraid];

        if (*rpc_nid < 0) {
          return -1;
        }

        *rpc_portal = 0;
        *rpc_pid = PPID_FYOD;

        switch (cmdID) {

          case CMD_ACCESS:
          case CMD_CHDIR:
          case CMD_CHMOD:
          case CMD_CHOWN:
          case CMD_LINK:
          case CMD_MKDIR:
          case CMD_OPEN:
          case CMD_RENAME:
          case CMD_RMDIR:
          case CMD_STAT:
          case CMD_SYMLINK:
          case CMD_TRUNCATE:
          case CMD_UNLINK:
            cmd->unitNumber = (UINT8) (MAX_DISKS_PER_FYOD_NODE*raid+subraid);
            break;

          default:
           break;
        }
    }
    else {
      *rpc_nid =    YOD_NID;
      *rpc_portal = YOD_PTL;
      *rpc_pid =    YOD_PID;
    }	       
    return 0;
}
