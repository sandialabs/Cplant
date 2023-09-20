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
/* $Id: stat.c,v 1.23 2002/02/12 18:51:13 pumatst Exp $ */

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include "protocol_switch.h"
#include "puma_unistd.h"
#include "puma.h"
#include "errno.h"
#include "fdTable.h"

/******************************************************************************/

/* Digital UNIX Fortran stub */
#if defined(__osf__)
int
stat_( char *path, struct stat *sbuf )
{
    return stat( path, sbuf );
}
#endif

#if defined(__linux__)
int
__xstat( int linux_version __attribute__ ((unused)), const char *path,
         struct stat *sbuf )
{
  int protocol;
  protocol = path2io_proto(path);
  return io_ops[protocol]->stat( APATH, sbuf);
}
int __xstat64(int, const char *, struct stat *) __attribute__ ((alias("__xstat")));
#else
int
stat( const char *path, struct stat *sbuf )
{
  int protocol;
  protocol = path2io_proto(path);
  return io_ops[protocol]->stat( APATH, sbuf);
}
#endif
