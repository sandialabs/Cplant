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
/* $Id: llseek.c,v 1.1 2001/03/09 21:57:58 pumatst Exp $ */

#include <sys/types.h>
#include <unistd.h>

loff_t __lseek64( int fd, loff_t offset, int whence);
loff_t __llseek( int fd, loff_t offset, int whence);
loff_t __llseek64( int fd, loff_t offset, int whence);

/******************************************************************************/
loff_t 
__lseek64( int fd, loff_t offset, int whence) 
{
  loff_t loff;
  off_t off = (off_t) offset;
  loff = (loff_t) lseek( fd, off, whence ); 
  return loff;
}

loff_t 
__llseek( int fd, loff_t offset, int whence) 
{
  return __lseek64(fd, offset, whence);
}

loff_t 
__llseek64( int fd, loff_t offset, int whence) 
{
  return __lseek64(fd, offset, whence);
}
