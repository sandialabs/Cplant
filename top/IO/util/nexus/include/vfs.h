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
#ifndef _VFS_H_
#define _VFS_H_
/*
 * Virtual file system (vfs) support.
 *
 * $Id: vfs.h,v 1.1 1999/11/29 19:45:46 lward Exp $
 */
#include "queue.h"
#include "rwlk.h"
#include "ztypes.h"
#include "creds.h"

/*
 * Device number zero is illegal always. Used to indicate "no device".
 */
#define NODEV		0

/*
 * File system statistics. One for one with NFS V2 statfsres data.
 */
struct fsstats {
	u_int32_t fsstats_tsize;			/* preferred xfer len */
	u_int32_t fsstats_bsize;			/* FS blksiz (bytes) */
	u_int32_t fsstats_blocks;			/* total blocks */
	u_int32_t fsstats_bfree;			/* free blocks */
	u_int32_t fsstats_bavail;			/* allocatable blocks */
};

struct vfs;
struct vnode;
struct vnhndl;

/*
 * Virtual file system operations.
 */
struct vfsops {
	void	(*vfsop_unmount)(struct vfs *);
	int	(*vfsop_getroot)(struct vfs *, rwty_t, struct vnode **);
	int	(*vfsop_statfs)(struct vfs *, struct fsstats *);
	int	(*vfsop_getv)(struct vfs *, z_ino_t, struct vnode **, unsigned);
	int	(*vfsop_getv_by_handle)(struct vfs *,
					struct vnhndl *,
					struct vnode **, unsigned);
	int	(*vfsop_lock)(struct vfs *, rwty_t, int);
	void	(*vfsop_unlock)(struct vfs *);
	rwty_t	(*vfsop_lkty)(struct vfs *);
};

#define VFSOP_UNMOUNT(vfsp) \
	((*(vfsp)->vfs_ops.vfsop_unmount)(vfsp))
#define VFSOP_GETROOT(vfsp, lkty, vpp) \
	((*(vfsp)->vfs_ops.vfsop_getroot)((vfsp), (lkty), (vpp)))
#define VFSOP_STATFS(vfsp, fsstatbuf) \
	((*(vfsp)->vfs_ops.vfsop_statfs)((vfsp), (fsstatbuf)))
#define VFSOP_GETV(vfsp, inum, vpp, lkf) \
	((*(vfsp)->vfs_ops.vfsop_getv)((vfsp), (inum), (vpp), (lkf)))
#define VFSOP_GETV_BY_HANDLE(vfsp, vnhndlp, vpp, lkf) \
	((*(vfsp)->vfs_ops.vfsop_getv_by_handle)((vfsp), \
						 (vnhndlp), \
						 (vpp), (lkf)))
#define VFSOP_LOCK(vfsp, lkty, dontwait) \
	((*(vfsp)->vfs_ops.vfsop_lock)((vfsp), (lkty), (dontwait)))
#define VFSOP_UNLOCK(vfsp) \
	((*(vfsp)->vfs_ops.vfsop_unlock)(vfsp))
#define VFSOP_LKTY(vfsp) \
	((*(vfsp)->vfs_ops.vfsop_lkty)(vfsp))

/*
 * Lists of vnodes.
 */
TAILQ_HEAD(vnode_list, vnode);

/*
 * Virtual file system record.
 */
struct vfs {
	/*
	 * The following may be accessed/modified only when the calling
	 * thread either holds the first (and only) VFSXLOCK or the
	 * record mutex (vfs_mutex field) below.
	 */
	unsigned vfs_flags;				/* flags (see below) */
	unsigned vfs_ref;				/* soft ref count */
	struct vnode_list vfs_vnodes;			/* active vnodes */

	/*
	 * The following may be accessed any time a reference is held.
	 */
	struct vfsops vfs_ops;				/* operations */
	void	*vfs_private;				/* FS private data */
	z_dev_t	vfs_dev;				/* device num */
	rwlock_t vfs_rwlock;				/* generic FS lock */

	/*
	 * The following may be accessed/modified only when the
	 * vfs package mutex (vfsmutex in vfs_subr.c) is held.
	 */
	TAILQ_ENTRY(vfs) vfs_link;			/* mounts list link */

	mutex_t	vfs_mutex;				/* inter-lock */
	cond_t	vfs_cond;				/* record waiters */
};

/*
 * Vnode flags.
 */
#define VFSXLOCK	0x1				/* record locked */
#define VFSXWANT	0x2				/* has waiters */

/*
 * Associate a vfs record.
 */
#define vfs_setup(vfsp, flags, dev, ops, private) \
	do { \
		(vfsp)->vfs_flags = (flags); \
		(vfsp)->vfs_dev = (dev); \
		(vfsp)->vfs_ops = *(ops); \
		(vfsp)->vfs_private = (private); \
	} while (0)

/*
 * VFS locked?
 */
#define vfs_islocked(vfsp) \
	VFSOP_LKTY(vfsp)

/*
 * VFS configuration table entry.
 */
struct vfsconf {
	const char *vfsconf_type;			/* FS type name */
	int	(*vfsconf_mount)(const void *,
				 size_t,
				 struct creds *,
				 struct vfs **);
};

extern struct vfsconf vfsconftbl[];

extern struct vfsops dead_vfsops;

extern void vfs_init(void);
extern int mount_root(const char *, const void *, size_t, struct creds *);
extern int umount_root(void);
extern int mount_fs(const char *, const char *,
		    const void *, size_t, struct creds *);
extern int getvfs(z_dev_t, struct vfs **);
extern struct vfs * vfs_new(z_dev_t, struct vfsops *, void *);
extern void vfs_ref(struct vfs *);
extern void vfs_rele(struct vfs *);
extern int vfs_get(struct vfs *, rwty_t, unsigned);
extern void vfs_put(struct vfs *);
extern int vfs_mount(const char *, const void *, size_t,
		     struct creds *, struct vfs **);
extern int vfs_umount(struct vfs *, struct creds *);
extern int vfs_vfind(z_dev_t, z_ino_t, rwty_t, int, struct vnode **);
extern int vfs_generic_op_lock(struct vfs *, rwty_t, int);
extern void vfs_generic_op_unlock(struct vfs *);
extern rwty_t vfs_generic_op_lkty(struct vfs *);

#if 0
extern void nfs_initialize_server(void);
extern void mount_initialize_server(void);
#endif
extern int service_create(u_long, u_long, void (*)(), int, u_short);
#endif  /* defined(_VFS_H_) */
