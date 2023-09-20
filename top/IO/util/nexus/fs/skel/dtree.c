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
#include <unistd.h>
#include <sys/time.h>

#include "dtree.h"

/*
 * Directory tree inode operations.
 *
 * Author: Scott Morgan, Abba Technologies (smorgan@sandia.gov)
 *
 */

IDENTIFY("$Id: dtree.c,v 1.6 2001/07/18 18:57:39 rklundt Exp $");

/*
 * inode hash helper functions
 */

static unsigned
dtree_inohash_hash(void *key)
{
	z_ino_t		*inum = (z_ino_t *) key;

	return (unsigned ) *inum;

}

static int
dtree_inohash_comp(void *key1, void *key2)
{
	z_ino_t		*inum1 = (z_ino_t *) key1;
	z_ino_t		*inum2 = (z_ino_t *) key2;

	return (int ) ((int ) *inum1) - ((int ) *inum2);
}

/*
 * dtree_ino_destroy: destroy a directory tree inode. this function is
 * both the dtree hash table destroy function (thus its formal declaration)
 * and a generic inode destruction utility function.
 *
 * input parameters:
 *	ent:		the hash table entry (ie, a dtree inode)
 *
 * output parameters: none.
 *
 * return value: none.
 */
void
dtree_ino_destroy(void *ent)
{
	dtree_ino	*ino = (dtree_ino *) ent;
	rwty_t		lock;

	/* lock the inode (both op and attributes), as appropriate */
	if ((lock = rw_lkstat(&ino->dti_lock)) != RWLK_WRITE) {
		/* if called with the inode locked, it *must* be write locked */
		assert(lock == RWLK_NONE);
		assert(rw_lock(&ino->dti_lock, RWLK_WRITE, 0) == 0);
	}

	if ((lock = rw_lkstat(&ino->dti_alock)) != RWLK_WRITE) {
		/* if called with the inode locked, it *must* be write locked */
		assert(lock == RWLK_NONE);
		assert(rw_lock(&ino->dti_alock, RWLK_WRITE, 0) == 0);
	}

	if (ino->dti_symlink)
		free(ino->dti_symlink);
	if (ino->dti_name)
		free(ino->dti_name);
	if (ino->dti_fh != NULL)
		free(ino->dti_fh);

	rw_unlock(&ino->dti_lock);
	rw_destroy(&ino->dti_lock);
	rw_unlock(&ino->dti_alock);
	rw_destroy(&ino->dti_alock);

	free(ino);

	return;
}

/*
 * dtree_mkinum: create an inode number in the passed directory inode.
 *
 * input parameters:
 *	dinode: the directory containing a new inode
 *	inode: the inode that requires a new inode number.
 *
 * output parameters: none (inode numbers are set within).
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */
static int
dtree_mkinum(dtree_ino *dinode, dtree_ino *inode)
{
	int		ret = 0;

	/* write LOCK dinode? */
	if (S_ISDIR(inode->dti_mode)) {
		mutex_lock(&dinode->dti_root->dt_lock);

		/*
		 * check for file system full. this should probably
		 * attempt to reuse directory inodes....
		 */
		if (dinode->dti_root->dt_lastdino == DTREE_DIRINUM_MAX)
			ret = ENOSPC;
		else {
			/* for readdir: inode numbers start at 2 in dir */
			inode->dti_dlastino = dinode->dti_root->dt_lastdino + 2;
			dinode->dti_root->dt_lastdino += DTREE_DIRINUM_INCR;
		}

		(void ) mutex_unlock(&dinode->dti_root->dt_lock);
	}

	/*
	 * check for wrap. as above, this should probably
	 * attempt to reuse non-dir inodes....
	 */
	if ((dinode->dti_dlastino & DTREE_INUM_MASK) == 0)
		ret = ENOSPC;
	else
		inode->dti_inum = ++dinode->dti_dlastino;

	return ret;
}

/*
 * dtree_mkino: make a dtree inode.
 *
 * input parameters:
 *	dinode:		the parent directory inode of the new inode.
 *	name:		the name associated with the new inode.
 *	mode:		the mode of the new inode (used in inum allocation).
 *	ops:		the underlying inode operations of the new inode.
 *
 * output parameters:
 *	inode:		the resultant new inode.
 *
 * return value: zero on success, an appropriate errno of failure.
 */
static int
dtree_mkino(dtree_ino *dinode, const char *name, z_mode_t mode,
	    dtree_ino_ops *ops, dtree_ino **inode)
{
	int			ret = 0;
	dtree_ino		*ino;

	if ((ino = m_alloc(sizeof(dtree_ino))) == NULL)
		ret = ENOMEM;
	else {
		(void ) memset(ino, 0, sizeof(dtree_ino));
		rw_init(&ino->dti_lock);
		rw_init(&ino->dti_alock);

		ino->dti_namelen = strlen(name);

		/*
		 * set the inode type mode bits now for dtree_mkinum().
		 * they'll be reset accordingly later.
		 */
		ino->dti_mode = mode;
		ino->dti_dev = dinode->dti_dev;
		if ((ino->dti_name = m_alloc(ino->dti_namelen+1)) == NULL) {
			free(ino);
			ret = ENOMEM;
		}
		else if ((ret = dtree_mkinum(dinode, ino)) != 0) {
			free(ino->dti_name);
			free(ino);
		}
		else {
			(void ) strcpy(ino->dti_name, name);
			ino->dti_child = NULL;
			ino->dti_ref = ino->dti_link = 1;
			if (ops != NULL)
				(void ) memcpy(&ino->dti_ops, ops,
					       sizeof(dtree_ino_ops));
		}
	}

	if (!ret)
		*inode = ino;

	return ret;
}

/*
 * dtree_find: locate a name within a directory inode. NOTE: any locking
 * required by this operation must be supplied by the caller.
 *
 * input parameters:
 *	dinode: the directory to search.
 *	name: the name to search for.
 *
 * output parameters: none.
 *
 * return value: a pointer to the dtree inode is returned on success,
 * NULL on failure.
 */
static dtree_ino *
dtree_find(dtree_ino *dinode, char *name)
{
	dtree_ino	*i;

	/* sanity check */
	assert(dinode != NULL && name != NULL);

	i = dinode->dti_child;
	while (i != NULL) {
		if (!strcmp(i->dti_name, name))
			break;
		i = i->dti_next;
	}

	return i;
}

/*
 * dtree_insert_inode: helper function for dtree_add_inode() to insert an
 * inode in the inode number ordered list of the passed directory inode.
 *
 * NOTE: this operation is expensive. a future implementation change to
 * disassociate directory name space from the directory tree structure
 * should alleviate this inefficiency (and the need for this function).
 *
 * input parameters:
 *	dinode:		the directory inode to insert the inode into.
 *	inode:		the inode to insert.
 *
 * output parameters: none.
 *
 * return value: none.
 */
static void
dtree_insert_inode(dtree_ino *dinode, dtree_ino *inode)
{
	dtree_ino		*ino;

	/* search for the proper slot... */
	if (inode->dti_inum < dinode->dti_dhead->dti_inum) {
		/* go to the head of the class. */
		inode->dti_prev = dinode->dti_dhead;
		inode->dti_next = NULL;
		dinode->dti_dhead->dti_next = inode;
		dinode->dti_dhead = inode;
	}
	else {
		 if (dinode->dti_child->dti_inum - inode->dti_inum <
		     inode->dti_inum - dinode->dti_dhead->dti_inum) {
			/* search from the tail */
			for (ino = dinode->dti_child; ino != NULL;
			     ino = ino->dti_next)
				if (ino->dti_inum < inode->dti_inum)
					break;
		}
		else {
			/* search from the head */
			for (ino = dinode->dti_dhead; ino != NULL;
			     ino = ino->dti_prev)
				if (ino->dti_inum > inode->dti_inum) {
					ino = ino->dti_next;
					break;
				}
		}

		/* insert in front of the located inode */
		inode->dti_next = ino;
		inode->dti_prev = ino->dti_prev;
		ino->dti_prev->dti_next = inode;
		ino->dti_prev = inode;
	}

	return;
}

