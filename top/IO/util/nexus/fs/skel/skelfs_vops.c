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
#ifdef SKELFS
#include <stdlib.h>
#include <string.h>
#include "skelfs.h"

/*
 * skelfs_vops: VFS node operations for a simple directory file system.
 *
 * Author: Scott Morgan, Abba Technologies (smorgan@sandia.gov)
 *
 */

/*
 * dtree node directory ops. since this is a simple directory file system, 
 * we only need attribute functions.
 */

int
skelfs_dtree_gattr(dtree_ino *inode IS_UNUSED, dtree_prvattr *attr)
{
	/* attributes are assumed */
	attr->dta_size = (u_int64_t ) VMPAGESIZE;
	attr->dta_blocksize = (u_int32_t ) 0;
	attr->dta_rdev = (z_dev_t ) 0;
	attr->dta_blocks = (u_int32_t ) 1;

	return 0;
}

int
skelfs_dtree_sattr(dtree_ino *inode IS_UNUSED, dtree_prvattr *attr IS_UNUSED)
{
	/* can't set any directory attributes anyway */
	return 0;
}

dtree_ino_ops skelfs_ino_ops = {
	NULL,				/* no non-dir inode creation */
	NULL,				/* no inode read */
	NULL,				/* no inode write */
	skelfs_dtree_gattr,
	skelfs_dtree_sattr,
	NULL
};

int
skelfs_findi(struct skelfs *skelfs, dtree_ino *ino, rwty_t lock,
	     struct vnode **vnode)
{
	int			ret = 0;
	struct vnode		*vp = NULL;
	extern struct vops	skelfs_vops;

	/* lock out any other callers from instancing this vnode */
	do {
		ret = vfs_vfind(skelfs->sfs_dev, ino->dti_inum, lock, 0, &vp);

		if (ret == ENOENT) {
			ret = 0;
			/* create the new vnode */
			vp = v_new(skelfs->sfs_vfs, ino->dti_inum,
				       &skelfs_vops, ino);

			if (vp == NULL) {
				/*
				 * another thread has instanced this vnode:
				 * reacquire it.
				 */
				continue;
			}
			else {
			  /* reset the lock to the caller's desire */
			  if (lock == RWLK_READ) {
				 VOP_UNLOCK(vp);
				 VOP_LOCK(vp, lock, 0);
			  }
			}
		}
	} while (vp == NULL && ret == 0);

	*vnode = vp;

	return ret;
}

int
skelfs_vop_getattr(struct vnode *vnode, struct vstat *attr)
{
	dtree_ino		*ino = V2SKEL(vnode);

	/* do we need to insure (assert()) a read lock here? */

	return dtree_getattr(ino, attr);
}

int
skelfs_vop_setattr(struct vnode *vnode, struct vstat *attr, struct creds *cr)
{
	dtree_ino		*ino = V2SKEL(vnode);

	assert(VOP_LKTY(vnode) == RWLK_WRITE);

	return dtree_setattr(ino, attr, cr);
}

int
skelfs_vop_lookup(struct vnode *dvnode, const char *name, struct creds *cr,
		  rwty_t lock, struct vnode **vnode)
{
	int			ret;
	dtree_ino		*dino = V2SKEL(dvnode);
	dtree_ino		*ino;
	struct skelfs		*skelfs = V2SKELFS(dvnode);

	/* search for the vnode in the vnode cache. */
	ret = dtree_lookup(dino, (char *) name, cr, &ino);

	/* get a corresponding vnode */
	if (ret == 0)
		ret = skelfs_findi(skelfs, ino, lock, vnode);

	/*
	 * NOTE: IMPORTANT IMPLEMENTATION REQUIREMENT!
	 * 
	 * unlock the parent directory: since the upper VFS/NFS service
	 * layer may need to reacquire a lock on this directory for a
	 * subsequent operation, it is incumbent upon the file system
	 * layer to drop this lock when no longer necessary.
	 *
	 * we're dropping the lock after the vnode instantiation call
	 * (skelfs_findi()) to prevent races with other callers from
	 * instantiating the vnode first.
	 */
	VOP_UNLOCK(dvnode);

	return ret;
}

/*
 * the following operations are not necessary for an internally defined
 * skeletal file system.
 */
