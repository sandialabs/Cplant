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
/* $Id: ioctl.c,v 1.4 2001/02/16 05:29:18 lafisk Exp $ */

#include <sys/ioctl.h>
#include <stdio.h>
#include <stdarg.h>
#include "protocol_switch.h"

#ifdef __osf__
int ioctl(int fd, int request, ...) 
#else
int ioctl(int fd, unsigned long int request, ...) 
#endif
{
  int protocol;
  va_list arg;
  char* ptr;

  va_start(arg, request);
  ptr = va_arg(arg, char*); 
  va_end(arg);

  protocol = fd2io_proto(fd);
  return io_ops[protocol]->ioctl(fd, request, ptr);
}