/*
 * dtree_add_inode: add an inode to a directory. NOTE: any required locks
 * must be supplied by the caller.
 *
 * input parameters:
 *	dinode: the directory inode to add to.
 *	inode: the inode to add to the directory.
 *	new: inode is new to the directory tree.
 *
 * output parameters: none.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */
static int
dtree_add_inode(dtree_ino *dinode, dtree_ino *inode, int new)
{
	int		ret = 0;

	/* sanity check: insure the inode number doesn't already exist */
	if (new &&
	    hash_get(dinode->dti_root->dt_inos, &inode->dti_inum) != NULL)
		panic("dtree_add_inode: duplicate inode number "
		      FMT_Z_DEV_T "/" FMT_Z_INO_T,
		      dinode->dti_dev, dinode->dti_inum);

	/* insert inode into tree hash: should we CHECK for */
	if (new && hash_insert(dinode->dti_root->dt_inos, &inode->dti_inum,
			inode) != 0) {
		/* most likely out of memory */
		ret = errno;
	}
	else {
		/* add the inode to the directory */
		mutex_lock(&dinode->dti_root->dt_lock);
		dinode->dti_root->dt_ninos++;
		mutex_unlock(&dinode->dti_root->dt_lock);

		/*
		 * NOTE: to support the current dtree_readdir() algorithm,
		 * the inode's must be in inode number order (ascending
		 * from dti_dhead, descending from dti_child). in a fairly
		 * static name space, this operation is trivial. if a
		 * multiplicity of dtree_rename()s occur into different
		 * directories, this operation becomes expensive.
		 *
		 * a pending modification to disassociate the name space
		 * from the directory tree linkage should alleviate this
		 * inefficiency....
		 */

		if (dinode->dti_child != NULL &&
		    inode->dti_inum < dinode->dti_child->dti_inum) {
			dtree_insert_inode(dinode, inode);
		}
		else {
			if (dinode->dti_child == NULL)
				dinode->dti_dhead = inode;
			else
				dinode->dti_child->dti_prev = inode;

			inode->dti_next = dinode->dti_child;
			inode->dti_prev = NULL;
			/* NOTE: inode child must be initialized by caller */

			dinode->dti_child = inode;
		}

		inode->dti_parent = dinode;
		inode->dti_root = dinode->dti_root;
	}

	return ret;
}

/*
 * dtree_rem_inode: remove the inode from the passed directory. NOTE: any
 * required locks must be supplied by the caller.
 *
 * input parameters:
 *	dinode: the directory to remove from.
 *	inode: the inode to remove.
 *	destroy: inode to be destroyed.
 *
 * output parameters: none.
 *
 * return value: zero on success, an appropriate errno on failure.
 */
static int
dtree_rem_inode(dtree_ino *dinode, dtree_ino *inode, int destroy)
{
#if 0
	/***
	 *** this won't work with rename support. must rethink...
	 ***/
	/* reclaim inode numbers, as appropriate */
	if (dinode->dti_lastdino - 1 == inode->dti_inum)
		dinode->dti_lastdino = inode->dti_next->dti_inum + 1;
#endif

	if (dinode->dti_child == inode) {
		dinode->dti_child = dinode->dti_child->dti_next;
		if (dinode->dti_child != NULL)
			dinode->dti_child->dti_prev = NULL;
	}
	else {
		inode->dti_prev->dti_next = inode->dti_next;
		if (inode->dti_next != NULL)
			inode->dti_next->dti_prev = inode->dti_prev;
	}

	if (dinode->dti_dhead == inode)
		dinode->dti_dhead = inode->dti_prev;

	/* paranoia: zero inode links */
	inode->dti_next = inode->dti_prev = inode->dti_parent = NULL;

	/* destroy inode, as appropriate */
	if (destroy) {
		/*
		 * remove inode from hash table: since the search by inode
		 * number must be tree-lock protected, the tree must be
		 * locked during removal.
		 */
		mutex_lock(&dinode->dti_root->dt_lock);
		dinode->dti_root->dt_ninos--;

		if (hash_remove(dinode->dti_root->dt_inos,
				&inode->dti_inum) == NULL)
			panic("dtree_rem_inode: freeing free inode "
			      FMT_Z_DEV_T "/" FMT_Z_INO_T,
			      inode->dti_dev, inode->dti_inum);

		mutex_unlock(&dinode->dti_root->dt_lock);

		/*
		 * actual destruction of inode resources is done by the
		 * reclaim function. at this point, the inode is a
		 * disjoint graph of one node and not locatable within
		 * the file system.
		 */
	}

	return 0;
}

/*
 * dtree_isauth: authorize a client for the specified operation.
 *
 * input parameters:
 *	dinode: the parent directory inode containing inode.
 *	inode: the dtree inode to operate on
 *	cr: the client credentials
 *	op: the operation to authorize
 *
 * output parameters: none.
 *
 * return value: boolean: TRUE for an authorized operation, FALSE otherwise.
 */
static int
dtree_isauth(dtree_ino *dinode, dtree_ino *inode, struct creds *cr,
	     enum dtree_op op)
{
	int		ismylock = 0;
	int		d_ismylock = 0;		/* for dinode */
	int		ret = FALSE;
	size_t		i;

	/* sanity check */
	if (inode == NULL || cr == NULL) {
		errno = EINVAL;
		return ret;
	}

	/* allow the super user to do anything */
	if (is_suser(cr))
		return TRUE;

	/* acquire attributes lock */
	if (rw_lkstat(&inode->dti_alock) == RWLK_NONE) {
		ismylock++;
		if ((ret = rw_lock(&inode->dti_alock, RWLK_READ, 0)) != 0)
			return ret;
	}

	/* support sticky directories? */
	switch (op) {

	case DTREE_OP_READ:
		if (inode->dti_uid == cr->cr_uid && inode->dti_mode & S_IRUSR)
			ret = TRUE;
		else if (inode->dti_mode & S_IROTH)
			ret = TRUE;
		else {
			if (!(inode->dti_mode & S_IRGRP))
				break;		/* no group read */

			/* group search */
			for (i = 0; i < cr->cr_ngroups; i++) {
				if (inode->dti_gid == cr->cr_groups[i]) {
					ret = TRUE;
					break;
				}
			}
		}
		break;

	case DTREE_OP_WRITE:
		if (inode->dti_uid == cr->cr_uid && inode->dti_mode & S_IWUSR)
			ret = TRUE;
		else if (inode->dti_mode & S_IWOTH)
			ret = TRUE;
		else {
			if (!(inode->dti_mode & S_IWGRP))
				break;		/* no group write */

			/* group search */
			for (i = 0; i < cr->cr_ngroups; i++) {
				if (inode->dti_gid == cr->cr_groups[i]) {
					ret = TRUE;
					break;
				}
			}
		}
		break;

	case DTREE_OP_EXEC:
		if (inode->dti_uid == cr->cr_uid && inode->dti_mode & S_IXUSR)
			ret = TRUE;
		else if (inode->dti_mode & S_IXOTH)
			ret = TRUE;
		else {
			if (!(inode->dti_mode & S_IXGRP))
				break;		/* no group exec */

			/* group search */
			for (i = 0; i < cr->cr_ngroups; i++) {
				if (inode->dti_gid == cr->cr_groups[i]) {
					ret = TRUE;
					break;
				}
			}
		}
		break;

	case DTREE_OP_SATTR:
		/* only an inode owner can change attributes */
		if (inode->dti_uid == cr->cr_uid)
			ret = TRUE;
			break;

	/*
	 * write permission on directory is required for both create and
	 * destroy. the parent will be used to support "sticky" directories
	 * (later).
	 */
	case DTREE_OP_CREATE:
	case DTREE_OP_DESTROY:

		/* acquire directory inode attributes lock */
		if (rw_lkstat(&dinode->dti_alock) == RWLK_NONE) {
			d_ismylock++;
			if ((ret = rw_lock(&dinode->dti_alock, RWLK_READ, 0))
					!= 0)
				return ret;
		}

		if (dinode->dti_uid == cr->cr_uid && dinode->dti_mode & S_IWUSR)
			ret = TRUE;
		else if (dinode->dti_mode & S_IWOTH)
			ret = TRUE;
		else {
			if (!(dinode->dti_mode & S_IWGRP))
				break;		/* no group write */

			/* group search */
			for (i = 0; i < cr->cr_ngroups; i++) {
				if (dinode->dti_gid == cr->cr_groups[i]) {
					ret = TRUE;
					break;
				}
			}
		}

		if (d_ismylock)
			rw_unlock(&dinode->dti_alock);
		break;

	}

