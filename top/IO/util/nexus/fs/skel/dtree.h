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
#ifndef _DTREE_H_
#define _DTREE_H_
#include <sys/stat.h>

#include "cmn.h"
#include "hash.h"
#include "vfs.h"
#include "vnode.h"
#include "direct.h"

/*
 * Directory tree types and definitions.
 *
 * Author: Scott Morgan, Abba Technologies (smorgan@sandia.gov)
 *
 * $Id: dtree.h,v 1.3 2000/05/23 09:34:41 smorgan Exp $
 *
 * $Log: dtree.h,v $
 * Revision 1.3  2000/05/23 09:34:41  smorgan
 *
 *
 * modifications to support automatic skeletal file system directory creation
 * via a configuration file.
 *
 * Revision 1.2  2000/03/15 02:53:19  lward
 *
 *
 * Ugh! Remove all the RPC and NFS 'isms. Why were they there in the first place?
 * This isn't NFS.
 *
 * Then, fix a bad bug at restart. When generating the file handles, it was
 * done into an nfs_fh (?!?!?), which is 32 bytes. At restart, the upper
 * levels dutifully passed down that the handle was 32 bytes of data. The
 * routine that tore the handle apart decided this was too big and err'd.
 *
 * Now, build the right sized handle and tell the caller it's correct length.
 * Seems to work now.
 *
 * Revision 1.1  2000/02/16 00:49:39  smorgan
 *
 *
 * Addition of the skeletal file system.
 *
 *
 */

#ifndef FALSE
#define FALSE		0
#endif

#ifndef TRUE
#define TRUE		1
#endif

struct dtree;
struct dtree_ino;

/* Directory tree inode attributes. */
typedef struct vstat dtree_attr;

struct dtree_private_attributes {
	u_int64_t		dta_size;
	u_int32_t		dta_blocksize;
	z_dev_t			dta_rdev;
	u_int32_t		dta_blocks;
};

typedef struct dtree_private_attributes		dtree_prvattr;

/* Directory tree inode operations */
struct dtree_inode_ops {
	/* i/o ops */
	int			(*dtio_create)(struct dtree_ino *, void **);
	int			(*dtio_read)(struct dtree_ino *, z_off_t,
					     char *, u_int32_t *);
	int			(*dtio_write)(struct dtree_ino *, z_off_t,
					      char *, u_int32_t *);
	int			(*dtio_gattr)(struct dtree_ino *,
					      dtree_prvattr *);
	int			(*dtio_sattr)(struct dtree_ino *,
					      dtree_prvattr *);
	int			(*dtio_destroy)(struct dtree_ino *);
};

typedef struct dtree_inode_ops		dtree_ino_ops;

enum dtree_op {
	DTREE_OP_READ = 0,
	DTREE_OP_WRITE,
	DTREE_OP_EXEC,			/* directory traversal */
	DTREE_OP_SATTR,
	DTREE_OP_CREATE,
	DTREE_OP_DESTROY
};

struct dtree_ino {
	/*
	 * the tree-link structure, lifted from Linux procfs.
	 *
	 * presumably, the name space used in file systems based
	 * on this abstraction will be small. if larger name spaces
	 * are required, a more efficient core storage mechanism will
	 * be required.
	 */
	rwlock_t		dti_lock;
	unsigned short		dti_namelen;
	char			*dti_name;

	struct dtree_ino	*dti_parent,
				*dti_prev,
				*dti_next,
				*dti_child,
				*dti_dhead;

	z_dev_t			dti_dev;		/* device number */
	z_ino_t			dti_inum;		/* inode number */
	z_nlink_t		dti_ref;		/* reference count */
	z_ino_t			dti_dlastino;		/* last dir inum */

	struct dtree		*dti_root;		/* dtree root */

	/* attributes */
	rwlock_t		dti_alock;		/* attribute lock */
	z_mode_t		dti_mode;		/* permissions & type */
	z_nlink_t		dti_link;		/* link count */
	z_uid_t			dti_uid;		/* owner */
	z_gid_t			dti_gid;		/* group */
	struct timespec		dti_atime;		/* access time */
	struct timespec		dti_mtime;		/* modify time */
	struct timespec		dti_ctime;		/* inode modify time */
	char			*dti_symlink;		/* symlink */
	void			*dti_prv;		/* private inode data */
	void			*dti_fh;		/* file handle */

	/* dtree inode operations */
	struct dtree_inode_ops	dti_ops;
};

typedef struct dtree_ino	dtree_ino;

struct dtree {
	mutex_t			dt_lock;
	z_dev_t			dt_fsid;	/* file system identifier */
	dtree_ino		*dt_root;	/* the root of all evil */
	hash_t			*dt_inos;	/* inode hash table */
	unsigned long		dt_lastdino;	/* last directory inode base */
	unsigned int		dt_ninos;	/* total number of inodes */
};

typedef struct dtree		dtree;

#define DTREE_HASH_SIZE		67
#define DTREE_DEF_DIRMODE	(S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH)
#define DTREE_DEF_FILEMODE	DEFFILEMODE
#define DTREE_DEF_GID		0
#define DTREE_DIRINUM_INCR	0x00010000	/* inum base incr for dirs */
#define DTREE_DIRINUM_MAX	0x7fff0000	/* max base dir inum */
#define DTREE_INUM_MASK		0x0000ffff	/* non-dir inode numbers */


/* Directory tree operations */
extern int		dtree_create_root(z_dev_t, dtree_ino_ops *, dtree **);
extern int		dtree_statfs(dtree *, struct fsstats *);
extern void		dtree_destroy(dtree *);

/* Directory tree inode operations */
extern int		dtree_getattr(dtree_ino *, dtree_attr *);
extern int		dtree_setattr(dtree_ino *, dtree_attr *,
					      struct creds *);

extern int		dtree_lookup(dtree_ino *, char *, struct creds *,
				      dtree_ino **);
extern int		dtree_readlink(dtree_ino *, char *, u_int32_t *);
extern int		dtree_read(dtree_ino *, z_off_t, char *, u_int32_t *,
				   struct creds *);
extern int		dtree_write(dtree_ino *, z_off_t, char *, u_int32_t *,
				    struct creds *);
extern int		dtree_create(dtree_ino *, char *, dtree_attr *,
				     struct creds *, dtree_ino_ops *,
				     dtree_ino **);
extern int		dtree_remove(dtree_ino *, char *, dtree_ino *,
				     struct creds *);
extern int		dtree_rename(dtree_ino *, char *, dtree_ino *,
				     dtree_ino *, char *, struct creds *);
extern int		dtree_link(dtree_ino *, dtree_ino *, char *,
				   struct creds *);
extern int		dtree_symlink(dtree_ino *, char *, char *,
				      dtree_attr *, struct creds *);
extern int		dtree_mkdir(dtree_ino *, char *, dtree_attr *,
				    struct creds *, dtree_ino_ops *,
				    dtree_ino **);
extern int		dtree_rmdir(dtree_ino *, char *, dtree_ino *,
				    struct creds *);
extern int		dtree_readdir(dtree_ino *, z_off_t *, char *,
				      u_int32_t *, int *, struct creds *);
extern int		dtree_findi(dtree *, z_ino_t, rwty_t, dtree_ino **);
extern void		dtree_ino_destroy(void *);
extern int		dtree_ino_lock(dtree_ino *, rwty_t, int);
extern void		dtree_ino_unlock(dtree_ino *);
extern rwty_t		dtree_ino_lkty(dtree_ino *);

#endif /* _DTREE_H_ */
