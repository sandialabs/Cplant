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
#include "cmn.h"
#include "smp.h"
#include "vnode.h"
/*
 * Dead file system vnode operations.
 *
 */

IDENTIFY("$Id: dead_vops.c,v 1.0 1999/11/17 23:33:32 lee Exp $");

#define dead_vop_getattr \
	((int (*)(struct vnode *, struct vstat *))dead_vop_ebadf)

#define dead_vop_setattr \
	((int (*)(struct vnode *, struct vstat *, \
		  struct creds *))dead_vop_ebadf)

static int
dead_vop_lookup(struct vnode *vp,
		const char *name IS_UNUSED,
		struct creds *crp IS_UNUSED,
		rwty_t lkf IS_UNUSED,
		struct vnode **vpp IS_UNUSED)
{

	VOP_UNLOCK(vp);
	return ENOENT;
}

#define dead_vop_readlink \
	((int (*)(struct vnode *, char *, u_int32_t *))dead_vop_ebadf)

#define dead_vop_read \
	((int (*)(struct vnode *, z_off_t, char *, u_int32_t *, \
		  struct creds *))dead_vop_eio)

#define dead_vop_write \
	((int (*)(struct vnode *, z_off_t, char *, u_int32_t *, \
		  struct creds *))dead_vop_eio)

#define dead_vop_create \
	((int (*)(struct vnode *, const char *, struct vstat *, \
		  struct creds *, rwty_t, struct vnode **))dead_vop_illop)

#define dead_vop_remove \
	((int (*)(struct vnode *, const char *, struct vnode *, \
		  struct creds *))dead_vop_illop)

#define dead_vop_rename \
	((int (*)(struct vnode *, const char *, \
		  struct vnode *, \
		  struct vnode *, const char *, \
		  struct creds *))dead_vop_illop)

#define dead_vop_link \
	((int (*)(struct vnode *, struct vnode *, const char *, \
		  struct creds *))dead_vop_illop)

#define dead_vop_symlink \
	((int (*)(struct vnode *, const char *, const char *, \
		  struct vstat *, struct creds *))dead_vop_illop)

#define dead_vop_mkdir \
	((int (*)(struct vnode *, const char *, struct vstat *, \
		  struct creds *, rwty_t, struct vnode **))dead_vop_illop)

#define dead_vop_rmdir \
	((int (*)(struct vnode *, const char *, struct vnode *, \
		  struct creds *))dead_vop_illop)

#define dead_vop_readdir \
	((int (*)(struct vnode *, \
		  z_off_t *, char *, u_int32_t *, \
		  int *, struct creds *))dead_vop_illop)

#define dead_vop_handle \
	((int (*)(struct vnode *, struct vnhndl *))v_generic_op_nosys)

#define dead_vop_lock \
	v_generic_op_lock

#define dead_vop_unlock \
	v_generic_op_unlock

#define dead_vop_lkty \
	v_generic_op_lkty

#define dead_vop_reclaim \
	((void (*)(struct vnode *))dead_vop_illop)

static int
dead_vop_ebadf(void)
{

	return EBADF;
}

static int
dead_vop_eio(void)
{

	return EIO;
}

static void
dead_vop_illop(void)
{

	panic("illegal operation on dead vnode");
}

struct vops dead_vops = {
	dead_vop_getattr,
	dead_vop_setattr,
	dead_vop_lookup,
	dead_vop_readlink,
	dead_vop_read,
	dead_vop_write,
	dead_vop_create,
	dead_vop_remove,
	dead_vop_rename,
	dead_vop_link,
	dead_vop_symlink,
	dead_vop_mkdir,
	dead_vop_rmdir,
	dead_vop_readdir,
	dead_vop_handle,
	dead_vop_lock,
	dead_vop_unlock,
	dead_vop_lkty,
	dead_vop_reclaim
};
