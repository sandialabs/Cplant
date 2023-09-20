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
#ifdef PROCFS
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "procfs.h"

/*
 * procfs_vops: VFS vnode operations for the process file system.
 *
 * Author: Scott Morgan, Abba Technologies (smorgan@sandia.gov)
 *
 */

IDENTIFY("");

/*
 * default procfs dtree node ops.
 */

int
procfs_ino_create(dtree_ino *inode IS_UNUSED, void **prv_data IS_UNUSED)
{
	/*
	 * since the procfs files are generally stateless (or their state
	 * is kept elsewhere), no actual creation operations are necessary.
	 */
	return 0;
}

int
procfs_ino_gattr(dtree_ino *inode, dtree_prvattr *attr)
{
	/* attributes are assumed */
	if (dtree_isdir(inode)) {
		attr->dta_size = (u_int64_t ) VMPAGESIZE;
		attr->dta_blocks = (u_int32_t ) 1;
	}
	else {
		/*
		 * NFS READs require the file to have some size to occur.
		 */
		attr->dta_size = (u_int64_t ) VMPAGESIZE;
		attr->dta_blocks = (u_int32_t ) 1;
	}

	attr->dta_blocksize = (u_int32_t ) 0;
	attr->dta_rdev = (z_dev_t ) 0;

	return 0;
}

int
procfs_ino_destroy(dtree_ino *inode IS_UNUSED)
{
	/*
	 * since the procfs files are generally stateless (or their state
	 * is kept elsewhere), no actual destroy operations are necessary.
	 */
	return 0;
}

dtree_ino_ops procfs_dirino_ops = {
	NULL,				/* no non-dir inode creation */
	NULL,				/* no inode read */
	NULL,				/* no inode write */
	procfs_ino_gattr,
	NULL,				/* no inode set attribute */
	NULL				/* no inode destroy */
};

int
procfs_findi(struct procfs *procfs, dtree_ino *ino, rwty_t lock,
	     struct vnode **vnode)
{
	int			ret = 0;
	struct vnode		*vp = NULL;
	extern struct vops	procfs_vops;

