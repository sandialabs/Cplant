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
#ifndef _PROCFS_H_
#define _PROCFS_H_

#include "cmn.h"
#include "vfs.h"
#include "vnode.h"
#include "direct.h"
#include "dtree.h"

/*
 * Process file system types and definitions
 *
 * Most of the process file system is implemented in the underlying
 * dtree layer.
 *
 * IDENTIFY("$Id: procfs.h,v 1.2 2000/10/19 16:10:48 lward Exp $");
 */

struct procfs {
	mutex_t		procfs_lock;		/* vnode instance lock */

	dtree		*procfs_root;		/* the directory structure */
	z_dev_t		procfs_dev;		/* device number */
	struct vfs	*procfs_vfs;		/* the VFS layer above skel */
};

typedef dtree_ino_ops	procfs_ino_ops;
typedef dtree_ino	procfs_ino;

#define V2PROCFS(v)	((struct procfs *) (v)->v_vfsp->vfs_private)
#define V2PROC(v)	((dtree_ino *) (v)->v_private)

#define VFS2PROCFS(vfs)	((struct procfs *) (vfs)->vfs_private)

#define PROCFS_DEV	0x00000001
#define PROCFS_DELIM	'/'

#define PROCFS_FHSIZE	(sizeof(z_dev_t) + sizeof(z_ino_t))

extern struct vfsops	procfs_vfsops;
extern struct vops	procfs_vops;
extern dtree_ino_ops	procfs_dirino_ops;

/* procfs start */
extern int		procfs_startsvc(void *, size_t, struct creds *);

/* procfs global helper function(s) */
extern int		procfs_findi(struct procfs *, dtree_ino *, rwty_t,
				     struct vnode **);
extern int		_procfs_register(const char *, procfs_ino_ops *,
					const struct vstat *, z_mode_t,
					void *);

#define procfs_register(name,ops,attr) \
			_procfs_register(name, ops, attr, VS_IFREG, NULL)
#define procfs_register_dir(name,ops,attr) \
			_procfs_register(name, ops, attr, VS_IFDIR, NULL)
#define procfs_register_symlink(name,link,ops,attr) \
			_procfs_register(name, ops, attr, VS_IFLNK, link)

/* generic inode helper functions */
extern int		procfs_ino_create(dtree_ino *, void **);
extern int		procfs_ino_gattr(dtree_ino *, dtree_prvattr *);
extern int		procfs_ino_destroy(dtree_ino *);

#define procfs_ino_sattr	NULL

/* default procfs entry attributes, by type */
extern const struct vstat	procfs_defattr_file;
extern const struct vstat	procfs_defattr_dir;
extern const struct vstat	procfs_defattr_link;

#endif /* _PROCFS_H_ */

