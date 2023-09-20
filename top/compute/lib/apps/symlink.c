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
/* $Id: symlink.c,v 1.9 2002/02/12 18:51:13 pumatst Exp $ */

#include <unistd.h>
#include <string.h>
#include "protocol_switch.h"
#include "puma_unistd.h"
#include "puma.h"
#include "errno.h"

/* Digital UNIX Fortran stub */
#ifdef __osf__
int
symlink_(const  char *path1, const char *path2 )
{
    return symlink( path1, path2 );
}
#endif


int
__symlink(const  char *path1, const char *path2 )
{
    return symlink( path1, path2 );
}

int
symlink(const  char *path1, const char *path2 )
{
  int protocol;
  protocol = path2io_proto2(path1, path2);
  if (protocol < 0) {
    errno = EFAULT;
    return -1;
  }
  return io_ops[protocol]->symlink(APATH, BPATH);
}
