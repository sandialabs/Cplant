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
/*
** $Id: fileio.h,v 1.4 2001/11/01 01:36:11 pumatst Exp $
** Portals 3.0 module file that deals with access through /dev/protals3
*/

#ifndef P3_FILEIO_H
#define P3_FILEIO_H

#include <linux/fs.h>

#define P3_MAJOR	(62)
#define P3_DEV_NAME	"portals3"


/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

int p3ioctl(struct inode*, struct file*, unsigned int, unsigned long);
int p3open(struct inode *inodeP, struct file *fileP);
int p3release(struct inode *inodeP, struct file *fileP);

extern struct file_operations p3_fops;

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

#endif /* P3_FILEIO_H */
