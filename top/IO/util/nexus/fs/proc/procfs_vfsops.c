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
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

#include "procfs.h"

/*
 * "Process" file system.
 *
 * Author: Scott Morgan, Abba Technologies (smorgan@sandia.gov)
 *
 */

IDENTIFY("$Id: procfs_vfsops.c,v 1.1 2000/06/22 17:14:00 smorgan Exp $");

/*
 * Procfs global resources.
 */

static mutex_t			_procfs_lock = MUTEX_INITIALIZER;
static struct procfs		*_procfs = NULL;

/*
 * procfs_startsvc: initialize process file system global resources.
 *
 * input parameters:
 *	args:		arguments from start invocation.
 *	arglen:		length of arguments.
 *	cr:		authorization credentials.
 *
 * input parameters: currently, all input parameters are unused.
 *
 * output parameters: none.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */

int
procfs_startsvc(void *args IS_UNUSED, size_t argslen IS_UNUSED,
		struct creds *cr)
{
	int		ret;

	if (!is_suser(cr))
		return EPERM;

	mutex_lock(&_procfs_lock);

	/* insure a single initialization (RPC retries) */
	if (_procfs != NULL) {
		ret = EBUSY;
	}

	if ((_procfs = malloc(sizeof(struct procfs))) == NULL)
		ret = ENOMEM;
	else {
		mutex_init(&_procfs->procfs_lock);
		_procfs->procfs_dev = PROCFS_DEV;
		ret = dtree_create_root(PROCFS_DEV, &procfs_dirino_ops,
					&_procfs->procfs_root);
	}

	mutex_unlock(&_procfs_lock);

	if (ret && _procfs != NULL) {
		mutex_destroy(&_procfs->procfs_lock);
		dtree_destroy(_procfs->procfs_root);
		free(_procfs);
		_procfs = NULL;
	}

	return ret;
}

/*
 * procfs_mount: mount the process file system.
 *
 * input parameters:
 *	args:		mount args (the configuration file path).
 *	argslen:	length of args.
 *	cr:		credentials of mounter.
 *
 * output parameters:
 *	vfs:		the newly created VFS instance for the process
 *			file system.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */
int
procfs_mount(const char *args IS_UNUSED, size_t argslen IS_UNUSED,
	     struct creds *cr, struct vfs **vfs)
{
	int			ret = 0;
	extern struct vfsops	procfs_vfsops;

	/* only root can mount */
	if (!is_suser(cr))
		return EPERM;

	mutex_lock(&_procfs_lock);
	if (_procfs == NULL)
		ret = ENODEV; /* file system type not configured */

	if (!ret) {
		_procfs->procfs_vfs = vfs_new(_procfs->procfs_dev,
					      &procfs_vfsops,
					      _procfs);
		if (_procfs->procfs_vfs == NULL)
			ret = ENOMEM;
	}

	if (!ret)
		*vfs = _procfs->procfs_vfs;

	mutex_unlock(&_procfs_lock);

	return ret;
}

/*
 * procfs_vfsop_unmount: unmount a previously mounted process file system.
 *
 * input parameters:
 *	vfs:		the VFS instance associated with the mounted
 *			process file system.
 *
 * output parameters: none.
 *
 * return value: none.
 */
static void
procfs_vfsop_unmount(struct vfs *vfs IS_UNUSED)
{
	/* unmount is a no-op in the procfs-specific space */
	return;
}

/*
 * procfs_vfsop_getroot: return the root vnode associated with the passed
 * process file system instance.
 *
 * input parameters:
 *	vfs:		the VFS instance associated with the process file
 *			system.
 *	lock:		the type of lock desired on the returned root vnode.
 *
 * output parameters:
 *	vnode:		the root vnode of the process file system.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */
static int
procfs_vfsop_getroot(struct vfs *vfs, rwty_t lock, struct vnode **vnode)
{
	int			ret = 0;
	dtree_ino		*ino;
	struct procfs		*procfs = VFS2PROCFS(vfs);

	ino = procfs->procfs_root->dt_root;

	/* get a corresponding vnode */
	ret = procfs_findi(procfs, ino, lock, vnode);

	return ret;
}

