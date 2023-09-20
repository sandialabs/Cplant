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
 * Virtual file stystem configuration table.
 */
#include <stdio.h>

#include "cmn.h"
#include "smp.h"
#include "vfs.h"

IDENTIFY("$Id: vfs_conf.c,v 1.4 2000/03/03 23:10:00 lward Exp $");

extern int nfs_mount(const void *, size_t, struct creds *, struct vfs **);
#ifdef SKELFS
extern int skelfs_mount(const void *, size_t, struct creds *, struct vfs **);
#endif

struct vfsconf vfsconftbl[] = {
	{ "nfs",	nfs_mount },
#ifdef SKELFS
	{ "skelfs",	skelfs_mount },
#endif
	{ "",		NULL }
};