	if (ismylock)
		rw_unlock(&inode->dti_alock);

	return ret;
}

/*
 * dtree_findi: given a inode number is a directory tree, return the
 * associated directory tree inode.
 *
 * input parameters:
 *	tree: the directory tree to search.
 *	inum: the inode number to search for.
 *	lock: the lock type required for this inode.
 *
 * output parameters:
 *	ino: the located inode.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */
int
dtree_findi(dtree *tree, z_ino_t inum, rwty_t lock, dtree_ino **inop)
{
	int			ret = 0;
	dtree_ino		*ino;

	/* sanity check */
	if (tree == NULL || inop == NULL)
		return EINVAL;

	/*
	 * lock the entire tree down until we acquire the desired lock
	 * on the requested inode.
	 */
	mutex_lock(&tree->dt_lock);

	/* since all inodes are stored in a hash table, just search it */
	if ((ino = (dtree_ino *) hash_get(tree->dt_inos, &inum)) == NULL)
		ret = ENOENT;		/* not found */
	else {
		*inop = ino;
		ret = rw_lock(&ino->dti_lock, lock, 0);
	}

	mutex_unlock(&tree->dt_lock);

	return ret;
}

/*
 * dtree_load_attr: helper function to load attributes on newly created
 * directory tree inodes.
 *
 * input parameters:
 *	inode: the inode whose attributes are to be loaded.
 *	dinode: the parent directory of inode.
 *	cr: the caller's identification/authorization credentials
 *	attr: the attributes to load.
 *	type: type of inode being set (ie, the mode bits corresponding to
 *		the inode's type).
 *
 * output parameters:
 *	now: the time used in updating inode attributes.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */
static int
dtree_load_attr(dtree_ino *inode, dtree_ino *dinode, struct creds *cr,
		dtree_attr *attr, z_mode_t type, struct timeval *now)
{
	size_t		i;

	if (VST_DOESSET_MODE(attr))
		inode->dti_mode = (attr->vst_mode & ~S_IFMT) | type;
	else {
		if (type == S_IFDIR)
			inode->dti_mode = DTREE_DEF_DIRMODE | type;
		else
			inode->dti_mode = DTREE_DEF_FILEMODE | type;
	}

	if (VST_DOESSET_UID(attr))
		inode->dti_uid = attr->vst_uid;
	else
		inode->dti_uid = cr->cr_uid;

	/* group inheritence from parent directory */
	if (VST_DOESSET_GID(attr)) {
		/* insure the caller is in the specified group */
		if (!is_suser(cr)) {
			for (i = 0; i < cr->cr_ngroups; i++)
				if (attr->vst_gid == cr->cr_groups[i])
					break;

			if (i == cr->cr_ngroups)
				return EACCES;
		}
		inode->dti_gid = attr->vst_gid;
	}
	else
		inode->dti_gid = dinode->dti_gid;

	/* our granularity of time is microseconds */
	(void ) gettimeofday(now, NULL);

	if (VST_DOESSET_MTIME_SEC(attr))
		inode->dti_mtime.tv_sec = attr->vst_mtime.tv_sec;
	else
		inode->dti_mtime.tv_sec = now->tv_sec;

	if (VST_DOESSET_MTIME_NSEC(attr))
		inode->dti_mtime.tv_nsec = attr->vst_mtime.tv_nsec;
	else
		inode->dti_mtime.tv_nsec = (unsigned ) now->tv_usec * 1000;

	if (VST_DOESSET_ATIME_SEC(attr))
		inode->dti_atime.tv_sec = attr->vst_atime.tv_sec;
	else
		inode->dti_atime.tv_sec = now->tv_sec;

	if (VST_DOESSET_ATIME_NSEC(attr))
		inode->dti_atime.tv_nsec = attr->vst_atime.tv_nsec;
	else
		inode->dti_atime.tv_nsec = (unsigned ) now->tv_usec * 1000;

	inode->dti_ctime.tv_sec = now->tv_sec;
	inode->dti_ctime.tv_nsec = now->tv_usec * 1000;

	return 0;
}

/*
 * dtree_create_root: create an in-core directory tree with a single
 * route directory.
 *
 * input parameters:
 *	fsid: the file system creating the directory tree.
 *	ops: root directory inode operations.
 *
 * output parameters:
 *	treep: a pointer to the created dtree.
 *
 * return value: zero (0) is returned on success, an appropriate errno
 * on failure.
 */
int
dtree_create_root(z_dev_t fsid, dtree_ino_ops *ops, dtree **treep)
{
	int		ret = 0;
	struct timeval	now;
	dtree		*tree = NULL;
	dtree_ino	*root = NULL;

	/* sanity check */
	if (treep == NULL)
		return EINVAL;

	/* allocate/initialize dtree descriptor */
	if ((tree = m_alloc(sizeof(dtree))) == NULL)
		ret = ENOMEM;
	else {
		(void ) memset(tree, 0, sizeof(dtree));
		tree->dt_lastdino = 0;

		mutex_init(&tree->dt_lock);
	}

	/* create inode hash table (for inum based lookups) */
	if (!ret && (tree->dt_inos = hash_create(DTREE_HASH_SIZE,
					       dtree_inohash_hash,
					       dtree_inohash_comp,
					       dtree_ino_destroy))
				== NULL)
		ret = errno;

	/*
	 * create root inode: since the root inode is special, it is
	 * created in-line, rather than calling dtree_mkino().
	 */
	if (!ret && (root = m_alloc(sizeof(dtree_ino))) == NULL) {
		ret = ENOMEM;
	}
	else {
		(void ) memset(root, 0, sizeof(dtree_ino));
		rw_init(&root->dti_lock);
		rw_init(&root->dti_alock);

		root->dti_namelen = 1;
		if ((root->dti_name = m_alloc(2 * sizeof(char))) == NULL)
			ret = ENOMEM;
		else {
			(void ) strcpy(root->dti_name, "/");
			root->dti_parent = root;	/* only root link */
			(void ) memcpy(&root->dti_ops, ops,
				       sizeof(dtree_ino_ops));

			root->dti_inum = tree->dt_lastdino + 2;
			tree->dt_fsid = fsid;
			root->dti_dev = fsid;
			tree->dt_lastdino += DTREE_DIRINUM_INCR;
			tree->dt_root = root;
			tree->dt_ninos = 1;
			root->dti_dlastino = root->dti_inum;
			root->dti_ref = root->dti_link = 2;
			root->dti_root = tree;

			root->dti_mode = S_IFDIR|DTREE_DEF_DIRMODE;
			root->dti_uid = SUSER_UID;
			root->dti_gid = DTREE_DEF_GID;

			(void ) gettimeofday(&now, NULL);
			root->dti_atime.tv_sec = root->dti_mtime.tv_sec
					       = root->dti_ctime.tv_sec
					       = now.tv_sec;
			/* our granularity of time is microseconds */
			root->dti_atime.tv_nsec = root->dti_mtime.tv_nsec
					       = root->dti_ctime.tv_nsec
					       = now.tv_usec * 1000;

			if ((ret = hash_insert(tree->dt_inos, &root->dti_inum,
					       root)) != 0) {
				free(root);
			}
		}
	}

	/* dispose of errors */
	if (ret) {
		if (tree != NULL) {
			mutex_destroy(&tree->dt_lock);
			if (tree->dt_inos != NULL)
				hash_destroy(tree->dt_inos);  /* free()s root */

			free(tree);
		}
	}
	else
		*treep = tree;

	return ret;
}