/*
 * procfs_vfsop_statfs: return file system status (see struct fsstats for
 * details).
 *
 * input parameters:
 *	vfs:		VFS associated with the mounted process file
 *			system.
 *
 * output parameters:
 *	fsattr:		returned file system status.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */
static int
procfs_vfsop_statfs(struct vfs *vfs, struct fsstats *fsattr)
{
	struct procfs		*procfs = VFS2PROCFS(vfs);

	return dtree_statfs(procfs->procfs_root, fsattr);
}

/*
 * procfs_vfsop_getv: return the process file system vnode identified by
 * the passed inode number.
 *
 * input parameters:
 *	vfs:		VFS associated with the mounted process file
 *			system.
 *	inum:		inode number of the desired vnode.
 *	lock:		type of lock desired on the returned vnode.
 *
 * output parameters:
 *	vnode:		the returned vnode.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */
static int
procfs_vfsop_getv(struct vfs *vfs, z_ino_t inum, struct vnode **vnode,
		  unsigned lock)
{
	int			ret = 0;
	dtree_ino		*ino;
	struct procfs		*procfs = VFS2PROCFS(vfs);

	if ((ret = dtree_findi(procfs->procfs_root, inum, (rwty_t) lock,
			       &ino)) != 0)
		return ret;

	/* get a corresponding vnode */
	ret = procfs_findi(procfs, ino, (rwty_t ) lock, vnode);

	return ret;
}

/*
 * procfs_vfsop_getv_by_handle: return the process file system vnode
 * identified by the passed handle.
 *
 * input parameters:
 *	vfs:		VFS associated with the mounted process file
 *			system.
 *	handle:		file handle of the desired vnode.
 *	lock:		type of lock desired on the returned vnode.
 *
 * output parameters:
 *	vnode:		the returned vnode.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */
static int
procfs_vfsop_getv_by_handle(struct vfs *vfs, struct vnhndl *handle,
			    struct vnode **vnode, rwty_t lock)
{
	z_ino_t			inum;

	if (handle->vnhnd_len > PROCFS_FHSIZE)
		return EINVAL;

	/* the process file system handle is simply the dev/inode number */
	(void ) memcpy(&inum, &handle->vnhnd_data[sizeof(z_dev_t)],
		       sizeof(z_ino_t));

	return procfs_vfsop_getv(vfs, inum, vnode, lock);
}

/*
 * _procfs_register: register a new procfs path. all registered pathnames must
 * be absolute (to the procfs root). _procfs_register() will register any type
 * of process file system entry (file, directory or link), with the appropriate
 * entry type arg. the macros procfs_register(), procfs_register_dir() and
 * procfs_register_symlink() are provided for ease of use (see procfs.h)
 *
 * input parameters:
 *	name: the path name to create.
 *	ops: the inode operations associated with this path.
 *	attr: attributes of the new procfs entry.
 *	type: type of process file system entry to create.
 *	arg: procfs path-entry specific argument.
 *
 * output parameters: none.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */

/* default directory attributes */
const struct vstat		procfs_defattr_dir = {
	VS_IFDIR|VS_IRUSR|VS_IXUSR|
	    ((VS_IRUSR|VS_IXUSR) >> 3)|((VS_IRUSR|VS_IXUSR) >> 6),
	(z_nlink_t ) -1, SUSER_UID, SUSER_UID, (u_int64_t ) -1, (u_int32_t ) -1,
	(z_dev_t ) -1, (u_int32_t ) -1, (z_dev_t ) -1, (z_ino_t ) -1,
	{ (long ) -1, (long ) -1 }, { (long ) -1, (long ) -1 },
	{ (long ) -1, (long ) -1 }
};

/* default file attributes */
const struct vstat		procfs_defattr_file = {
	VS_IFREG|VS_IRUSR|VS_IWUSR|(VS_IRUSR >> 3)|(VS_IRUSR >> 6),
	(z_nlink_t ) -1, SUSER_UID, SUSER_UID, (u_int64_t ) -1, (u_int32_t ) -1,
	(z_dev_t ) -1, (u_int32_t ) -1, (z_dev_t ) -1, (z_ino_t ) -1,
	{ (long ) -1, (long ) -1 }, { (long ) -1, (long ) -1 },
	{ (long ) -1, (long ) -1 }
};

