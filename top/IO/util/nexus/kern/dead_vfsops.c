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
 * The "dead" virtual file system operations.
 *
 */
#include "cmn.h"
#include "smp.h"
#include "vfs.h"

IDENTIFY("$Id: dead_vfsops.c,v 1.0 1999/11/17 23:33:32 lee Exp $");

static void
dead_vfsop_illop(void)
{

	panic("illegal operation on dead file system");
}

static int
dead_vfsop_nodev(void)
{

	return ENODEV;
}

static int
dead_vfsop_noent(void)
{

	return ENOENT;
}

#define dead_vfsop_unmount \
	((void (*)(struct vfs *))dead_vfsop_illop)
#define dead_vfsop_getroot \
	((int (*)(struct vfs *, rwty_t, struct vnode **))dead_vfsop_nodev)
#define dead_vfsop_statfs \
	((int (*)(struct vfs *, struct fsstats *))dead_vfsop_nodev)
#define dead_vfsop_getv \
	((int (*)(struct vfs *, z_ino_t, struct vnode **, \
		  unsigned))dead_vfsop_noent)
#define dead_vfsop_getv_by_handle \
	((int (*)(struct vfs *, struct vnhndl *, struct vnode **, \
		  unsigned))dead_vfsop_noent)
#define dead_vfsop_lock \
	vfs_generic_op_lock
#define dead_vfsop_unlock \
	vfs_generic_op_unlock
#define dead_vfsop_lkty \
	vfs_generic_op_lkty

/*
 * Virtual file system operations.
 */
struct vfsops dead_vfsops = {
	dead_vfsop_unmount,
	dead_vfsop_getroot,
	dead_vfsop_statfs,
	dead_vfsop_getv,
	dead_vfsop_getv_by_handle,
	dead_vfsop_lock,
	dead_vfsop_unlock,
	dead_vfsop_lkty
};