/*
 * dtree_statfs: return file system statistics for the passed directory tree.
 *
 * input parameters:
 *	tree:		the directory tree file system to interrogate.
 *
 * output parameters:
 *	stats:		the directory tree file system statistics.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */
int
dtree_statfs(dtree *tree, struct fsstats *stats)
{
	unsigned int	ninos;

	mutex_lock(&tree->dt_lock);
	ninos = tree->dt_ninos;
	mutex_unlock(&tree->dt_lock);

	stats->fsstats_tsize = VMPAGESIZE;
	stats->fsstats_bsize = VMPAGESIZE;
	stats->fsstats_blocks = VMPAGESIZE * ninos;
	stats->fsstats_bfree = 0;
	stats->fsstats_bavail = 0;

	return 0;
}

/*
 * dtree_destroy: chop down the directory tree (freeing all resources
 * as we go).
 *
 * input parameters:
 *	tree: the dtree to destroy.
 *
 * output parameters: none.
 *
 * return value: none.
 */
void
dtree_destroy(dtree *tree)
{
	/* sanity check */
	if (tree == NULL)
		return;

	mutex_lock(&tree->dt_lock);

	/* destroying the inode hash table destroys all inodes */
	hash_destroy(tree->dt_inos);

	(void ) mutex_unlock(&tree->dt_lock);
	(void ) mutex_destroy(&tree->dt_lock);
	free(tree);

	return;
}

/*
 * dtree_getattr: return the attributes of the passed dtree inode.
 *
 * input parameters:
 *	inode: dtree node whose attributes are desired.
 *
 * output parameters:
 *	attr: the returned attributes.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */
int
dtree_getattr(dtree_ino *inode, dtree_attr *attr)
{
	int		ret = 0;

	/* sanity check */
	if (inode == NULL || attr == NULL)
		return EINVAL;

	/* insure inode is at least read locked */
	BUG_CHECK(rw_lkstat(&inode->dti_lock) != RWLK_NONE);

	if ((ret = rw_lock(&inode->dti_alock, RWLK_READ, 0)) != 0)
		return ret;

	/* copy attributes maintained by the dtree inode */
	(void ) memset(attr, 0, sizeof(struct vstat));
	attr->vst_mode = inode->dti_mode;
	attr->vst_nlink = inode->dti_link;
	attr->vst_uid = inode->dti_uid;
	attr->vst_gid = inode->dti_gid;
	attr->vst_fsid = inode->dti_dev;
	attr->vst_fileid = inode->dti_inum;
	(void ) memcpy(&attr->vst_atime, &inode->dti_atime,
		       3 * sizeof(struct timespec));

	/*
	 * fill in the remaining attributes from the inode specific
	 * attributes function. the dtree_attr struct fits nicely
	 * within the appropriate space of the struct vstat (lucky,
	 * huh).
	 */
	if (inode->dti_ops.dtio_gattr != NULL)
		ret = (*inode->dti_ops.dtio_gattr)(inode,
					(dtree_prvattr *) &attr->vst_size);

	rw_unlock(&inode->dti_alock);

	return ret;
}

/*
 * dtree_setattr: set attributes on the passed inode, if the credentials
 * suffice.
 *
 * input parameters:
 *	inode: the dtree inode to modify.
 *	attr: the attributes to set.
 *
 * output parameters: none.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */
int
dtree_setattr(dtree_ino *inode, dtree_attr *attr, struct creds *cr)
{
	int		ret = 0;
	size_t		i;
	struct timeval	now;

	/* sanity check */
	if (inode == NULL || attr == NULL || cr == NULL)
		return EINVAL;

	if (!VST_VALID(attr))
		return EINVAL;

	/* insure inode is write locked */
	BUG_CHECK(rw_lkstat(&inode->dti_lock) == RWLK_WRITE);

	if ((ret = rw_lock(&inode->dti_alock, RWLK_WRITE, 0)) != 0)
		return ret;

	/* authenticate/authorize for this operation */
	if (dtree_isauth(inode->dti_parent, inode, cr, DTREE_OP_SATTR)
			== FALSE) {
		rw_unlock(&inode->dti_alock);
		return EPERM;
	}

	/*
	 * since setting the group attribute requires an authorization check,
	 * attempt to set it first.
	 */
	if (VST_DOESSET_GID(attr)) {
		/* insure caller is in the desired group */
		for (i = 0; i < cr->cr_ngroups; i++)
			if (attr->vst_gid == cr->cr_groups[i])
				break;

		if (i == cr->cr_ngroups) {
			rw_unlock(&inode->dti_alock);
			return EACCES;
		}
		else
			inode->dti_gid = attr->vst_gid;
	}

	/*
	 * attempt to set the underlying dtree inode attributes before
	 * setting any other inode maintained attributes
	 */
	if (inode->dti_ops.dtio_sattr != NULL &&
	    (ret = (*inode->dti_ops.dtio_sattr)(inode,
				(dtree_prvattr *) &attr->vst_size)) != 0) {
		rw_unlock(&inode->dti_alock);
		return ret;
	}

	if (VST_DOESSET_MODE(attr))
		inode->dti_mode = (inode->dti_mode & S_IFMT) |
		  (attr->vst_mode & (VS_IRWXU | VS_IRWXU >> 3 | VS_IRWXU >> 6));

	if (VST_DOESSET_UID(attr))
		inode->dti_uid = attr->vst_uid;

	if (VST_DOESSET_ATIME_SEC(attr))
		inode->dti_atime.tv_sec = attr->vst_atime.tv_sec;

	if (VST_DOESSET_ATIME_NSEC(attr))
		inode->dti_atime.tv_nsec = attr->vst_atime.tv_nsec;

	if (VST_DOESSET_MTIME_SEC(attr))
		inode->dti_mtime.tv_sec = attr->vst_mtime.tv_sec;

	if (VST_DOESSET_MTIME_NSEC(attr))
		inode->dti_mtime.tv_nsec = attr->vst_mtime.tv_nsec;

	/* update the inode's ctime */
	(void ) gettimeofday(&now, NULL);
	inode->dti_ctime.tv_sec = now.tv_sec;
	inode->dti_ctime.tv_nsec = now.tv_usec * 1000;

	rw_unlock(&inode->dti_alock);

	return ret;
}

/*
 * dtree_lookup: find a node in the passed directory.
 *
 * input parameters:
 *	dinode: the directory to search.
 *	name: the name of the node to find in dinode.
 *	cr: authorization credentials.
 *
 * output parameters:
 *	inop: pointer to the located dtree inode.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */
