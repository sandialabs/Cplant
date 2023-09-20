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
/* $Id: dummy_io_fns.c,v 1.18 2002/02/12 18:51:13 pumatst Exp $ */

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include "protocol_switch.h"
#include "puma_unistd.h"
#include "puma.h"
#include "errno.h"
#include "fdTable.h"
#include "shrink_path.h"

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

extern UINT16 _fyod_nid;
static int _fyod_dbg=0;
extern int Dbglevel;

extern int _fyod_map[FYOD_MAP_SZ];

#ifdef __linux__
#define PRINTF(args...)
#endif
 
#define PATH_IS_FYOD( S )  strncmp( (S) , "/raid_", 6) == 0 && \
	isdigit( S[6] ) && isdigit( S[7] ) && isdigit( S[8] ) && \
	S[9] == '/'

int yod_switch( int cmdID, const char *full_path, hostCmd_t *cmd, UINT32 *rpc_nid, UINT32 *rpc_pid, UINT32 *rpc_portal  );

int fstat_dummy(int fd, struct stat *buf);
int fstat_dummy2(int linux_version, int fd, struct stat *buf);
int access_dummy(const char* path, int mode);
int chdir_dummy(const char* path);
int chmod_dummy(const char* path, mode_t);
int chown_dummy(const char* path, uid_t, gid_t);
int close_dummy(int);
static int closeHost_dummy( int fd );
int creat_dummy(const char *fname, mode_t mode);
int dup_dummy( int fd );
int dup2_dummy( int old, int new );
int binval_dummy(int fd, void *p);
int fcntl_dummy( int fd, int request, int arg );
static int fcntlHost_dummy( int fd, int request, int arg );
int fstatfs_dummy(int fd, struct statfs *buf );
int fsync_dummy( int fd );
int ftruncate_dummy(int fd, long length);
int getdirentries_dummy( int fd, char *buf, int nbytes, long *basep);
int ioctl_dummy( int fd, unsigned long request, char* ptr); 
int link_dummy(  const char *path1,  const char *path2 );
off_t lseek_dummy( int fd, off_t offset, int whence );
int lstat_dummy(const char *path, struct stat *sbuf);
int lstat_dummy2( int linux_version, const char *path, struct stat *sbuf );
int mkdir_dummy( const char *path, mode_t mode);
int open_dummy(const char *fname, int flags, mode_t mode);
#ifdef __osf__
int read_dummy(int fd, void *buff, size_t nbytes);
#else
ssize_t read_dummy(int fd, void *buff, size_t nbytes);
#endif
void *mmap_dummy(void *start, size_t len, int pr, int flags, int fd, off_t offset);
int munmap_dummy(void *start, size_t len);
int rename_dummy( const char *path1, const char *path2 );
int rmdir_dummy( const char *path);
int stat_dummy( const char *path, struct stat *sbuf );
int stat_dummy2( int linux_version, const char *path, struct stat *sbuf );
int statfs_dummy( const char *path, struct statfs *sbuf );
int symlink_dummy(const  char *path1, const char *path2 );
int truncate_dummy(const char *path, off_t length);
char * ttyname_dummy( int fd );
int unlink_dummy( const char *path );
#ifdef __osf__
int write_dummy( int fd, const void *buff, size_t nbytes);
#else
ssize_t write_dummy( int fd, const void *buff, size_t nbytes);
#endif

io_ops_t io_ops_dummy =
{
  open_dummy, creat_dummy, access_dummy, chdir_dummy, chmod_dummy, chown_dummy, 
  close_dummy, dup_dummy, dup2_dummy, fcntl_dummy, binval_dummy, fstat_dummy, 
  fstatfs_dummy, fsync_dummy, 
  ftruncate_dummy, getdirentries_dummy, ioctl_dummy, link_dummy, lseek_dummy, 
  lstat_dummy,
  mkdir_dummy, read_dummy, mmap_dummy, munmap_dummy, rename_dummy, rmdir_dummy, stat_dummy, statfs_dummy,
  symlink_dummy, truncate_dummy, ttyname_dummy, unlink_dummy, write_dummy
};

int
binval_dummy(int fd, void *p)
{
  errno = EFAULT;
  return -1;
}

#if defined(__linux__)

int
fstat_dummy(int fd, struct stat *buf)
{
  return fstat_dummy2(0, fd, buf);
}