	/* lock out any other callers from instancing this vnode */
	do {
		ret = vfs_vfind(procfs->procfs_dev, ino->dti_inum, lock, 0,
				&vp);

		if (ret == ENOENT) {
			ret = 0;
			/* create the new vnode */
			vp = v_new(procfs->procfs_vfs, ino->dti_inum,
				       &procfs_vops, ino);

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

/*
 * procfs_vop_setattr: normally, a process file system would never allow
 * attributes to be set. however, since the VFS layer above this is
 * implemented via NFS, some operations that should be successful (eg,
 * open(,O_TRUNC,) for writing procfs files will become an NFS SETATTR
 * of size zero (0)) will require set attribute support.
 *
 * currently, the only attribute that can be set is the size (and only
 * to zero (0)).
 *
 * input parameters:
 *	vnode:		the vnode whose attributes will be set.
 *	attr:		the attributes to set.
 *	cr:		caller authorization crendentials.
 *
 * output parameters: none.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 *
 */
int
procfs_vop_setattr(struct vnode *vnode, struct vstat *attr, struct creds *cr)
{
	dtree_ino			*ino = V2PROC(vnode);

	/* insure the vnode is locked */
	BUG_CHECK(VOP_LKTY(vnode) == RWLK_WRITE);

	/* disallow setting of all attributes, save size zero (0) */
	if (!VST_VALID(attr)
	    || VST_DOESSET_MODE(attr)
	    || VST_DOESSET_UID(attr)
	    || VST_DOESSET_GID(attr)
	    || VST_DOESSET_ATIME_SEC(attr)
	    || VST_DOESSET_ATIME_NSEC(attr)
	    || VST_DOESSET_MTIME_SEC(attr)
	    || VST_DOESSET_MTIME_NSEC(attr))
		return EPERM;

	/* caller can only set the size if they have write authorization */
	if (dtree_isauth(NULL, ino, cr, DTREE_OP_WRITE) == FALSE)
		return EPERM;

	if (VST_DOESSET_SIZE(attr) && attr->vst_size != (u_int64_t) 0)
		return EPERM;

	/* setting the size to zero actually does nothing.... */
	return 0;
}

int
procfs_vop_getattr(struct vnode *vnode, struct vstat *attr)
{
	dtree_ino		*ino = V2PROC(vnode);
	int			err;
	struct timeval		now;

	err = dtree_getattr(ino, attr);
	if (!err) {
		err = gettimeofday(&now, NULL);
		if (err)
			LOG(LOG_ERR,
			    "procfs_vop_getattr: gettimeofday: %s",
			    strerror(errno));
	}
	if (!err && VS_ISREG(attr->vst_mode)) {
		/*
		 * NFS clients that cache data sometimes try to
		 * decide whether the data is valid or not by looking
		 * at the attributes. Always report that the ctime, mtime
		 * and atime are *now* to minimize tiem spent in the
		 * client's cache.
		 */
		attr->vst_atime.tv_sec =
		    attr->vst_mtime.tv_sec =
		    attr->vst_ctime.tv_sec = now.tv_sec;
		attr->vst_atime.tv_nsec =
		    attr->vst_mtime.tv_nsec =
		    attr->vst_ctime.tv_nsec = now.tv_sec * 1000;
	}

	return err;
}

int
procfs_vop_lookup(struct vnode *dvnode, const char *name, struct creds *cr,
		  rwty_t lock, struct vnode **vnode)
{
	int			ret;
	dtree_ino		*dino = V2PROC(dvnode);
	dtree_ino		*ino;
	struct procfs		*procfs = V2PROCFS(dvnode);

	/* search for the vnode in the vnode cache. */
	ret = dtree_lookup(dino, (char *) name, cr, &ino);

	/* get a corresponding vnode */
	if (ret == 0)
		ret = procfs_findi(procfs, ino, lock, vnode);

	/*
	 * NOTE: IMPORTANT IMPLEMENTATION REQUIREMENT!
	 * 
	 * unlock the parent directory: since the upper VFS/NFS service
	 * layer may need to reacquire a lock on this directory for a
	 * subsequent operation, it is incumbent upon the file system
	 * layer to drop this lock when no longer necessary.
	 *
	 * we're dropping the lock after the vnode instantiation call
	 * (procfs_findi()) to prevent races with other callers from
	 * instantiating the vnode first.
	 */
	VOP_UNLOCK(dvnode);


	return ret;
}

int
procfs_vop_read(struct vnode *vnode, z_off_t offset, char *buf,
		 u_int32_t *buflen, struct creds *cr)
{
	int			mylock = 0;
	int			ret = 0;
	dtree_ino		*ino = V2PROC(vnode);

	/*
	 * since the upper VFS layer (ie, the ENFS service) does *no*
	 * locking on the vnode prior to I/O calls, the lock must be
	 * acquired here. failure to do so could potentially allow
	 * another thread to remove the vnode during the i/o call.
	 */
	if (VOP_LKTY(vnode) == RWLK_NONE) {
		ret = VOP_LOCK(vnode, RWLK_READ, 0);
		mylock++;
	}

	if (!ret) {
		ret = dtree_read(ino, offset, buf, buflen, cr);

		if (mylock)
			VOP_UNLOCK(vnode);
	}

	return ret;
}

int
procfs_vop_write(struct vnode *vnode, z_off_t offset, char *buf,
		 u_int32_t *buflen, struct creds *cr)
{
	int			mylock = 0;
	int			ret = 0;
	dtree_ino		*ino = V2PROC(vnode);

	/*
	 * since the upper VFS layer (ie, the ENFS service) does *no*
	 * locking on the vnode prior to I/O calls, the lock must be
	 * acquired here. failure to do so could potentially allow
	 * another thread to remove the vnode during the i/o call.
	 */
	if (VOP_LKTY(vnode) == RWLK_NONE) {
		ret = VOP_LOCK(vnode, RWLK_READ, 0);
		mylock++;
	}

	if (!ret) {
		ret = dtree_write(ino, offset, buf, buflen, cr);

		if (mylock)
			VOP_UNLOCK(vnode);
	}

	return ret;
}

int
procfs_vop_readlink(struct vnode *vnode, char *buf, u_int32_t *len)
{
	dtree_ino		*ino = V2PROC(vnode);

	return dtree_readlink(ino, buf, len);
}

int
procfs_vop_readdir(struct vnode *dvnode, z_off_t *off, char *buf,
		   u_int32_t *buflen, int *eod, struct creds *cr)
{
	dtree_ino		*dino = V2PROC(dvnode);

	return dtree_readdir(dino, off, buf, buflen, eod, cr);
}

int
procfs_vop_handle(struct vnode *vnode, struct vnhndl *handle)
{
	int			ret = 0;
	dtree_ino		*ino = V2PROC(vnode);

	/* the only handle required is the inode/dev number itself. */

	if (ino->dti_fh == NULL) {
		/* create the file handle */
		if ((ino->dti_fh = malloc(PROCFS_FHSIZE)) == NULL)
			ret = ENOMEM;
		else {
			(void) memset(ino->dti_fh, 0, PROCFS_FHSIZE);
			(void) memcpy(ino->dti_fh, &ino->dti_dev,
				      sizeof(z_dev_t));
			(void) memcpy((char *)ino->dti_fh + sizeof(z_dev_t),
				      &ino->dti_inum, sizeof(z_ino_t));
		}
	}

	if (!ret) {
		handle->vnhnd_data = (const char *) ino->dti_fh;
		handle->vnhnd_len = PROCFS_FHSIZE;
	}

	return ret;
}

int
procfs_vop_lock(struct vnode *vp, rwty_t lock, int dontwait)
{
	dtree_ino		*ino = V2PROC(vp);

	return dtree_ino_lock(ino, lock, dontwait);
}

void
procfs_vop_unlock(struct vnode *vp)
{
	dtree_ino		*ino = V2PROC(vp);

	return dtree_ino_unlock(ino);
}

rwty_t
procfs_vop_lkty(struct vnode *vp)
{
	dtree_ino		*ino = V2PROC(vp);

	return dtree_ino_lkty(ino);
}

void
procfs_vop_reclaim(struct vnode *vnode)
{
	dtree_ino		*ino = V2PROC(vnode);

	/*
	 * since the directory tree inode now holds the vnode lock,
	 * the inode resources cannot be released until the vnode
	 * is reclaimed.
	 */

	dtree_ino_destroy(ino);

	return;
}

/*
 * procfs_vop_noperm: return an EPERM for disallowed process file system
 * operations.
 */
static int
procfs_vop_noperm (void)
{
	return EPERM;
}

/* operations not supported by the process file system */
#define procfs_vop_create \
	(int (*)(struct vnode *, const char *, struct vstat *, struct creds *, \
		 rwty_t, struct vnode **)) \
			procfs_vop_noperm
#define procfs_vop_remove \
	(int (*)(struct vnode *, const char *, struct vnode *, \
		 struct creds *)) \
			procfs_vop_noperm
#define procfs_vop_rename \
	(int (*)(struct vnode *, const char *, struct vnode *, struct vnode *, \
		 const char *, struct creds *)) \
			procfs_vop_noperm
#define procfs_vop_link \
	(int (*)(struct vnode *, struct vnode *, const char *, \
		 struct creds *)) \
			procfs_vop_noperm
#define procfs_vop_symlink \
	(int (*)(struct vnode *, const char *, const char *, struct vstat *, \
		 struct creds *)) \
			procfs_vop_noperm
#define procfs_vop_mkdir \
	(int (*)(struct vnode *, const char *, struct vstat *, struct creds *, \
		 rwty_t, struct vnode **)) \
			procfs_vop_noperm
#define procfs_vop_rmdir \
	(int (*)(struct vnode *, const char *, struct vnode *, \
		 struct creds *)) \
			procfs_vop_noperm
#define procfs_vop_strategy \
	(int (*)(struct vnode *, struct buffer *)) \
			v_generic_op_illop

struct vops procfs_vops = {
	procfs_vop_getattr,
	procfs_vop_setattr,
	procfs_vop_lookup,
	procfs_vop_readlink,
	procfs_vop_read,
	procfs_vop_write,
	procfs_vop_strategy,
	procfs_vop_create,
	procfs_vop_remove,
	procfs_vop_rename,
	procfs_vop_link,
	procfs_vop_symlink,
	procfs_vop_mkdir,
	procfs_vop_rmdir,
	procfs_vop_readdir,
	procfs_vop_handle,
	procfs_vop_lock,
	procfs_vop_unlock,
	procfs_vop_lkty,
	procfs_vop_reclaim
};
#endif /* PROCFS */
