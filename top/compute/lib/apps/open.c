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
/* $Id: open.c,v 1.35 2002/02/26 19:18:27 rklundt Exp $ */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include "protocol_switch.h"
#include "puma.h"
#include "puma_unistd.h"
#include "fdTable.h"
#include "errno.h"
#include "ppid.h"
#include "pf_io.h"


int 
__open(const char *fname, int flags, ...)
{
  int fd, protocol;

  va_list arg;
  mode_t mode;

    if (flags & O_CREAT) {
      va_start(arg, flags);
      mode = va_arg(arg, mode_t);
      va_end(arg);
    } 
    else {
      mode = 0;
    }

    if ( !___startup_complete ) {
      return pf_open(fname, flags, mode);
    }

    protocol = path2io_proto( fname );

    fd = io_ops[protocol]->open( APATH, flags, mode);

    return fd;
}


int 
open(const char *fname, int flags, ...)
{
  int fd, protocol;

  va_list arg;
  mode_t mode;

    if (flags & O_CREAT) {
      va_start(arg, flags);
      mode = va_arg(arg, mode_t);
      va_end(arg);
    } 
    else {
      mode = 0;
    }

    protocol = path2io_proto( fname );

    fd = io_ops[protocol]->open( APATH, flags, mode);

    return fd;
}  /* end of open() */

#ifdef __linux__
int
open64(const char *fname, int flags, int mode)
{
  return open(fname, flags, mode);
}

int
__open64(const char *fname, int flags, int mode)
{
  return __open(fname, flags, mode);
}
#endif

/******************************************************************************/

int
creat(const char *fname, mode_t mode)
{
    int fd, protocol;

    protocol = path2io_proto( fname );

    fd = io_ops[protocol]->creat( APATH, mode);

    return fd;
}