int
fstat_dummy2(int linux_stat_version __attribute__ ((unused)), int fd,
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
fstat_dummy(int fd, struct stat *sbuf)
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
access_dummy(const char *path, int amode)
{
#undef CMD
#define CMD     (cmd.info.accessCmd)

char 	full_path[MAXPATHLEN];
char 	*new_fname;

hostCmd_t	cmd;
hostReply_t 	ack;

UINT32 rpc_nid, rpc_pid, rpc_portal;

    MAKE_FULL_PATH(full_path, path, -1);

    new_fname = convert_pound(full_path, _my_rank);

    if ( new_fname == NULL )   {
	errno= ENAMETOOLONG;
	return(-1);
    }

    strcpy(full_path, new_fname);
    CMD.fnameLen = strlen(full_path) + 1;   /* + 1 for '\0' at eos */
    CMD.amode = amode;

    yod_switch(CMD_ACCESS, full_path, &cmd, &rpc_nid, &rpc_pid, &rpc_portal);

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

}  /* end of access_dummy() */


int
chdir_dummy( const char *path )
{

#undef CMD
#define CMD     (cmd.info.chdirCmd)

hostCmd_t	cmd;
hostReply_t	ack;
char		full_path[MAXPATHLEN];
char		*test_path;

UINT32 rpc_nid, rpc_pid, rpc_portal;

    MAKE_FULL_PATH(full_path, path, -1);
    test_path = shrink_path(full_path);

    if (test_path == NULL)   {
	errno= ENAMETOOLONG;
	return(-1);
    }

    CMD.fnameLen = strlen(test_path) + 1;   /* + 1 for '\0' at eos */

    yod_switch( CMD_CHDIR, full_path, &cmd, &rpc_nid, &rpc_pid, &rpc_portal );

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

}  /* end of chdir_dummy() */

int
chmod_dummy(const char *path, mode_t mode )
{

#undef CMD
#define CMD     (cmd.info.chmodCmd)

char 		full_path[MAXPATHLEN];
char 		*new_fname;

hostCmd_t	cmd;
hostReply_t 	ack;

UINT32 rpc_nid, rpc_pid, rpc_portal;


    MAKE_FULL_PATH(full_path, path, -1);

    new_fname = convert_pound(full_path, _my_rank );

    if ( new_fname == NULL )   {
	errno= ENAMETOOLONG;
	return(-1);
    }

    strcpy(full_path, new_fname);
    CMD.fnameLen = strlen(full_path) + 1;   /* + 1 for '\0' at eos */

    CMD.mode = mode;

    yod_switch(CMD_CHMOD, full_path, &cmd, &rpc_nid, &rpc_pid, &rpc_portal );

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

}  /* end of chmod_dummy() */

int 
chown_dummy( const char *path, uid_t owner, gid_t group)
{

#undef CMD
#define CMD     (cmd.info.chownCmd)

char 		full_path[MAXPATHLEN];
char 		*new_fname;

hostCmd_t	cmd;
hostReply_t 	ack;

UINT32 rpc_nid, rpc_pid, rpc_portal;

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

    yod_switch( CMD_CHOWN, full_path, &cmd, &rpc_nid, &rpc_pid, &rpc_portal );

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

}  /* end of chown_dummy() */


int close_dummy( int fd )
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
	    retVal  = closeHost_dummy( fd );
	}
	destroyFdTableEntry( FD_ENTRY( fd ) );
	FD_ENTRY( fd ) = NULL;
    }
    return( retVal );
}

static int 
closeHost_dummy( int fd )
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
creat_dummy(const char *fname, mode_t mode)
{
  return(open(fname, O_WRONLY | O_CREAT | O_TRUNC, mode));
}

int
dup_dummy( int fd )
{
  return( fcntl( fd, F_DUPFD, 0 ) ); 
}

int
dup2_dummy( int old, int new )
{
    close( new ); 
    return( fcntl( old, F_DUPFD, new ) ); 
}

int 
fcntl_dummy( int fd, int request, int arg )
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
	    return( fcntlHost_dummy( fd, request, arg ) );
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
fcntlHost_dummy( int fd, int request, int arg )
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
fstatfs_dummy(int fd, struct statfs *buf )
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
fstatfs_dummy(int fd, struct statfs *buf )
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
fsync_dummy( int fd )
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
ftruncate_dummy(int fd, long length)
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

}  /* end of truncate_dummy() */

