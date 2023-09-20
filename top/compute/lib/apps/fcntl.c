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
/* $Id: fcntl.c,v 1.9 2002/02/12 18:51:13 pumatst Exp $ */

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include <stdarg.h>

#include "protocol_switch.h"
#include "puma.h"
#include "puma_unistd.h"
#include "fdTable.h"
#include "errno.h"


/******************************************************************************/

#if 0
/*
** we need access to the native __fcntl (to manipulate /dev/portals)
** so let's not define our own here.
*/
#ifdef __osf__
int
__fcntl( int fd, int request, ... )
{
va_list arg;
int argument = 0;
  
    if ( ! validFd( fd ) ) {
	errno= EBADF;
	return(-1);
    }

    switch( request ) {
	case F_DUPFD:
	case F_SETFD:
	case F_SETFL:
	case F_SETOWN:
	case F_GETLK: 
	case F_SETLK: 
	case F_SETLKW:
	    va_start( arg, request );
	    argument = va_arg( arg, INT32 );
	    va_end( arg );
	    break;
    }
    return( fcntl2( fd, request, argument ) );
}
#endif
#endif

int
fcntl( int fd, int request, ... )
{
  int protocol;
  va_list arg;
  int argument = 0;
  
    if ( ! validFd( fd ) ) {
	errno= EBADF;
	return(-1);
    }
    switch( request ) {
	case F_DUPFD:
	case F_SETFD:
	case F_SETFL:
	case F_SETOWN:
	case F_GETLK: 
	case F_SETLK: 
	case F_SETLKW:
	    va_start( arg, request );
	    argument = va_arg( arg, int );
	    va_end( arg );
	    break;
    }
    protocol = fd2io_proto(fd);
    return io_ops[protocol]->fcntl(fd, request, argument);
    //return( fcntl2( fd, request, argument ) );
}

#ifdef __GNUC__
int __fcntl( int fd, int request, ... )
  __attribute__ ((weak, alias ("fcntl")));
#endif
