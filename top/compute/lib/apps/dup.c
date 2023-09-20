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
/* $Id: dup.c,v 1.5 2001/02/16 05:28:43 lafisk Exp $ */

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include "protocol_switch.h"

/******************************************************************************/

/* Digital UNIX Fortran stub */
#ifdef __osf__
int
dup_( int fd )
{
    return dup( fd );
}
#endif


int
__dup( int fd )
{
    return dup( fd );
}

int
dup( int fd )
{
  int protocol;
  protocol = fd2io_proto(fd);
  return io_ops[protocol]->dup(fd);
} 

int
__dup2( int old, int new )
{
    return dup2( old, new );
}

int
dup2( int old, int new )
{
  int protocol;
  protocol = fd2io_proto(old);

  return io_ops[protocol]->dup2(old,new);
}
