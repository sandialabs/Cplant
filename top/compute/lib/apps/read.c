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
/* $Id: read.c,v 1.13 2002/02/12 18:51:13 pumatst Exp $ */

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "protocol_switch.h"
#include "puma.h"
#include "puma_unistd.h"
#include "fdTable.h"
#include "errno.h"

#include "puma_io.h"

#define vdebug(type, var) fprintf(stderr, #var"=%"#type"\n", var)

/******************************************************************************/


/* Digital UNIX Fortran stub */
ssize_t
read_(int fd, void *buff, size_t nbytes)
{
    return read(fd, buff, nbytes);
}

ssize_t
__read(int fd, void *buff, size_t nbytes)
{
    return read(fd, buff, nbytes);
}


ssize_t
read(int fd, void *buff, size_t nbytes)
{
  int protocol;
  protocol = fd2io_proto(fd);
  return io_ops[protocol]->read(fd, buff, nbytes);
}
