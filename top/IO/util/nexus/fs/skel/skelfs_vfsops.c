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
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include "skelfsmnt.h"
#include "skelfs.h"

/*
 * Skeletal client side file system.
 *
 * Author: Scott Morgan, Abba Technologies (smorgan@sandia.gov)
 *
 */

IDENTIFY("$Id: skelfs_vfsops.c,v 1.5 2001/07/18 18:57:39 rklundt Exp $");

/*
 * the internal instance of the skeletal file system is created at service
 * start. the internal skeletal file system allows peer file systems (or
 * the VFS kernel layer) to create their own directories within the
 * internal skeletal file system prior to mounting said file system.
 */
struct skelfs		*_skelfs_intern = NULL;	/* internal name space */
static int		_skelfs_instance = 0;
static mutex_t		_skelfs_instlk = MUTEX_INITIALIZER;

/*
 * skelfs_parseconf: parse the skeletal file system configuration file.
 * skelfs_parseconf() can be called either from the service start routine
 * (skelfs_startsvc()) or the mount routine (skelfs_mount()).
 *
 * currently, the only configuration consists of a list of predefined
 * directories to create in the mounted/internal skeletal file system,
 * one per line. optionally, each line can also include the user, group,
 * and mode for each directory (all delimited by SKELFSCONF_DELIMs).
 *
 * skelfs_parseconf() has a helper function skelfs_parsedirent() (below)
 * that parses an individual directory entry.
 *
 * input args:
 *	skelfs:		the skeletal file system instance to configure.
 *	conf:		the configuration file to parse.
 *
 * output parameters: none.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */

#define SKELFSCONF_DELIM	':'

/* default directory permissions: only mode, user and group are set */
static const struct vstat	skelfs_defattr = {
	VS_IFDIR|VS_IRWXU|((VS_IRUSR|VS_IXUSR) >> 3)|((VS_IRUSR|VS_IXUSR) >> 6),
	(z_nlink_t ) -1, SUSER_UID, SUSER_UID, (u_int64_t ) -1, (u_int32_t ) -1,
	(z_dev_t ) -1, (u_int32_t ) -1, (z_dev_t ) -1, (z_ino_t ) -1,
	{ (long ) -1, (long ) -1 }, { (long ) -1, (long ) -1 },
	{ (long ) -1, (long ) -1 }
};

static void
skelfs_parsedirent(char *dir, struct vstat *attr)
{
	char		*group;
	char		*mode;
	char		*user;
	char		*p;
	char		buf[NSS_BUFLEN_PASSWD];
	struct passwd	pwd, *pwdp;
	struct group	grp, *grpp;

	/* strip off newline */
	if (dir[strlen(dir)-1] == '\n')
		dir[strlen(dir)-1] = '\0';

	/* search for user */
	if ((p = strchr(dir, SKELFSCONF_DELIM)) != NULL) {
		*p = '\0';
		user = ++p;

		if ((p = strchr(p, SKELFSCONF_DELIM)) != NULL)
			*p = '\0';

		if (*user != '\0') {		/* non-empty user field */
			if (isdigit(*user))
				attr->vst_uid =
				    (z_uid_t ) strtol(user, NULL, 0);
			else {
				if (getpwnam_r(user, &pwd, buf,
					       NSS_BUFLEN_PASSWD, &pwdp)
						!= 0) {
					LOG(LOG_WARNING,
		"skelfs_parseconf: unknown user %s for dir %s. using default",
					    user, dir);
				}
				else
					attr->vst_uid = (z_uid_t ) pwdp->pw_uid;
			}
		}
	}

	/* search for group */
	if ((group = p) != NULL) {
		group = ++p;

		if ((p = strchr(p, SKELFSCONF_DELIM)) != NULL)
			*p = '\0';

		if (*group != '\0') {		/* non-empty group field */
			if (isdigit(*group))
				attr->vst_gid =
				    (z_gid_t ) strtol(group, NULL, 0);
			else {
				if (getgrnam_r(group, &grp, buf,
					       NSS_BUFLEN_PASSWD, &grpp)
						!= 0) {
					LOG(LOG_WARNING,
		"skelfs_parseconf: unknown group %s for dir %s. using default",
					    group, dir);
				}
				else
					attr->vst_gid = (z_gid_t ) grpp->gr_gid;
			}
		}
	}

	/* search for mode */
	if ((mode = p) != NULL) {
		mode = ++p;

		if ((p = strchr(p, SKELFSCONF_DELIM)) != NULL)
			*p = '\0';	/* trailing delimiter */

		if (*mode != '\0') {		/* non-empty mode field */
			if (!isdigit(*mode))
					LOG(LOG_WARNING,
		"skelfs_parseconf: malformed mode %s for dir %s. using default",
					    mode, dir);
			else
				attr->vst_mode = VS_IFDIR | (~VS_IFMT &
					(z_mode_t ) strtol(mode, NULL, 0));
		}
	}

	return;
}