int
skelfs_vop_readlink(struct vnode *vnode, char *buf, u_int32_t *len)
{
	dtree_ino		*ino = V2SKEL(vnode);

	return dtree_readlink(ino, buf, len);
}

int
skelfs_vop_remove(struct vnode *dvnode, const char *name, struct vnode *vnode,
		  struct creds *cr)
{
	dtree_ino		*dino = V2SKEL(dvnode);
	dtree_ino		*ino = V2SKEL(vnode);

	assert(VOP_LKTY(dvnode) == RWLK_WRITE && VOP_LKTY(vnode) == RWLK_WRITE);

	/*
	 * the skeletal file system does not support ordinary files. as such,
	 * the only remove supported is for symbolic links.
	 *
	 * this check is purely paranoia: since there is no supported file
	 * creation routine, the only possible inode type to remove is a
	 * symbolic link....
	 */
	if (!S_ISLNK(ino->dti_mode))
		return EPERM;

	return dtree_remove(dino, (char *) name, ino, cr);
}

int
skelfs_vop_rename(struct vnode *fdvnode, const char *fname, struct vnode *vnode,
		  struct vnode *tdvnode, const char *tname, struct creds *cr)
{
	dtree_ino		*fdino = V2SKEL(fdvnode),
				*tdino = V2SKEL(tdvnode),
				*ino = V2SKEL(vnode);

	assert(VOP_LKTY(fdvnode) == RWLK_WRITE &&
	       VOP_LKTY(tdvnode) == RWLK_WRITE && v_islocked(vnode));

	if (fdvnode->v_vfsp != tdvnode->v_vfsp)
		return EXDEV;

	return dtree_rename(fdino, (char *) fname, ino, tdino, (char *) tname,
			    cr);
}

int
skelfs_vop_symlink(struct vnode *dvnode, const char *name, const char *path,
		   struct vstat *attr, struct creds *cr)
{
	dtree_ino		*dino = V2SKEL(dvnode);

	assert(VOP_LKTY(dvnode) == RWLK_WRITE);

	return dtree_symlink(dino, (char *) name, (char *) path,
			     (dtree_attr *) attr, cr);
}

int
skelfs_vop_mkdir(struct vnode *dvnode, const char *name, struct vstat *attr,
		 struct creds *cr, rwty_t lock, struct vnode **vnode)
{
	int			ret = 0;
	dtree_ino		*dino = V2SKEL(dvnode);
	dtree_ino		*ino;
	struct skelfs		*skelfs = V2SKELFS(dvnode);

	assert(VOP_LKTY(dvnode) == RWLK_WRITE);

	/* make the directory inode */
	if ((ret = dtree_mkdir(dino, (char *) name, (dtree_attr *) attr, cr,
			       &skelfs_ino_ops, &ino)) != 0)
		return ret;

	/* get a corresponding vnode */
	ret = skelfs_findi(skelfs, ino, lock, vnode);

	return ret;
}

int
skelfs_vop_rmdir(struct vnode *dvnode, const char *name, struct vnode *vnode,
		 struct creds *cr)
{
	int			ret = 0;
	dtree_ino		*dino = V2SKEL(dvnode);
	dtree_ino		*ino = V2SKEL(vnode);

	assert(VOP_LKTY(dvnode) == RWLK_WRITE &&
	       VOP_LKTY(vnode) == RWLK_WRITE);

	if ((ret = dtree_rmdir(dino, (char *) name, ino, cr)) != 0)
		return ret;

	v_put(vnode);

	return ret;
}

int
skelfs_vop_readdir(struct vnode *dvnode, z_off_t *off, char *buf,
		   u_int32_t *buflen, int *eod, struct creds *cr)
{
	dtree_ino		*dino = V2SKEL(dvnode);

	assert(VOP_LKTY(dvnode) == RWLK_READ);

	return dtree_readdir(dino, off, buf, buflen, eod, cr);
}