int
getdirentries_dummy( int fd, char *buf, int nbytes, long *basep
#ifdef __GNUC__
__attribute__ ((unused))
#endif
)
{
#undef CMD
#define CMD     (cmd.info.getdirentriesCmd)

  

hostCmd_t	cmd;
hostReply_t	ack;

    CMD.hostFileIndex = FD_ENTRY_HOST_FILE_INDEX( fd ); 
    CMD.nbytes = nbytes;

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


    return( ack.retVal );
				   
}  

int ioctl_dummy(int fd, unsigned long request, char* ptr)
{
  /* go back thru the orig libc call */
  extern int __ioctl(int fd, unsigned long request, ...);
  return __ioctl(fd, request, ptr);
}

int
link_dummy(  const char *path1,  const char *path2 )
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

UINT32 rpc_nid, rpc_pid, rpc_portal;

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

    yod_switch(CMD_LINK, full_path1, &cmd, &rpc_nid, &rpc_pid, &rpc_portal );

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
lseek_dummy( int fd, off_t offset, int whence )
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
                  3) which calls lstat_dummy2 w/ linux lstat/lxstat args...
*/
int lstat_dummy(const char *path, struct stat *sbuf) 
{
  return lstat_dummy2( 0, path, sbuf);
}

int
lstat_dummy2( int linux_version __attribute__ ((unused)), const char *path,
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

int lstat_dummy(const char *path, struct stat *sbuf) 
{
  return lstat_dummy2( 0, path, sbuf);
}

int
lstat_dummy2( int linux_version, const char *path, struct stat *sbuf )
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
mkdir_dummy( const char *path, mode_t mode)
{
#undef CMD
#define CMD     (cmd.info.mkdirCmd)

char 	full_path[MAXPATHLEN];
char 	*new_fname;

hostCmd_t	cmd;
hostReply_t 	ack;

UINT32 rpc_nid, rpc_pid, rpc_portal;

    MAKE_FULL_PATH(full_path, path, -1);

    new_fname = convert_pound(full_path, _my_rank );

    if ( new_fname == NULL )   {
	errno= ENAMETOOLONG;
	return(-1);
    }

    strcpy(full_path, new_fname);
    CMD.fnameLen = strlen(full_path) + 1;   /* + 1 for '\0' at eos */

    CMD.mode = mode & (~(_my_umask & 0000777));

    yod_switch( CMD_MKDIR, full_path, &cmd, &rpc_nid, &rpc_pid, &rpc_portal );

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

}  /* end of mkdir_dummy() */


int
open_dummy(const char *fname, int flags, mode_t mode)
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
UINT32          rpc_nid;
UINT32          rpc_pid;
UINT32          rpc_portal;

    if ( _fyod_dbg != 0 ) {
	printf ("open: Open entered Version July  99 \n");
	printf ("open: Open file limit = %d\n", _NFILE);
    }

    for ( fd = 0; fd < _NFILE; fd++) 
    {
	if ( FD_ENTRY( fd ) == NULL )  
	    break;	
    }

    /* too many open files? */
    if ( fd == _NFILE ) {
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

    yod_switch( CMD_OPEN, full_path, &cmd, &rpc_nid, &rpc_pid, &rpc_portal );
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

	FD_ENTRY_PROTOCOL(fd) = DUMMY_IO_PROTO;
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
}  /* end of open_dummy() */

#ifdef __osf__
int
#else
ssize_t
#endif
read_dummy(int fd, void *buff, size_t nbytes)
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

void *
mmap_dummy(void *start, size_t len, int prot, int flags, int fd, off_t offset)
{
   return (void *)MAP_FAILED;
}
int
munmap_dummy(void *start, size_t len)
{
   return -1;
}
int
rename_dummy( const char *path1, const char *path2 )
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

UINT32 rpc_nid, rpc_pid, rpc_portal;

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

    yod_switch(CMD_RENAME, full_path1, &cmd, &rpc_nid, &rpc_pid, &rpc_portal );

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
rmdir_dummy( const char *path)
{
#undef CMD
#define CMD     (cmd.info.rmdirCmd)

CHAR 	full_path[MAXPATHLEN];
CHAR 	*new_fname;

hostCmd_t	cmd;
hostReply_t 	ack;

UINT32 rpc_nid, rpc_pid, rpc_portal;

    MAKE_FULL_PATH(full_path, path, -1);

    new_fname = convert_pound(full_path, _my_rank );

    if ( new_fname == NULL )   {
	errno= ENAMETOOLONG;
	return(-1);
    }

    strcpy(full_path, new_fname);
    CMD.fnameLen = strlen(full_path) + 1;   /* + 1 for '\0' at eos */

    yod_switch(CMD_RMDIR, full_path, &cmd, &rpc_nid, &rpc_pid, &rpc_portal );

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

}  /* end of rmdir_dummy() */

#if defined(__linux__)
int
stat_dummy( const char *path, struct stat *sbuf )
{
  return stat_dummy2(0, path, sbuf);
}

int
stat_dummy2( int linux_version __attribute__ ((unused)), const char *path,
         struct stat *sbuf )
{
#undef CMD
#define CMD     (cmd.info.statCmd)

char    	full_path[MAXPATHLEN];
char    	*new_fname;

hostCmd_t	cmd;
hostReply_t	ack;

UINT32   rpc_nid, rpc_pid, rpc_portal;

    MAKE_FULL_PATH(full_path, path, -1);

    new_fname = convert_pound(full_path, _my_rank );
	
    if ( new_fname == NULL ) {
	errno= ENAMETOOLONG;
	return(-1);
    }

    strcpy(full_path, new_fname);
    CMD.fnameLen = strlen(full_path) + 1;   /* + 1 for '\0' at eos */

    yod_switch(CMD_STAT, full_path, &cmd, &rpc_nid, &rpc_pid, &rpc_portal);

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
stat_dummy( const char *path, struct stat *sbuf )
{
#undef CMD
#define CMD     (cmd.info.statCmd)

char    	full_path[MAXPATHLEN];
char    	*new_fname;

hostCmd_t	  cmd;
hostReply_t	  ack;
struct linux_stat linux_stat;

UINT32            rpc_nid;
UINT32            rpc_pid;
UINT32            rpc_portal;

    MAKE_FULL_PATH(full_path, path, -1);

    new_fname = convert_pound(full_path, _my_rank );
	
    if ( new_fname == NULL ) {
	errno= ENAMETOOLONG;
	return(-1);
    }

    strcpy(full_path, new_fname);
    CMD.fnameLen = strlen(full_path) + 1;   /* + 1 for '\0' at eos */

    yod_switch(CMD_STAT, full_path, &cmd, &rpc_nid, &rpc_pid, &rpc_portal);

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
statfs_dummy( const char *path, struct statfs *sbuf )
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
statfs_dummy( const char *path, struct statfs *sbuf )
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
symlink_dummy(const  char *path1, const char *path2 )
{
#undef CMD
#define CMD     (cmd.info.linkCmd)

char 	full_path2[MAXPATHLEN];
char 	buff[MAXPATHLEN*2+2];
char 	*new_fname1;
char 	*new_fname2;

hostCmd_t	cmd;
hostReply_t ack;

UINT32 rpc_nid, rpc_pid, rpc_portal;

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

    yod_switch(CMD_SYMLINK, path1, &cmd, &rpc_nid, &rpc_pid, &rpc_portal );

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
truncate_dummy(const char *path, off_t length)
{
#undef CMD
#define CMD     (cmd.info.truncateCmd)

char	full_path[MAXPATHLEN];
char	*new_fname;
  
hostCmd_t	cmd;
hostReply_t	ack;

UINT32 rpc_nid, rpc_pid, rpc_portal;

    MAKE_FULL_PATH(full_path, path, -1);

    new_fname = convert_pound(full_path, _my_rank);

    if( new_fname == NULL) {
	errno = ENAMETOOLONG;
	return(-1);
    }

    strcpy(full_path, new_fname);
    CMD.fnameLen = strlen(full_path) + 1;   /* + 1 for '\0' at eos */
    CMD.length = length;

    yod_switch( CMD_TRUNCATE, full_path, &cmd, &rpc_nid, &rpc_pid, &rpc_portal );

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

}  /* end of truncate_dummy() */

char *
ttyname_dummy( int fd )
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
unlink_dummy( const char *path )
{
#undef CMD
#define CMD     (cmd.info.unlinkCmd)

char 	full_path[MAXPATHLEN];
char 	*new_fname;

UINT32 rpc_nid, rpc_pid, rpc_portal;

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

    yod_switch(CMD_UNLINK, full_path, &cmd, &rpc_nid, &rpc_pid, &rpc_portal );

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
write_dummy( int fd, const void *buff, size_t nbytes)
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