static int
skelfs_parseconf(struct skelfs *skelfs, char *conf)
{
	char		dir[Z_MAXPATHLEN+NSS_BUFLEN_PASSWD];
	int		error;
	int		ret = 0;
	FILE		*fp;
	struct vstat	attr;

	if (conf[0] == '\0')
		return 0;	/* no configuration file specified */

	if ((fp = fopen(conf, "r")) == NULL) {
		error = errno;
		LOG(LOG_ERR, "skelfs_parseconf: cannot open config file %s: %s",
		    conf, strerror(error));

		return error;
	}

	while (fgets(dir, Z_MAXPATHLEN, fp) != NULL) {
		if (dir[0] != '/')
			continue;

		(void ) memcpy(&attr, &skelfs_defattr, sizeof(struct vstat));
		skelfs_parsedirent(dir, &attr);

		if ((ret = skelfs_register(skelfs, dir, &attr) != 0)) {
			LOG(LOG_ERR,
			    "skelfs_parseconf: cannot create directory %s: %s",
			    dir, strerror(errno));
			break;
		}
	}

	return ret;
}

/*
 * skelfs_parseargs: parse mount/startsvc arguments.
 *
 * input parameters:
 *	args:		arguments to parse.
 *	argslen:	length of arguments.
 *
 * output parameters:
 *	conf:		mount configuration.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */
int
skelfs_parseargs(const char *args, size_t argslen, skelfsmnt *conf)
{
	XDR		xdrs;

	/* parse service start args */
	(void ) memset(&xdrs, 0, sizeof(xdrs));
	(void ) memset(conf, 0, sizeof(skelfsmnt));
	xdrmem_create(&xdrs, args, argslen, XDR_DECODE);
	if (!xdr_skelfsmnt(&xdrs, conf)) {
		LOG(LOG_ERR, "skelfs_parseconf: can't decode FS specific args");
		xdr_free(xdr_skelfsmnt, conf);
		return EINVAL;
	}

	return 0;
}

/*
 * skelfs_startsvc: initialize skeletal file system global resources.
 *
 * input parameters:
 *	args:		arguments from start invocation.
 *	arglen:		length of arguments.
 *	cr:		authorization credentials.
 *
 * skelfs_startsvc() takes the same argument list as skelfs_mount().
 *
 * output parameters: none.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */
int
skelfs_startsvc(void *args, size_t argslen, struct creds *cr)
{
	int		ret;
	skelfsmnt	conf;

	if (!is_suser(cr))
		return EPERM;

	if ((ret = skelfs_parseargs(args, argslen, &conf)) != 0)
		return ret;

	mutex_lock(&_skelfs_instlk);

	/* insure a single initialization (RPC retries) */
	if (_skelfs_intern != NULL) {
		xdr_free(xdr_skelfsmnt, &conf);
		mutex_unlock(&_skelfs_instlk);
		return EBUSY;
	}

	/* create the internal name space skelfs */
	if ((_skelfs_intern = m_alloc(sizeof(struct skelfs))) == NULL) {
		xdr_free(xdr_skelfsmnt, &conf);
		mutex_unlock(&_skelfs_instlk);
		return ENOMEM;
	}
	(void ) memset(_skelfs_intern, 0, sizeof(struct skelfs));

	mutex_init(&_skelfs_intern->sfs_instlk);

	_skelfs_intern->sfs_dev = ++_skelfs_instance | SKELFS_DEVMASK;

	ret = dtree_create_root(_skelfs_intern->sfs_dev, &skelfs_ino_ops,
				&_skelfs_intern->sfs_root);

	/* parse the passed service start args: ie, parse the config file */
	if (!ret && (ret = skelfs_parseconf(_skelfs_intern, conf.cpath)) != 0)
		LOG(LOG_ERR,
		    "skelfs_startsvc: error parsing config file. giving up");

	if (ret) {
		_skelfs_instance--;

		dtree_destroy(_skelfs_intern->sfs_root);
		mutex_destroy(&_skelfs_intern->sfs_instlk);
		free(_skelfs_intern);
		_skelfs_intern = NULL;
	}

	mutex_unlock(&_skelfs_instlk);

	xdr_free(xdr_skelfsmnt, &conf);

	return ret;
}

/*
 * skelfs_mount: mount a skeletal file system. skelfs_mount() first attempts
 * to mount the internal skeletal file system (if instantiated by
 * skelfs_startsvc()). otherwise a new skeletal file system is created and
 * mounted.
 *
 * skelfs_mount(), like skelfs_startsvc(), takes a single configuration file
 * path argument. this configuration file allows the mounter to create
 * predefined directory paths listed therein (see skelfs_parseconf()).
 *
 * input parameters:
 *	args:		mount args (the configuration file path).
 *	argslen:	length of args.
 *	cr:		credentials of mounter.
 *
 * output parameters:
 *	vfs:		the newly created VFS instance for this mounted
 *			skeletal file system.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */
int
skelfs_mount(const char *args, size_t argslen, struct creds *cr,
	     struct vfs **vfs)
{
	int			ret = 0;
	struct skelfs		*skelfs = NULL;	/* file system instance */
	skelfsmnt		conf;
	extern struct vfsops	skelfs_vfsops;

	/* only root can mount */
	if (!is_suser(cr))
		return EPERM;

	/* parse mount args */
	if ((ret = skelfs_parseargs(args, argslen, &conf)) != 0)
		return ret;

	/* mount the internal skeletal file system, as appropriate */
	mutex_lock(&_skelfs_instlk);
	if (conf.mount_internal) {
		skelfs = _skelfs_intern;

		if (_skelfs_intern != NULL && _skelfs_intern->sfs_vfs != NULL) {
			/* only one skelfs <=> vfs mapping is supported */
			xdr_free(xdr_skelfsmnt, &conf);
			mutex_unlock(&_skelfs_instlk);
			return EBUSY;
		}
	}

	/*
	 * the internal skeletal file system can be created at initial
	 * mount as well as by the service start routine.
	 */
	if (skelfs == NULL) {
		if ((skelfs = m_alloc(sizeof(struct skelfs))) == NULL) {
			xdr_free(xdr_skelfsmnt, &conf);
			mutex_unlock(&_skelfs_instlk);
			return ENOMEM;
		}

		(void ) memset(skelfs, 0, sizeof(struct skelfs));

		skelfs->sfs_dev = ++_skelfs_instance;
		mutex_init(&skelfs->sfs_instlk);


		/*
		 * check for wrap. there should be a better way to handle this,
		 * perhaps with an allocation map of skeletal file systems.
		 */
		if ((_skelfs_instance & (~SKELFS_DEVMASK)) == 0) {
			ret = ENOSPC;
			mutex_destroy(&skelfs->sfs_instlk);
		}
		else
			ret = dtree_create_root(skelfs->sfs_dev,
						&skelfs_ino_ops,
						&skelfs->sfs_root);

		if (ret)
			_skelfs_instance--;
	}

	/* parse the passed service start args: ie, parse the config file */
	if (!ret && (ret = skelfs_parseconf(skelfs, conf.cpath)) != 0)
		LOG(LOG_ERR,
		    "skelfs_mount: error parsing config file. giving up");

	mutex_unlock(&_skelfs_instlk);

	if (!ret) {
		skelfs->sfs_vfs = vfs_new(skelfs->sfs_dev, &skelfs_vfsops,
					  skelfs);
		if (skelfs->sfs_vfs == NULL)
			ret = ENOMEM;
	}

	if (ret) {
		if (skelfs != _skelfs_intern) {
			dtree_destroy(skelfs->sfs_root);
			mutex_destroy(&skelfs->sfs_instlk);
			free(skelfs);
		}
	}
	else
		*vfs = skelfs->sfs_vfs;

	xdr_free(xdr_skelfsmnt, &conf);

	return ret;
}

/*
 * skelfs_vfsop_unmount: unmount a previously mounted skeletal file system.
 *
 * input parameters:
 *	vfs:		the VFS instance associated with the mounted
 *			skeletal file system.
 *
 * output parameters: none.
 *
 * return value: none.
 */
static void
skelfs_vfsop_unmount(struct vfs *vfs)
{
	struct skelfs		*skelfs = VFS2SKELFS(vfs);

	/* don't destroy the internal namespace on unmount */
	mutex_lock(&_skelfs_instlk);
	if (skelfs != _skelfs_intern) {
		mutex_lock(&skelfs->sfs_instlk);
		dtree_destroy(skelfs->sfs_root);

		mutex_unlock(&skelfs->sfs_instlk);
		mutex_destroy(&skelfs->sfs_instlk);
		free(skelfs);
	}
	mutex_unlock(&_skelfs_instlk);

	return;
}

/*
 * skelfs_vfsop_getroot: return the root vnode associated with the passed
 * skeletal file system instance.
 *
 * input parameters:
 *	vfs:		the VFS instance associated with the skeletal file
 *			system.
 *	lock:		the type of lock desired on the returned root vnode.
 *
 * output parameters:
 *	vnode:		the root vnode of the skeletal file system.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */
static int
skelfs_vfsop_getroot(struct vfs *vfs, rwty_t lock, struct vnode **vnode)
{
	int			ret = 0;
	dtree_ino		*ino;
	struct skelfs		*skelfs = VFS2SKELFS(vfs);

	ino = skelfs->sfs_root->dt_root;

	/* get a corresponding vnode */
	ret = skelfs_findi(skelfs, ino, lock, vnode);

	return ret;
}

