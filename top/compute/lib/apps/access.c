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
/* $Id: access.c,v 1.12 2002/02/12 18:51:12 pumatst Exp $ */
#include <string.h>
#include <unistd.h>
#include "protocol_switch.h"
#include "puma_unistd.h"
#include "puma.h"
#include "errno.h"


/******************************************************************************/

/* Digital UNIX Fortran stub */
#ifdef __osf__
int
access_(const char *path, int amode)
{
  return access( path, amode);
}
#endif


int
__access(const CHAR *path, int amode)
{
   return access( path, amode);
}

int
access(const char *path, int amode)
{
  int protocol;

  protocol = path2io_proto( path);

  return io_ops[protocol]->access( APATH, amode);
}