int
skelfs_vop_handle(struct vnode *vnode, struct vnhndl *handle)
{
	int			ret = 0;
	dtree_ino		*ino = V2SKEL(vnode);

	/*
	 * the only handle required is the inode/dev number itself: the
	 * skeletal filesystem id encodes the instance of the skelfs.
	 */

	if (ino->dti_fh == NULL) {
		/* create the file handle */
		if ((ino->dti_fh = m_alloc(SKELFS_FHSIZE)) == NULL)
			ret = ENOMEM;
		else {
			(void) memset(ino->dti_fh, 0, SKELFS_FHSIZE);
			(void) memcpy(ino->dti_fh, &ino->dti_dev,
				      sizeof(z_dev_t));
			(void) memcpy((char *)ino->dti_fh + sizeof(z_dev_t),
				      &ino->dti_inum, sizeof(z_ino_t));
		}
	}

	if (!ret) {
		handle->vnhnd_data = (const char *) ino->dti_fh;
		handle->vnhnd_len = SKELFS_FHSIZE;
	}

	return ret;
}

int
skelfs_vop_lock(struct vnode *vp, rwty_t lock, int dontwait)
{
	dtree_ino		*ino = V2SKEL(vp);

	return dtree_ino_lock(ino, lock, dontwait);
}

void
skelfs_vop_unlock(struct vnode *vp)
{
	dtree_ino		*ino = V2SKEL(vp);

	return dtree_ino_unlock(ino);
}

rwty_t
skelfs_vop_lkty(struct vnode *vp)
{
	dtree_ino		*ino = V2SKEL(vp);

	return dtree_ino_lkty(ino);
}

void
skelfs_vop_reclaim(struct vnode *vnode)
{
	dtree_ino		*ino = V2SKEL(vnode);

	/*
	 * since the directory tree inode now holds the vnode lock,
	 * the inode resources cannot be released until the vnode
	 * is reclaimed.
	 */

	/* !!! Do not destroy dtree inodes !!!
	 * These are only destroyed at umount time.
	 */

/* 	dtree_ino_destroy(ino); */

	return;
}

/* operations not supported by the skeletal file system */
#define skelfs_vop_read \
	(int (*)(struct vnode *, z_off_t, char *, u_int32_t *, \
		 struct creds *)) \
			v_generic_op_nosys
#define skelfs_vop_write \
	(int (*)(struct vnode *, z_off_t, char *, u_int32_t *, \
		 struct creds *)) \
			v_generic_op_nosys
#define skelfs_vop_create \
	(int (*)(struct vnode *, const char *, struct vstat *, struct creds *, \
		 rwty_t, struct vnode **)) \
			v_generic_op_nosys
#define skelfs_vop_link \
	(int (*)(struct vnode *, struct vnode *, const char *, \
		 struct creds *)) \
			v_generic_op_nosys

/*
 * a purely internal name space does not require dynamic directory
 * operations as above. if a non-dynamic name space is required (using
 * skelfs_register() to create the file system name space), enable the
 * defines below and disable the actual implementations above.
 */

#if 0
#define skelfs_vop_rename \
	(int (*)(struct vnode *, const char *, struct vnode *, struct vnode *, \
		 const char *, struct creds *)) \
			v_generic_op_nosys
#define skelfs_vop_readlink \
	(int (*)(struct vnode *, char *, u_int32_t *)) \
			v_generic_op_nosys
#define skelfs_vop_remove \
	(int (*)(struct vnode *, const char *, struct vnode *, \
		 struct creds *)) \
			v_generic_op_nosys
#define skelfs_vop_symlink \
	(int (*)(struct vnode *, const char *, const char *, struct vstat *, \
		 struct creds *)) \
			v_generic_op_nosys
#define skelfs_vop_mkdir \
	(int (*)(struct vnode *, const char *, struct vstat *, struct creds *, \
		 rwty_t, struct vnode **)) \
			v_generic_op_nosys
#define skelfs_vop_rmdir \
	(int (*)(struct vnode *, const char *, struct vnode *, \
		 struct creds *)) \
			v_generic_op_nosys
#endif

struct vops skelfs_vops = {
	skelfs_vop_getattr,
	skelfs_vop_setattr,
	skelfs_vop_lookup,
	skelfs_vop_readlink,
	skelfs_vop_read,
	skelfs_vop_write,
	skelfs_vop_create,
	skelfs_vop_remove,
	skelfs_vop_rename,
	skelfs_vop_link,
	skelfs_vop_symlink,
	skelfs_vop_mkdir,
	skelfs_vop_rmdir,
	skelfs_vop_readdir,
	skelfs_vop_handle,
	skelfs_vop_lock,
	skelfs_vop_unlock,
	skelfs_vop_lkty,
	skelfs_vop_reclaim
};
#endif /* SKELFS */