/*
 * skelfs_vfsop_statfs:
 *
 * input parameters:
 *
 * output parameters:
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */
static int
skelfs_vfsop_statfs(struct vfs *vfs, struct fsstats *fsattr)
{
	struct skelfs		*skelfs = VFS2SKELFS(vfs);

	return dtree_statfs(skelfs->sfs_root, fsattr);
}

/*
 * skelfs_vfsop_getv:
 *
 * input parameters:
 *
 * output parameters:
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */
static int
skelfs_vfsop_getv(struct vfs *vfs, z_ino_t inum, struct vnode **vnode,
		  unsigned lock)
{
	int			ret = 0;
	dtree_ino		*ino;
	struct skelfs		*skelfs = VFS2SKELFS(vfs);

	if ((ret = dtree_findi(skelfs->sfs_root, inum, (rwty_t) lock,
			       &ino)) != 0)
		return ret;

	/* get a corresponding vnode */
	ret = skelfs_findi(skelfs, ino, (rwty_t ) lock, vnode);

	return ret;
}

/*
 * skelfs_vfsop_getv_by_handle:
 *
 * input parameters:
 *
 * output parameters:
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */
static int
skelfs_vfsop_getv_by_handle(struct vfs *vfs, struct vnhndl *handle,
			    struct vnode **vnode, rwty_t lock)
{
	z_ino_t			inum;

	if (handle->vnhnd_len > SKELFS_FHSIZE)
		return EINVAL;

	/* the skeletal file system handle is simply the dev/inode number */
	(void ) memcpy(&inum, &handle->vnhnd_data[sizeof(z_dev_t)],
		       sizeof(z_ino_t));

	return skelfs_vfsop_getv(vfs, inum, vnode, lock);
}

/*
 * skelfs_register: create a directory (or path) in the internal skeletal
 * file system. all registered pathnames must be absolute (to the passed
 * skelfs root).
 *
 * input parameters:
 *	skelfs: the skeletal file system to create a directory in.
 *	name: the directory/path name to create.
 *	attr: the directory attributes to create the directory with.
 *
 * output parameters: none.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */
int
skelfs_register(struct skelfs *skelfs, const char *name, struct vstat *attr)
{
	char			*dir;
	char			*p;
	char			lname[Z_MAXPATHLEN+1];
	int			ret = 0;
	dtree_ino		*dino;
	dtree_ino		*ino;
	extern struct creds	suser_creds;

	/* sanity check */
	if (name == NULL || attr == NULL)
		return EINVAL;

	if (strlen(name) > Z_MAXPATHLEN)
		return ENAMETOOLONG;

	/* get the first directory component */
	if (*name == SKELFS_DELIM)
		name++;
	else
		return EINVAL;		/* pathnames must be absolute */

	(void ) strcpy(lname, name);

	dir = lname;
	dino = skelfs->sfs_root->dt_root;

	/* write lock the root directory for mkdir */
	if ((ret = dtree_ino_lock(dino, RWLK_WRITE, 0)) != 0)
		return ret;

	/* attempt to create each element directory in the path */
	while (dir != NULL) {
		/* skip "//" */
		if (*dir == SKELFS_DELIM) {
			dir++;
			continue;
		}

		/* terminate path component (locating next component) */
		if ((p = strchr(dir, SKELFS_DELIM)) != NULL) {
			*p++ = '\0';
		}

		if ((ret = dtree_mkdir(dino, dir, (dtree_attr *) attr,
				       &suser_creds, &skelfs_ino_ops, &ino))
				!= 0) {
			if (ret == EEXIST) {
				/* locate the existing directory */
				if ((ret = dtree_lookup(dino, dir, &suser_creds,
							&ino)) != 0)
					break;
			}
			else
				break;
		}

		/* lock the next path component before releasing the parent */
		if ((ret = dtree_ino_lock(ino, RWLK_WRITE, 0)) != 0)
			break;

		dtree_ino_unlock(dino);

		dino = ino;
		dir = p;
	}

	dtree_ino_unlock(dino);

	return ret;
}

/* use generic VFS locking */
#define skelfs_vfsop_lock		vfs_generic_op_lock
#define skelfs_vfsop_unlock		vfs_generic_op_unlock
#define skelfs_vfsop_lkty		vfs_generic_op_lkty

static struct vfsops skelfs_vfsops = {
	skelfs_vfsop_unmount,
	skelfs_vfsop_getroot,
	skelfs_vfsop_statfs,
	skelfs_vfsop_getv,
	skelfs_vfsop_getv_by_handle,
	skelfs_vfsop_lock,
	skelfs_vfsop_unlock,
	skelfs_vfsop_lkty
};
#endif /* SKELFS */
