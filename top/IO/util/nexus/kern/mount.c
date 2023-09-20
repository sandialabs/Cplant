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
#include <stdio.h>
#include <string.h>

#include "cmn.h"
#include "queue.h"
#include "vfs.h"
#include "vnode.h"

IDENTIFY("$Id: mount.c,v 1.2 1980/01/06 13:08:19 lward Exp $");

/*
 * Common parts of mount operations.
 */
static int
mount_common(const char *type,
	     const void *args,
	     size_t argslen,
	     struct creds *crp,
	     struct vfs **vfspp,
	     struct vnode **vpp)
{
	int	err;
	struct vfs *vfsp;
	struct vnode *vp;

	err = vfs_mount(type, args, argslen, crp, &vfsp);
	if (err)
		return err;

	err = VFSOP_GETROOT(vfsp, RWLK_WRITE, &vp);

	if (err) {
		int	err2;

		err2 = vfs_umount(vfsp, crp);
		if (err2)
			panic("mount_common: can't undo mount after err (%s)",
			      strerror(err));
	} else {
		*vfspp = vfsp;
		*vpp = vp;
	}

	return err;
}

/*
 * Mount a file system.
 */
int
mount_fs(const char *type,
	 const char *path,
	 const void *args,
	 size_t argslen,
	 struct creds *crp)
{
	int	err;
	struct namei_data nd;
	struct vfs *vfsp;
	struct vnode *vp;

	if (rootvp == NULL) {
		LOG(LOG_ERR, "mount_fs: no root yet");
		return ENOENT;
	}

	err = v_get(rootvp, RWLK_NONE, 0);
	if (err) {
		LOG(LOG_ERR, "mount_fs: can't get root");
		return err;
	}

	nd.nd_flags = 0;
	nd.nd_parent = rootvp;
	nd.nd_path = path;
	nd.nd_vp = NULL;
	nd.nd_lkf = RWLK_WRITE;
	nd.nd_crp = crp;
	err = namei(&nd);
	if (!err && nd.nd_vp == rootvp) {
		/*
		 * Don't allow them to mount over the root vnode.
		 */
		err = EBUSY;
		v_put(nd.nd_vp);
		nd.nd_vp = NULL;
	}
	v_rele(rootvp);
	if (err)
		return err;

	err = mount_common(type, args, argslen, crp, &vfsp, &vp);
	if (err) {
		v_put(nd.nd_vp);
		return err;
	}
	nd.nd_vp->v_cover = vp;
	VOP_UNLOCK(nd.nd_vp);
	vp->v_mountpoint = nd.nd_vp;
	VOP_UNLOCK(vp);
	vfs_put(vfsp);

	/*
	 * Keep the reference on the two vnodes. We don't want them
	 * reclaimed.
	 */
	return err;
}

/*
 * Mount the root file system.
 */
int
mount_root(const char *type,
	   const void *args,
	   size_t argslen,
	   struct creds *crp)
{
	int	err;
	struct vfs *vfsp;

	if (rootvp != NULL) {
		LOG(LOG_ERR, "mount_root: attempt to remount root");
		return EBUSY;
	}

	err = mount_common(type, args, argslen, crp, &vfsp, &rootvp);
	if (err)
		return err;

	/*
	 * We leave the reference on the root vnode so that
	 * it won't be reclaimed. As well, since it's the root
	 * of it's filesystem, as well as the root of the entire
	 * name space, it needs to be covering something. It covers
	 * itself.
	 */
	rootvp->v_cover = rootvp->v_mountpoint = rootvp;
	vfs_put(vfsp);
	VOP_UNLOCK(rootvp);

	return err;
}

/*
 * Unmount the root file system.
 *
 * Hmm, I believe we don't have to check for other mounted filesystems. They
 * would be covering a vnode on the root and a later check would catch that.
 */
int
umount_root(void)
{
	static mutex_t mymutex = MUTEX_INITIALIZER;
	int	err;
	struct vfs *vfsp;

	err = 0;

	mutex_lock(&mymutex);
	if (rootvp == NULL)
		err = ENODEV;
	if (!err)
		err = v_get(rootvp, RWLK_READ, 0);
	if (!err) {
		vfsp = rootvp->v_vfsp;
		err = vfs_get(vfsp, RWLK_WRITE, 0);
		v_put(rootvp);
	}
	if (!err)
		err = vfs_umount(rootvp->v_vfsp, &suser_creds);
	if (!err)
		rootvp = NULL;
	mutex_unlock(&mymutex);

	return err;
}
