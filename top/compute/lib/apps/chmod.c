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
/* $Id: chmod.c,v 1.12 2002/02/12 18:51:13 pumatst Exp $ */

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "protocol_switch.h"
#include "puma_unistd.h"
#include "puma.h"
#include "errno.h"

/******************************************************************************/

/* Digital UNIX Fortran stub */
#ifdef __osf__
int
chmod_( const char *path, mode_t mode )
{
    return chmod( path, mode );
}
#endif

int
__chmod( const char *path, mode_t mode )
{
    return chmod( path, mode );
}

int
chmod( const char *path, mode_t mode )
{
  int protocol;

  protocol = path2io_proto( path );
  return io_ops[protocol]->chmod( APATH, mode );
}
