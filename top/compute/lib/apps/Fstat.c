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
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include "protocol_switch.h"
#include "puma_unistd.h"
#include "puma.h"
#include "errno.h"
#include "fdTable.h"

/*
** $Id: Fstat.c,v 1.18 2002/02/12 18:51:12 pumatst Exp $
*/

/******************************************************************************/

/* Digital UNIX Fortran stub */
#ifdef __osf__
int
fstat_(int fd, struct stat *buf )
{
    return fstat( fd, buf );
}

/*
** Compaq OSF1 version 5 fix:
**
** We build with __TMP_V40_OBJ_COMPAT defined, and so use the
** older stat buffer and fstat, etc. functions.  But many of the
** libraries provided with OSF were not built this way.  We need
** to define the _F64 functions here so they are handled 
** properly.
**
** See the Fstat.OSF.Note for details.
*/
#ifdef __F64_STAT

static struct __f64_stat{
 __F64_STAT
};
static struct __old_stat{
 __PRE_F64_STAT
};
#define STATBUFCONV(newbuf, oldbuf) {      \
    memset(newbuf, 0, sizeof(struct __f64_stat)); \
    newbuf->st_dev    = oldbuf.st_dev;     \
    newbuf->st_mode   = oldbuf.st_mode;    \
    newbuf->st_nlink  = oldbuf.st_nlink;   \
    newbuf->st_uid    = oldbuf.st_uid;     \
    newbuf->st_gid    = oldbuf.st_gid;     \
    newbuf->st_rdev   = oldbuf.st_rdev;    \
    newbuf->st_ldev   = 0;                 \
    newbuf->st_size   = oldbuf.st_size;    \
    newbuf->st_flags  = oldbuf.st_flags;   \
    newbuf->st_gen    = oldbuf.st_gen;     \
    newbuf->st_ino    = oldbuf.st_ino;     \
    newbuf->st_atime  = oldbuf.st_atime;   \
    newbuf->st_uatime = oldbuf.st_uatime;  \
    newbuf->st_mtime  = oldbuf.st_mtime;   \
    newbuf->st_umtime = oldbuf.st_umtime;  \
    newbuf->st_ctime  = oldbuf.st_ctime;   \
    newbuf->st_uctime = oldbuf.st_uctime;  \
    newbuf->st_blksize= oldbuf.st_blksize; \
    newbuf->st_blocks = oldbuf.st_blocks;  \
}
int
_F64_fstat(int fd, struct __f64_stat *buf )
{
int rc;
struct stat oldbuf;    /* it's the same as __old_stat */

    rc = fstat( fd, &oldbuf );

    STATBUFCONV(buf, oldbuf);

    return rc;
}
int
_F64_lstat(char *path, struct __f64_stat *buf )
{
int rc;
struct stat oldbuf;    /* it's the same as __old_stat */

    rc = lstat( path, &oldbuf );

    STATBUFCONV(buf, oldbuf);

    return rc;
}
int
_F64_stat(char *path, struct __f64_stat *buf )
{
int rc;
struct stat oldbuf;    /* it's the same as __old_stat */

    rc = stat( path, &oldbuf );

    STATBUFCONV(buf, oldbuf);

    return rc;
}
/*
** It should not be necessary to define the __* (double underscore) versions of
** these functions.  A _* (single underscore) version should always override
** the __* version.  But some portions of Tru64's libc.a (catopen, for example)
** call __F64_fstat directly, bypassing a legitimate redefinition of this function.
*/
int
__F64_fstat(int fd, struct __f64_stat *buf )
{
    return _F64_fstat(fd, buf);
}
int
__F64_lstat(char *path, struct __f64_stat *buf )
{
    return _F64_lstat(path, buf);
}
int
__F64_stat(char *path, struct __f64_stat *buf )
{
    return _F64_stat(path, buf);
}

#endif

#endif


#if defined(__linux__)

/*
** glibc fstat call is a wrapper for fxstat
*/
int
fxstat(int linux_stat_version __attribute__ ((unused)), int fd,
       struct stat *buf )
{
  int protocol;
  protocol = fd2io_proto(fd);
  return io_ops[protocol]->fstat(fd, buf);
}

#else

int
fstat(int fd, struct stat *sbuf)
{
  int protocol;
  protocol = fd2io_proto(fd);
  return io_ops[protocol]->fstat(fd, sbuf);
}
#endif


#if defined(__linux__)
int
__fxstat(int linux_stat_version, int fd, struct stat *buf )
{
    return fxstat( linux_stat_version, fd, buf );
}
#else

int
__fstat(int fd, struct stat *buf )
{
    return fstat( fd, buf );
}

#endif

#ifdef __linux__
int __fxstat64(int, int, struct stat *) __attribute__ ((alias("__fxstat")));
#endif