int
dtree_lookup(dtree_ino *dinode, char *name, struct creds *cr, dtree_ino **inop)
{
	int		ret = 0;

	/* sanity check */
	if (dinode == NULL || name == NULL || cr == NULL || inop == NULL)
		return EINVAL;

	/* insure directory inode is at least read locked */
	BUG_CHECK(rw_lkstat(&dinode->dti_lock) != RWLK_NONE);

	/* authorize client for directory search */
	if (dtree_isauth(dinode->dti_parent, dinode, cr, DTREE_OP_EXEC)
			== FALSE)
		ret = EPERM;

	/* search the subdirectory of dinode for the desired entry */
	if (!ret && (*inop = dtree_find(dinode, name)) == NULL)
		ret = ENOENT;

	return ret;
}

/*
 * dtree_readlink: read the symbolic link contained in an inode.
 *
 * input parameters:
 *	inode: the symbolic link inode.
 *	linkbuf: buffer for link.
 *	len: length of input buffer.
 *
 * output parameters:
 *	len: actual length of symbolic link.
 *
 * return value: zero (0) on success, an appropriate errno on error.
 */
int
dtree_readlink(dtree_ino *inode, char *linkbuf, u_int32_t *len)
{
	int		ret = 0;
	struct timeval	now;

	/* sanity check */
	if (inode == NULL || linkbuf == NULL || len == NULL)
		return EINVAL;

	/* insure link inode is at least read locked */
	BUG_CHECK(rw_lkstat(&inode->dti_lock) != RWLK_NONE);

	(void ) strncpy(linkbuf, inode->dti_symlink, *len);

	*len = min(*len, strlen(inode->dti_symlink));

	/* update access time on inode */
	assert(rw_lock(&inode->dti_alock, RWLK_WRITE, 0) == 0);

	(void ) gettimeofday(&now, NULL);
	inode->dti_atime.tv_sec = now.tv_sec;
	inode->dti_atime.tv_nsec = now.tv_sec * 1000;

	rw_unlock(&inode->dti_alock);

	return ret;
}

/*
 * dtree_read: read a block of data from an inode.
 *
 * input parameters:
 *	inode: the inode to read from.
 *	off: offset for read.
 *	len: the length of the input buffer.
 *	cr: authorization credentials.
 *
 * output parameters:
 *	buf: the data read from inode.
 *	len: the actual length of read data.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */
int
dtree_read(dtree_ino *inode, z_off_t off, char *buf, u_int32_t *len,
	   struct creds *cr)
{
	int			ret = 0;
	struct timeval		now;

	/* sanity check */
	if (inode == NULL || len == NULL || cr == NULL)
		return EINVAL;

	if (buf == NULL)
		return EFAULT;

	/* insure inode is at least read locked */
	BUG_CHECK(rw_lkstat(&inode->dti_lock) != RWLK_NONE);

	/* if the underlying layer doesn't support reads, bail out */
	if (inode->dti_ops.dtio_read == NULL)
		return EPERM;

	/* authorize client for read */
	if (dtree_isauth(inode->dti_parent, inode, cr, DTREE_OP_READ) ==
			FALSE)
		return EPERM;

	/* insure the inode is not a directory */
	if (S_ISDIR(inode->dti_mode))
		return EISDIR;

	/* call the underlying read function */
	ret = (*inode->dti_ops.dtio_read)(inode, off, buf, len);

	/* update inode access time */
	if (ret == 0) {
		assert(rw_lock(&inode->dti_alock, RWLK_WRITE, 0) == 0);

		(void ) gettimeofday(&now, NULL);
		inode->dti_atime.tv_sec = now.tv_sec;
		inode->dti_atime.tv_nsec = now.tv_usec * 1000;

		rw_unlock(&inode->dti_alock);
	}

	return ret;
}

/*
 * dtree_write: write a block of data to an inode.
 *
 * input parameters:
 *	inode: the inode to write to.
 *	off: the offset to write at.
 *	buf: the block of data to write.
 *	len: the length of data to write.
 *	cr: authorization credentials.
 *
 * output parameters:
 *	len: the actual length of data written.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */
int
dtree_write(dtree_ino *inode, z_off_t off, char *buf, u_int32_t *len,
	    struct creds *cr)
{
	int			ret = 0;
	struct timeval		now;

	/* sanity check */
	if (inode == NULL || len == NULL || cr == NULL)
		return EINVAL;

	if (buf == NULL)
		return EFAULT;

	/* insure inode is at least read locked */
	BUG_CHECK(rw_lkstat(&inode->dti_lock) != RWLK_NONE);

	/* if the underlying layer doesn't support writes, bail out */
	if (inode->dti_ops.dtio_write == NULL)
		return EPERM;

	/* authorize client for inode write */
	if (dtree_isauth(inode->dti_parent, inode, cr, DTREE_OP_WRITE)
			== FALSE)
		return EPERM;

	/* paranoia: insure the inode is not a directory */
	if (S_ISDIR(inode->dti_mode))
		return EISDIR;

	ret = (*inode->dti_ops.dtio_write)(inode, off, buf, len);

	/* update inode modification time */
	if (ret == 0) {
		assert(rw_lock(&inode->dti_alock, RWLK_WRITE, 0) == 0);

		(void ) gettimeofday(&now, NULL);
		inode->dti_mtime.tv_sec = now.tv_sec;
		inode->dti_mtime.tv_nsec = now.tv_usec * 1000; 

		rw_unlock(&inode->dti_alock);
	}

	return ret;
}

/*
 * dtree_create: create a file in the passed directory inode.
 *
 * input parameters:
 *	dinode: the directory inode to create a file in.
 *	name: the file name to create.
 *	ops: dtree inode operations.
 *
 * output parameters:
 *	inop: the resultant inode.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */
int
dtree_create(dtree_ino *dinode, char *name, dtree_attr *attr, struct creds *cr,
	     dtree_ino_ops *ops, dtree_ino **inop)
{
	int		ret = 0;
	dtree_ino	*ino;
	struct timeval	now;

	/* sanity check */
	if (dinode == NULL || name == NULL || ops == NULL || inop == NULL)
		return EINVAL;

	if (strlen(name) > Z_MAXNAMLEN)
		return ENAMETOOLONG;

	/* insure inode is write locked */
	BUG_CHECK(rw_lkstat(&dinode->dti_lock) == RWLK_WRITE);

	/* if the underlying layer doesn't support file creates, bail out */
	if (dinode->dti_ops.dtio_create == NULL)
		return EPERM;

	/* insure client is authorized to create a file in this directory */
	if (dtree_isauth(dinode, dinode, cr, DTREE_OP_CREATE) == FALSE)
		return EACCES;

	/* insure the current name doesn't exist in this directory */
	if (dtree_find(dinode, name) != NULL)
		return EEXIST;

	/* paranoia: make sure the new inode is not a directory */
	if (S_ISDIR(attr->vst_mode))
		return EISDIR;			/* ??? */

	/* create the new inode */
	if ((ret = dtree_mkino(dinode, name, S_IFREG|DTREE_DEF_FILEMODE, ops,
			       &ino)) != 0)
		return ret;

	/* load the requested inode attributes */
	if ((ret = dtree_load_attr(ino, dinode, cr, attr, S_IFREG, &now))
				== 0) {
		/* call underlying create op */
		if ((ret = (*ino->dti_ops.dtio_create)(ino, &ino->dti_prv))
					== 0) {
			/* link inode into tree */
			if ((ret = dtree_add_inode(dinode, ino, TRUE)) == 0) {
				/* update mod time of directory */
				assert(rw_lock(&dinode->dti_alock, RWLK_WRITE,
					       0) == 0);

				dinode->dti_mtime.tv_sec = now.tv_sec;
				dinode->dti_mtime.tv_nsec = now.tv_usec * 1000;
				dinode->dti_ctime.tv_sec = now.tv_sec;
				dinode->dti_ctime.tv_nsec = now.tv_usec * 1000;
				dinode->dti_ref++;

				rw_unlock(&dinode->dti_alock);
			}
		}
	}

	if (ret == 0)
		*inop = ino;
	else
		dtree_ino_destroy(ino);

	return ret;
}