/* default link attributes */
const struct vstat		procfs_defattr_link = {
	VS_IFLNK|VS_IRWXU|(VS_IRWXU >> 3)|(VS_IRWXU >> 6),
	(z_nlink_t ) -1, SUSER_UID, SUSER_UID, (u_int64_t ) -1, (u_int32_t ) -1,
	(z_dev_t ) -1, (u_int32_t ) -1, (z_dev_t ) -1, (z_ino_t ) -1,
	{ (long ) -1, (long ) -1 }, { (long ) -1, (long ) -1 },
	{ (long ) -1, (long ) -1 }
};

int
_procfs_register(const char *name, procfs_ino_ops *ops,
		 const struct vstat *attr, z_mode_t type, void *arg)
{
	char			*path_elem;
	char			*p;
	char			lname[Z_MAXPATHLEN+1];
	int			last_path_elem = 0;
	int			ret = 0;
	dtree_ino		*dino;
	dtree_ino		*ino;
	extern struct creds	suser_creds;

	/* sanity check */
	if (name == NULL || attr == NULL)
		return EINVAL;

	if (strlen(name) > Z_MAXPATHLEN)
		return ENAMETOOLONG;

	if (!(type == VS_IFREG || type == VS_IFLNK || type == VS_IFDIR))
		return EINVAL;

	/* get the first directory component */
	if (*name == PROCFS_DELIM)
		name++;
	else
		return EINVAL;		/* pathnames must be absolute */

	(void ) strcpy(lname, name);

	path_elem = lname;
	mutex_lock(&_procfs->procfs_lock);
	dino = _procfs->procfs_root->dt_root;

	/* write lock the root directory for mkdir/create */
	ret = dtree_ino_lock(dino, RWLK_WRITE, 0);

	mutex_unlock(&_procfs->procfs_lock);
	if (ret)
		return ret;

	while (path_elem != NULL) {
		/* skip "//" */
		if (*path_elem == PROCFS_DELIM) {
			path_elem++;
			continue;
		}

		/* terminate path component (locating next component) */
		if ((p = strchr(path_elem, PROCFS_DELIM)) != NULL) {
			*p++ = '\0';
		}
		else
			last_path_elem++;

		if (last_path_elem) {
			switch (type) {
			case VS_IFDIR:
				ret = dtree_mkdir(dino, path_elem,
						  (struct vstat *) attr,
						  &suser_creds, ops, &ino);
				ino = NULL;
				break;

			case VS_IFREG:
				ret = dtree_create(dino, path_elem,
						   (struct vstat *) attr,
						   &suser_creds, ops, &ino);
				ino = NULL;
				break;

			case VS_IFLNK:
				ret = dtree_symlink(dino, path_elem,
						    (char *) arg,
						    (struct vstat *) attr,
						    &suser_creds);
				ino = NULL;
				break;
			}
		}
		else {
			ret = dtree_mkdir(dino, path_elem,
					  (struct vstat *) &procfs_defattr_dir,
					  &suser_creds, &procfs_dirino_ops,
					  &ino);
			if (ret != 0) {
				if (ret == EEXIST) {
					/* locate the existing directory */
					if ((ret = dtree_lookup(dino, path_elem,
								&suser_creds,
								&ino)) != 0)
						break;
				}
				else
					break;
			}
		}

		/* lock the next path component before releasing the parent */
		if (ino != NULL &&
		    (ret = dtree_ino_lock(ino, RWLK_WRITE, 0)) != 0)
			break;

		dtree_ino_unlock(dino);

		dino = ino;
		path_elem = p;
	}

	if (dino != NULL)
		dtree_ino_unlock(dino);

	return ret;
}

/* use generic VFS locking */
#define procfs_vfsop_lock		vfs_generic_op_lock
#define procfs_vfsop_unlock		vfs_generic_op_unlock
#define procfs_vfsop_lkty		vfs_generic_op_lkty

static struct vfsops procfs_vfsops = {
	procfs_vfsop_unmount,
	procfs_vfsop_getroot,
	procfs_vfsop_statfs,
	procfs_vfsop_getv,
	procfs_vfsop_getv_by_handle,
	procfs_vfsop_lock,
	procfs_vfsop_unlock,
	procfs_vfsop_lkty
};
#endif /* PROCFS */