/*
 * dtree_remove: remove a file from the passed directory inode.
 *
 * input parameters:
 *	dinode: the directory inode to remove from.
 *	name: the file inode name to remove.
 *	inode: the inode to remove.
 *	cr: authorization credentials.
 *
 * output parameters: none.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */
int
dtree_remove(dtree_ino *dinode, char *name, dtree_ino *inode, struct creds *cr)
{
	int		ret = 0;
	struct timeval	now;

	/* sanity check */
	if (dinode == NULL || name == NULL || inode == NULL || cr == NULL)
		return EINVAL;

	/* insure directory inode and inode are write locked */
	BUG_CHECK(rw_lkstat(&dinode->dti_lock) == RWLK_WRITE);
	BUG_CHECK(rw_lkstat(&inode->dti_lock) == RWLK_WRITE);

	/*
	 * if the underlying layer doesn't support file destroys, bail out,
	 * save on symbolic links (a pure name space operation, in theory).
	 */
	if (inode->dti_ops.dtio_destroy == NULL && !S_ISLNK(inode->dti_mode))
		return EPERM;

	/*
	 * verify inode is in dinode's directory and is not a directory
	 * itself.
	 */
	if (dtree_find(dinode, name) != inode) {
		ret = ENOENT;
	}
	else if (dtree_isauth(dinode, inode, cr, DTREE_OP_DESTROY) == FALSE) {
		ret = EPERM;
	}
	else if (S_ISDIR(inode->dti_mode)) {
		ret = EISDIR;
	}
	else {
		/*
		 * only destroy the inode if all references have been
		 * removed.
		 */
		assert(rw_lock(&inode->dti_alock, RWLK_WRITE, 0) == 0);

		if (--inode->dti_ref == 0) {
			/* call underlying destroy function */
			if (inode->dti_ops.dtio_destroy != NULL &&
			    (ret = (*inode->dti_ops.dtio_destroy)(inode))
					!= 0) {
				/* underlying destroy failed: restore inode */
				inode->dti_ref++;
			}
			else {
				/* write LOCK dinode? */
				if ((ret = dtree_rem_inode(dinode, inode, TRUE))
						== 0) {
					/* update dir mod time/link count */
					assert(rw_lock(&dinode->dti_alock,
					       RWLK_WRITE, 0) == 0);

					dinode->dti_ref--;
					(void ) gettimeofday(&now, NULL);
					dinode->dti_mtime.tv_sec = now.tv_sec;
					dinode->dti_mtime.tv_nsec =
						now.tv_usec * 1000;
					dinode->dti_ctime.tv_sec = now.tv_sec;
					dinode->dti_ctime.tv_nsec =
						now.tv_usec * 1000;

					rw_unlock(&dinode->dti_alock);
				}
			}
		}

		rw_unlock(&inode->dti_alock);
	}

	return ret;
}

/*
 * dtree_rename: move an inode around in the directory tree name space.
 *
 * input parameters:
 *	f_dinode: the source directory inode.
 *	f_name: the source inode name.
 *	inode: the inode to rename.
 *	t_dinode: the destination directory inode.
 *	t_name: the destination inode name.
 *	cr: authorization credentials.
 *
 * output parameters: none.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */
int
dtree_rename(dtree_ino *f_dinode, char *f_name, dtree_ino *inode,
	     dtree_ino *t_dinode, char *t_name, struct creds *cr)
{
	char		*name,
			*oname;
	int		ret = 0;
	struct timeval	now;

	/* sanity check */
	if (f_dinode == NULL || f_name == NULL || inode == NULL ||
	    t_dinode == NULL || t_name == NULL || cr == NULL)
		return EINVAL;

	/***
	 *** locking in rename is complex. truly, it requires simultaneously
	 *** obtained write locks the source and destination directories as
	 *** well as the inode itself to proceed.
	 ***/
	BUG_CHECK(rw_lkstat(&f_dinode->dti_lock) == RWLK_WRITE);
	BUG_CHECK(rw_lkstat(&t_dinode->dti_lock) == RWLK_WRITE);
	BUG_CHECK(rw_lkstat(&inode->dti_lock) == RWLK_WRITE);

	/* locate source name in source directory */
	if (dtree_find(f_dinode, f_name) != inode)
		return ENOENT;

	/* insure destination name does not exist in the destination dir */
	if (dtree_find(t_dinode, t_name) != NULL)
		return EEXIST;

	/*
	 * authorize client for rename operation: this requires both removal
	 * authorization in the source directory and creation authorization
	 * in the destination directory.
	 */
	if (dtree_isauth(f_dinode, inode, cr, DTREE_OP_DESTROY) == FALSE ||
	    dtree_isauth(t_dinode, t_dinode, cr, DTREE_OP_CREATE) == FALSE)
		return EACCES;

	/* allocate new name */
	if ((name = m_alloc(strlen(t_name)+1)) == NULL)
		return ENOMEM;
	(void ) strcpy(name, t_name);

	/*
	 * remove inode from source directory, adding to destination
	 * directory if successful.
	 */
	/* write LOCK f_dinode and t_dinode and inode? */
	oname = inode->dti_name;
	inode->dti_name = name;
	inode->dti_namelen = strlen(name);

	/*
	 * NOTE: the inode reuses the inode number from the source dir
	 */
	if ((ret = dtree_rem_inode(f_dinode, inode, FALSE)) == 0) {
		if ((ret = dtree_add_inode(t_dinode, inode, FALSE)) != 0) {
			/* destination dir insert failed: put it back */
			free(name);
			inode->dti_name = oname;
			inode->dti_namelen = strlen(oname);

			assert(dtree_add_inode(f_dinode, inode, FALSE) == 0);
		}
		else {
			/* update source directory link count/mod time */
			assert(rw_lock(&f_dinode->dti_alock, RWLK_WRITE, 0)
				== 0);
			f_dinode->dti_ref--;
			if (S_ISDIR(inode->dti_mode))
				f_dinode->dti_link--;
			(void ) gettimeofday(&now, NULL);
			f_dinode->dti_mtime.tv_sec = now.tv_sec;
			f_dinode->dti_mtime.tv_nsec = now.tv_usec * 1000;
			f_dinode->dti_ctime.tv_sec = now.tv_sec;
			f_dinode->dti_ctime.tv_sec = now.tv_usec * 1000;
			rw_unlock(&f_dinode->dti_alock);

			/* update destination directory link count/mod time */
			assert(rw_lock(&t_dinode->dti_alock, RWLK_WRITE, 0)
				== 0);
			t_dinode->dti_ref++;
			if (S_ISDIR(inode->dti_mode))
				t_dinode->dti_link++;
			t_dinode->dti_mtime.tv_sec = now.tv_sec;
			t_dinode->dti_mtime.tv_nsec = now.tv_usec * 1000;
			/* inode reference count remains the same */
			/* update inode ctime ATTR? */
			t_dinode->dti_ctime.tv_sec = now.tv_sec;
			t_dinode->dti_ctime.tv_sec = now.tv_usec * 1000;
			rw_unlock(&t_dinode->dti_alock);

			/* update inode change time */
			assert(rw_lock(&inode->dti_alock, RWLK_WRITE, 0)
				== 0);
			inode->dti_ctime.tv_sec = now.tv_sec;
			inode->dti_ctime.tv_nsec = now.tv_usec * 1000;
			rw_unlock(&inode->dti_alock);

			free(oname);
		}
	}

	return ret;
}

/*
 * dtree_link: add a link in the directory tree name space to an existing
 * inode.
 *
 * input parameters:
 *	inode: inode to create a new link for.
 *	dinode: directory to create the new link in.
 *	name: name of the new link.
 *	cr: authorization credentials.
 *
 * output parameters: none.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */
int
dtree_link(dtree_ino *inode IS_UNUSED, dtree_ino *dinode IS_UNUSED,
	   char *name IS_UNUSED, struct creds *cr IS_UNUSED)
{
	/*
	 * since the directory name space is integrally linked to the
	 * inode referred to by that name, hard links are not currently
	 * supportable in this interface.
	 */
	return EPERM;
#if 0
	int		ret = 0;

	/* sanity check */
	if (inode == NULL || dinode == NULL || name == NULL || cr == NULL)
		return EINVAL;

	/* authorize client for link's directory write operation */
	if (dtree_isauth(dinode->dti_parent, dinode, cr, DTREE_OP_WRITE)
			== FALSE)
		return EACCES;

	/* insure name doesn't exist in link's directory */
	if (dtree_find(dinode, name) != NULL)
		return EEXIST;

	/* add new name-space link to the directory */
	/* write LOCK dinode and inode? */
	if (dtree_add_ino(dinode, inode,
#endif
}

/*
 * dtree_symlink: create a symbolic link in the directory tree name
 * space.
 *
 * input parameters:
 *	dinode: the directory to create the symbolic link in.
 *	name: the name of the symbolic link.
 *	link: the path the symbolic link points to.
 *	attr: link inode attributes.
 *	cr: authorization credentials.
 *
 * output parameters: none.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */

/* symlink dtree ops */
static int
dtree_symlink_gattr(struct dtree_ino *inode,  dtree_prvattr *attr)
{
	attr->dta_size = (u_int64_t ) strlen(inode->dti_symlink);
	attr->dta_blocksize = (u_int32_t ) 0;
	attr->dta_rdev = (z_dev_t ) 0;
	attr->dta_blocks = (u_int32_t ) 1;

	return 0;
}

static int
dtree_symlink_sattr(struct dtree_ino *inode IS_UNUSED,
		    dtree_prvattr *attr IS_UNUSED)
{
	/* no private attributes can be set on symlinks */
	return 0;
}

static dtree_ino_ops	dtree_symlink_ops = {
	NULL,
	NULL,
	NULL,
	dtree_symlink_gattr,
	dtree_symlink_sattr,
	NULL
};

int
dtree_symlink(dtree_ino *dinode, char *name, char *link, dtree_attr *attr,
	      struct creds *cr)
{
	int		ret = 0;
	dtree_ino	*lino;		/* new link inode */
	struct timeval	now;

	/* sanity check */
	if (dinode == NULL || name == NULL || link == NULL || attr == NULL ||
	    cr == NULL)
		return EINVAL;

	/* insure directory inode is write locked */
	BUG_CHECK(rw_lkstat(&dinode->dti_lock) == RWLK_WRITE);

	/* authorize client for link directory write operation */
	if (dtree_isauth(dinode, dinode, cr, DTREE_OP_CREATE) == FALSE)
		return EACCES;

	/* insure name doesn't already exist in link directory */
	if (dtree_find(dinode, name) != NULL)
		return EEXIST;

	/* create the new inode */
	if ((ret = dtree_mkino(dinode, name, S_IFREG|DTREE_DEF_FILEMODE,
			       &dtree_symlink_ops, &lino)) != 0)
		return ret;

	/* add symbolic link to directory */
	if ((lino->dti_symlink = m_alloc(strlen(link)+1)) == NULL)
		ret = ENOMEM;
	else {
		(void ) strcpy(lino->dti_symlink, link);

		/* load the requested inode attributes */
		if ((ret = dtree_load_attr(lino, dinode, cr, attr, S_IFLNK,
					   &now)) == 0) {
			if ((ret = dtree_add_inode(dinode, lino, TRUE)) == 0) {
				/* update directories mod time/link count */
				assert(rw_lock(&dinode->dti_alock, RWLK_WRITE,
					       0) == 0);
				dinode->dti_mtime.tv_sec = now.tv_sec;
				dinode->dti_mtime.tv_nsec = now.tv_usec * 1000;
				dinode->dti_ctime.tv_sec = now.tv_sec;
				dinode->dti_ctime.tv_nsec = now.tv_usec * 1000;
				dinode->dti_ref++;
				rw_unlock(&dinode->dti_alock);
			}
		}
	}

	if (ret != 0)
		dtree_ino_destroy(lino);

	return ret;
}

/*
 * dtree_mkdir: create a new directory in the tree.
 *
 * input parameters:
 *	dinode: the parent directory of the new directory.
 *	name: the name of the new directory.
 *	attr: attributes of the new directory.
 *	cr: authorization credentials.
 *	ops: directory inode operations.
 *
 * output parameters:
 *	inop: the resultant dtree inode.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */
int
dtree_mkdir(dtree_ino *dinode, char *name, dtree_attr *attr, struct creds *cr,
	    dtree_ino_ops *ops, dtree_ino **inop)
{
	int		ret = 0;
	dtree_ino	*ino;
	struct timeval	now;

	/* sanity check */
	if (dinode == NULL || name == NULL || attr == NULL || cr == NULL ||
	    ops == NULL || inop == NULL)
		return EINVAL;

	if (strlen(name) > Z_MAXNAMLEN)
		return ENAMETOOLONG;

	/* insure parent directory inode is write locked */
	BUG_CHECK(rw_lkstat(&dinode->dti_lock) == RWLK_WRITE);

	/* authorize client for parent directory write */
	if (dtree_isauth(dinode, dinode, cr, DTREE_OP_CREATE) == FALSE)
		return EACCES;

	/* insure name doesn't already exist in parent directory */
	if (dtree_find(dinode, name) != NULL)
		return EEXIST;

	/* create the new directory inode */
	if ((ret = dtree_mkino(dinode, name, S_IFDIR|DTREE_DEF_DIRMODE, ops,
			       &ino)) != 0)
		return ret;

	ino->dti_ref = ino->dti_link = 2;

	/* link the new directory into the tree */
	if ((ret = dtree_load_attr(ino, dinode, cr, attr, S_IFDIR, &now))
			== 0) {
		/*
		 * call underlying create op: it would be quite strange for
		 * a directory to need creation support beyond that provided
		 * by this interface, but the ability is there....
		 */
		if (ino->dti_ops.dtio_create != NULL)
			ret = (*ino->dti_ops.dtio_create)(ino, &ino->dti_prv);

		if (!ret && (ret = dtree_add_inode(dinode, ino, TRUE)) == 0) {
			/* update dir mod time/link count */
			assert(rw_lock(&dinode->dti_alock, RWLK_WRITE,
				       0) == 0);
			dinode->dti_mtime.tv_sec = now.tv_sec;
			dinode->dti_mtime.tv_nsec = now.tv_usec * 1000;
			dinode->dti_ctime.tv_sec = now.tv_sec;
			dinode->dti_ctime.tv_nsec = now.tv_usec * 1000;
			dinode->dti_ref++;
			dinode->dti_link++;
			rw_unlock(&dinode->dti_alock);
		}
	}

	if (ret == 0)
		*inop = ino;
	else
		dtree_ino_destroy(ino);

	return ret;
}

/*
 * dtree_rmdir: remove a directory from the tree.
 *
 * input parameters:
 *	dinode: parent directory of the directory to be removed.
 *	name: name of the directory to be removed.
 *	inode: inode of the directory to be removed.
 *	cr: authorization credentials.
 *
 * output parameters: none.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */
int
dtree_rmdir(dtree_ino *dinode, char *name, dtree_ino *inode,
	    struct creds *cr)
{
	int		ret = 0;
	struct timeval	now;

	/* sanity check */
	if (dinode == NULL || name == NULL || inode == NULL || cr == NULL)
		return EINVAL;

	/* insure both the parent directory and inode are write locked */
	BUG_CHECK(rw_lkstat(&dinode->dti_lock) == RWLK_WRITE);
	BUG_CHECK(rw_lkstat(&inode->dti_lock) == RWLK_WRITE);

	/* authorize client for parent directory write operation */
	if (dtree_isauth(dinode, inode, cr, DTREE_OP_DESTROY) == FALSE)
		return EACCES;

	/*
	 * insure inode exists in the parent directory and is an empty
	 * directory.
	 */
	if (dtree_find(dinode, name) != inode) {
		ret = ENOENT;
	}
	else if (!S_ISDIR(inode->dti_mode)) {
		ret = ENOTDIR;
	}
	else if (inode->dti_ref > 2) {
		ret = ENOTEMPTY;
	}
	else {
		/*
		 * call underlying destroy op: it would be quite
		 * strange for a directory to need destruction support
		 * beyond that provided by this interface, but the
		 * ability is there....
		 */
		if (inode->dti_ops.dtio_destroy != NULL)
			ret = (*inode->dti_ops.dtio_destroy)(inode);

		/* unlink the directory, as appropriate */
		if (ret == 0 &&
		    (ret = dtree_rem_inode(dinode, inode, TRUE)) == 0) {
			/* update parent dir mod time/link count */
			assert(rw_lock(&dinode->dti_alock, RWLK_WRITE, 0) == 0);
			dinode->dti_ref--;
			dinode->dti_link--;
			(void ) gettimeofday(&now, NULL);
			dinode->dti_mtime.tv_sec = now.tv_sec;
			dinode->dti_mtime.tv_nsec = now.tv_usec * 1000;
			dinode->dti_ctime.tv_sec = now.tv_sec;
			dinode->dti_ctime.tv_nsec = now.tv_usec * 1000;
			rw_unlock(&dinode->dti_alock);
		}
	}

	return ret;
}

/*
 * dtree_adddirent: helper function for dtree_readdir to fill in directory
 * entries in the passed directory entry list.
 *
 * input parameters:
 *	name: the directory entry name to add.
 *	namelen: length of name.
 *	off: offset of entry to add in buffer (not a buffer offset).
 *	inum: inode number associated with name's inode.
 *	entlen: length of entry buffer.
 *
 * output parameters:
 *	dentries: the entry buffer being filled.
 *	entlen: remaining length of entry buffer.
 *	eod: end of directory flag
 *
 * return value:
 */
int
dtree_adddirent(struct directory_entry *entry, char *name, int namelen,
		unsigned int off, z_ino_t inum, int *entlen)
{
	int			i;

	if (*entlen - (int ) directory_record_length(namelen) < 0)
		return 1;	/* end of entry buffer reached */

	entry->de_fileid = (z_ino_t ) inum;
	entry->de_off = (z_off_t ) off;
	entry->de_namlen = (unsigned short ) namelen;

	for (i = 0; i <= namelen; i++)
		entry->de_name[i] = name[i];

	*entlen -= directory_record_length(namelen);

	return 0;
}

/*
 * dtree_readdir: read a directory in the tree.
 *
 * input parameters:
 *	dinode: the directory inode to read.
 *	off: entry offset into directory.
 *	buf: entry buffer.
 *	buflen: length of entry buffer.
 *	cr: authorization credentials.
 *
 * output parameters:
 *	buf: the filled directory entry buffer.
 *	buflen: the returned directory buffer entry length.
 *	eod: end of directory flag.
 *
 * return value: zero (0) on success, an appropriate errno on failure.
 */
int
dtree_readdir(dtree_ino *dinode, z_off_t *off, char *buf, u_int32_t *buflen,
	      int *eod, struct creds *cr)
{
	int		len;		/* running length of directory ents */
	int		ret = 0;	/* return value */
	unsigned int	myoff;		/* local offset */
	dtree_ino	*ino;		/* current directory entry */
	struct directory_entry *dirents = (struct directory_entry *) buf;
	struct timeval	now;		/* set directory access time */

	/* sanity check */
	if (dinode == NULL || off == NULL || buf == NULL || buflen == NULL ||
	    eod == NULL || cr == NULL)
		return EINVAL;

	/* insure directory inode is at least read locked */
	BUG_CHECK(rw_lkstat(&dinode->dti_lock) != RWLK_NONE);

	/* authorize client for directory read operation */
	if (dtree_isauth(dinode->dti_parent, dinode, cr, DTREE_OP_READ)
			== FALSE)
		return EACCES;

	myoff = (unsigned int) *off;
	ino = dinode->dti_dhead;
	len = *buflen;
	switch (myoff) {
	/* add "." and ".." */
	case 0:
		if (dtree_adddirent(dirents, ".", 1, myoff,
				    dinode->dti_inum, &len) != 0) {
			/* directory entries buffer too small! */
			ret = EINVAL;
			break;
		}
		dirents = (struct directory_entry *) ((char *) dirents +
				directory_record_length(1));
		myoff++;

	case 1:
		if (dtree_adddirent(dirents, "..", 2, myoff,
			dinode->dti_parent->dti_inum, &len) != 0) {
			break;
		}
		dirents = (struct directory_entry *) ((char *) dirents +
				directory_record_length(2));
		if (ino != NULL) {
			myoff = ino->dti_inum;	/* start with first dir entry */
		}
		else {
			/* only . and .. in this dir */
			*eod = 1;
			break;
		}

	default:
		/* offset is inode number: find it and continue */
		while (ino != NULL && ino->dti_inum < myoff)
			ino = ino->dti_prev;

		if (ino == NULL) {
			ret = EINVAL;		/* inum not found */
			break;
		}

		while (dtree_adddirent(dirents, ino->dti_name,
				       ino->dti_namelen, myoff, ino->dti_inum,
				       &len) == 0) {
			dirents = (struct directory_entry *) ((char *) dirents
				+ directory_record_length(ino->dti_namelen));

			ino = ino->dti_prev;
			if (ino == NULL) {
				*eod = 1;
				break;
			}
			else
				myoff = ino->dti_inum;
		}
	}

	*buflen = *buflen - len;
	*off = myoff;

	/* update directory access time */
	/* our granularity of time is microseconds */
	assert(rw_lock(&dinode->dti_alock, RWLK_WRITE, 0) == 0);
	(void ) gettimeofday(&now, NULL);
	dinode->dti_atime.tv_sec = now.tv_sec;
	dinode->dti_atime.tv_nsec = now.tv_usec * 1000;
	rw_unlock(&dinode->dti_alock);

	return ret;
}

/*
 * dtree inode locking functions. since the dtree can be interfaced through
 * both the core VFS layer and another consumer (eg, procfs), the dtree
 * layer must hold the lock. these functions must be used in all dtree-
 * based VFS implementations.
 */

int
dtree_ino_lock(dtree_ino *inode, rwty_t lock, int dontwait)
{
	return rw_lock(&inode->dti_lock, lock, dontwait);
}

void
dtree_ino_unlock(dtree_ino *inode)
{
	return rw_unlock(&inode->dti_lock);
}

rwty_t
dtree_ino_lkty(dtree_ino *inode)
{
	return rw_lkstat(&inode->dti_lock);
}
#endif /* SKELFS */
