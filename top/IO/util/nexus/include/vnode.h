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
#ifndef _VNODE_H_
#define _VNODE_H_
/*
 * Virtual nodes (vnodes) support.
 *
 * $Id: vnode.h,v 1.2 2000/03/03 22:59:27 lward Exp $
 */
#include "queue.h"
#include "rwlk.h"
#include "ztypes.h"
#include "creds.h"

/*
 * Maximum length of a path name.
 */
#ifndef Z_MAXPATHLEN
#define Z_MAXPATHLEN	1024
#endif

/*
 * Maximum length of a file name.
 */
#ifndef Z_MAXNAMLEN
#define Z_MAXNAMLEN	255
#endif

/*
 * Recursion limit for symbolic links.
 */
#ifndef Z_MAXSYMLINKS
#define	Z_MAXSYMLINKS	255
#endif

/*
 * I-number zero is illegal always. Used to indicate "no i-number".
 */
#define NOINO		0

struct vstat;
struct vfs;
struct vops;
struct vnode;
struct vnhndl;

/*
 * Vnode operations.
 *
 * Most operations expect some sort of, appropriate, lock to be held by the
 * caller. There are two exceptions though; read and write. These two
 * expect that there is no lock held by the caller. They are expected
 * to accomplish any required locking themselves.
 *
 * One more locking note: The lookup routine should expect a read lock on the
 * directory vnode at entry. The upper level routines expect that this lock is
 * dropped by lookup and that the lock requested by the caller applies to the
 * vnode target. Lookup should drop the directory lock always. Whether the
 * operation was successful or not. The only, weird, exception to that would
 * be if the directory vnode, passed, turned out to be the target as well. In
 * that case, the requested lock should be in place at return.
 *
 * This semantic for lookup is necessary so that the upper level routines don't
 * accidentally create a deadly embrace. Please don't defeat it by holding
 * the directory lock inside lookup while taking a lock on another vnode.
 * If you need to acquire simultaneous locks, use v_multilock().
 */
struct vops {
	int	(*vop_getattr)(struct vnode *, struct vstat *);
	int	(*vop_setattr)(struct vnode *, struct vstat *, struct creds *);
	int	(*vop_lookup)(struct vnode *, const char *, struct creds *,
			      rwty_t, struct vnode **);
	int	(*vop_readlink)(struct vnode *, char *, u_int32_t *);
	int	(*vop_read)(struct vnode *, z_off_t, char *, u_int32_t *,
			    struct creds *);
	int	(*vop_write)(struct vnode *, z_off_t, char *, u_int32_t *,
			     struct creds *);
	int	(*vop_create)(struct vnode *, const char *, struct vstat *,
			      struct creds *, rwty_t, struct vnode **);
	int	(*vop_remove)(struct vnode *, const char *,
			      struct vnode *, struct creds *);
	int	(*vop_rename)(struct vnode *, const char *, struct vnode *,
			      struct vnode *, const char *,
			      struct creds *);
	int	(*vop_link)(struct vnode *, struct vnode *, const char *,
			    struct creds *);
	int	(*vop_symlink)(struct vnode *, const char *, const char *,
			       struct vstat *, struct creds *);
	int	(*vop_mkdir)(struct vnode *, const char *, struct vstat *,
			     struct creds *, rwty_t, struct vnode **);
	int	(*vop_rmdir)(struct vnode *, const char *, struct vnode *,
			     struct creds *);
	int	(*vop_readdir)(struct vnode *, z_off_t *, char *, u_int32_t *,
			       int *, struct creds *);
	int	(*vop_handle)(struct vnode *, struct vnhndl *);
	int	(*vop_lock)(struct vnode *, rwty_t, int);
	void	(*vop_unlock)(struct vnode *);
	rwty_t	(*vop_lkty)(struct vnode *);
	void	(*vop_reclaim)(struct vnode *);
};

#define VOP_GETATTR(vp, vstbuf) \
	((*(vp)->v_ops.vop_getattr)((vp), (vstbuf)))
#define VOP_SETATTR(vp, vstbuf, crp) \
	((*(vp)->v_ops.vop_setattr)((vp), (vstbuf), (crp)))
#define VOP_LOOKUP(dvp, name, crp, lkf, vpp) \
	((*(dvp)->v_ops.vop_lookup)((dvp), (name), (crp), (lkf), (vpp)))
#define VOP_READLINK(vp, buf, buflen) \
	((*(vp)->v_ops.vop_readlink)((vp), (buf), (buflen)))
#define VOP_READ(vp, off, buf, buflen, crp) \
	((*(vp)->v_ops.vop_read)((vp), (off), (buf), (buflen), (crp)))
#define VOP_WRITE(vp, off, buf, buflen, crp) \
	((*(vp)->v_ops.vop_write)((vp), (off), (buf), (buflen), (crp)))
#define VOP_CREATE(dvp, name, vstbuf, crp, lkf, vpp) \
	((*(dvp)->v_ops.vop_create)((dvp), (name), (vstbuf), (crp), \
				   (lkf), (vpp)))
#define VOP_REMOVE(dvp, name, vp, crp) \
	((*(dvp)->v_ops.vop_remove)((dvp), (name), (vp), (crp)))
#define VOP_RENAME(sdvp, sname, vp, ddvp, dname, crp) \
	((*(sdvp)->v_ops.vop_rename)((sdvp), (sname), (vp), (ddvp), (dname), \
				     (crp)))
#define VOP_LINK(vp, ddvp, name, crp) \
	((*(ddvp)->v_ops.vop_link)((vp), (ddvp), (name), (crp)))
#define VOP_SYMLINK(ddvp, dname, path, vstbp, crp) \
	((*(ddvp)->v_ops.vop_symlink)((ddvp), (dname), (path), (vstbp), (crp)))
#define VOP_MKDIR(dvp, name, vstbuf, crp, lkf, vpp) \
	((*(dvp)->v_ops.vop_mkdir)((dvp), (name), (vstbuf), (crp), \
				   (lkf), (vpp)))
#define VOP_RMDIR(dvp, name, vp, crp) \
	((*(dvp)->v_ops.vop_rmdir)((dvp), (name), (vp), (crp)))
#define VOP_READDIR(dvp, offp, buf, buflen, eofp, crp) \
	((*(dvp)->v_ops.vop_readdir)((dvp), (offp), (buf), (buflen), \
				     (eofp), (crp)))
#define VOP_HANDLE(vp, vnhndlp) \
	((*(vp)->v_ops.vop_handle)((vp), (vnhndlp)))
#define VOP_LOCK(vp, lkty, dontwait) \
	((*(vp)->v_ops.vop_lock)((vp), (lkty), (dontwait)))
#define VOP_UNLOCK(vp) \
	((*(vp)->v_ops.vop_unlock)(vp))
#define VOP_LKTY(vp) \
	((*(vp)->v_ops.vop_lkty)(vp))
#define VOP_RECLAIM(vp) \
	((*(vp)->v_ops.vop_reclaim)(vp))

struct vncb;

/*
 * Virtual node (vnode) record. One allocated per active file in
 * the system.
 */
struct vnode {
	/*
	 * Some flags are only safe to change/examine when VXLOCK or
	 * v_mutex is held. Others are safe to examine/change when
	 * an operations lock is held. See the individual flags (below).
	 */
	unsigned v_flags;				/* flags (see below) */
	/*
	 * The below are safe to access/modify only when current thread is
	 * either the first (and only) holder of the VXLOCK flag or the
	 * vnode mutex (v_mutex field below) is held.
	 */
	unsigned v_ref;					/* soft ref count */

	/*
	 * Access to the below are safe with at least one reference held or
	 * with the VXLOCK flag set by the current thread.
	 */
	struct vops v_ops;				/* operations */
	z_ino_t	v_ino;					/* i-number */
	struct vfs *v_vfsp;				/* ptr to VFS record */
	void	*v_private;				/* FS private data */
	rwlock_t v_rwlock;				/* ops lock */

	/*
	 * The below are safe to access/modify only when the
	 * parent vfs mutex (see vfs_mutex field in vfs.h) is held.
	 */
	TAILQ_ENTRY(vnode) v_fslink;			/* VFS vnodes link */

	/*
	 * The below are safe to access/modify only when the
	 * vnodes package mutex (see vfs_subr.c) is held.
	 */
	TAILQ_ENTRY(vnode) v_link;			/* vnodes list link */

	/*
	 * The below are safe to access/modify only when the
	 * appropriate operations lock is held.
	 */
	struct vnode *v_mountpoint;			/* this root covers */
	struct vnode *v_cover;				/* covered by... */

	mutex_t	v_mutex;				/* inter-lock */
	cond_t	v_cond;					/* record waiters */
	LIST_HEAD(, vncb) v_callbacks;			/* reclaim callbacks */
};

/*
 * Vnode flags.
 *
 * Those marked with a `*' are safe to examine/modify only when VXLOCK
 * or v_mutex is held. The others may be safely examined and changed when
 * an appropriate operations lock is held.
 */
#define VXLOCK		0x0001				/* record locked (*) */
#define VXWANT		0x0002				/* has waiters (*) */

/*
 * Associate a vnode.
 */
#define v_setup(vp, flags, vfsp, ino, ops, private) \
	do { \
		(vp)->v_flags = (flags); \
		(vp)->v_ops = *(ops); \
		(vp)->v_vfsp = (vfsp); \
		(vp)->v_private = (private); \
		(vp)->v_ino = (ino); \
		(vp)->v_mountpoint = NULL; \
		(vp)->v_cover = NULL; \
		LIST_INIT(&(vp)->v_callbacks); \
	} while (0)

/*
 * Compare two vnodes.
 */
#define v_cmp(v1p, v2p) \
	((v1p) == (v2p) \
	   ? 0 \
	   : (v1p)->v_ino < (v2p)->v_ino \
	       ? -1 \
	       : (v1p)->v_ino > (v2p)->v_ino \
		   ? 1 \
		   : 0)

/*
 * Vnode locked?
 */
#define v_islocked(vp) \
	VOP_LKTY(vp)

/*
 * Vnode dead?
 */
#define v_isdead(vp) \
	((vp)->v_ino == NOINO && (vp)->v_vfsp == NULL)

/*
 * File system specific vnode handles.
 */
struct vnhndl {
	size_t	vnhnd_len;				/* handle length */
	const char *vnhnd_data;				/* handle data */
};

/*
 * The vnode maintains a callback list to be run when the vnode is
 * reclaimed. The list is run after the vnode is locked above but before
 * it is cleaned.
 *
 * The callback list is always run beginning with the most recently registered
 * callback and ending with the oldest.
 */
struct vncb {
	void	(*vncb_f)(struct vnode *);		/* function to call */
	LIST_ENTRY(vncb) vncb_next;			/* link to next */
};

/*
 * Field types in vstat structure.
 */
#define VST_TYP_MODE		z_mode_t
#define VST_TYP_NLINK		z_nlink_t
#define VST_TYP_UID		z_uid_t
#define VST_TYP_GID		z_gid_t
#define VST_TYP_SIZE		u_int64_t
#define VST_TYP_BLOCKSIZE	u_int32_t
#define VST_TYP_RDEV		z_dev_t
#define VST_TYP_BLOCKS		u_int32_t
#define VST_TYP_FSID		z_dev_t
#define VST_TYP_FILEID		z_ino_t
#define VST_TYP_TIME_SEC	long
#define VST_TYP_TIME_NSEC	long

/*
 * Vnode attributes.
 */
struct vstat {
	VST_TYP_MODE vst_mode;				/* protection */
	VST_TYP_NLINK vst_nlink;			/* # hard links */
	VST_TYP_UID vst_uid;				/* owner user id */
	VST_TYP_GID vst_gid;				/* owner group id */
	VST_TYP_SIZE vst_size;				/* file size in bytes */
	VST_TYP_BLOCKSIZE vst_blocksize;		/* preferred blk size */
	VST_TYP_RDEV vst_rdev;				/* special device # */
	VST_TYP_BLOCKS vst_blocks;			/* blocks allocated */
	VST_TYP_FSID vst_fsid;				/* device # */
	VST_TYP_FILEID vst_fileid;			/* inode # */
	struct timespec vst_atime;			/* last access */
	struct timespec vst_mtime;			/* last modification */
	struct timespec vst_ctime;			/* last change */
};

/*
 * File type bits in vst_mode.
 */
#define VS_IFMT		0170000				/* file type bits */
#define VS_IFLNK	0120000				/* symlink */
#define VS_IFREG	0100000				/* regular file */
#define VS_IFDIR	0040000				/* directory */

/*
 * File type tests.
 */
#define	VS_ISLNK(m)	((m) & VS_IFLNK)
#define	VS_ISREG(m)	((m) & VS_IFREG)
#define	VS_ISDIR(m)	((m) & VS_IFDIR)

/*
 * Permission bits in vst_mode.
 */
#define VS_IRWXU	0000700				/* owner: rwx */
#define VS_IRUSR	0000400				/* owner: r */
#define VS_IWUSR	0000200				/* owner: w */
#define VS_IXUSR	0000100				/* owner: x */

/*
 * Does vstat try to set attribute?
 */
#define VST_DOESSET(attr, typ) \
	((attr) != (typ )-1)

/*
 * Are any user-immutable attributes set?
 */
#define VST_VALID(vstp) \
	(!(VST_DOESSET((vstp)->vst_nlink, VST_TYP_NLINK) || \
	   VST_DOESSET((vstp)->vst_blocksize, VST_TYP_BLOCKSIZE) || \
	   VST_DOESSET((vstp)->vst_rdev, VST_TYP_RDEV) || \
	   VST_DOESSET((vstp)->vst_blocks, VST_TYP_BLOCKS) || \
	   VST_DOESSET((vstp)->vst_fsid, VST_TYP_FSID) || \
	   VST_DOESSET((vstp)->vst_fileid, VST_TYP_FILEID) || \
	   VST_DOESSET((vstp)->vst_ctime.tv_sec, VST_TYP_TIME_SEC) || \
	   VST_DOESSET((vstp)->vst_ctime.tv_nsec, VST_TYP_TIME_NSEC)))

/*
 * Caller trys to change named attribute?
 */
#define VST_DOESSET_MODE(vstp) \
	VST_DOESSET((vstp)->vst_mode, VST_TYP_MODE)
#define VST_DOESSET_UID(vstp) \
	VST_DOESSET((vstp)->vst_uid, VST_TYP_UID)
#define VST_DOESSET_GID(vstp) \
	VST_DOESSET((vstp)->vst_gid, VST_TYP_GID)
#define VST_DOESSET_SIZE(vstp) \
	VST_DOESSET((vstp)->vst_size, VST_TYP_SIZE)
#define VST_DOESSET_ATIME_SEC(vstp) \
	VST_DOESSET((vstp)->vst_atime.tv_sec, VST_TYP_TIME_SEC)
#define VST_DOESSET_ATIME_NSEC(vstp) \
	VST_DOESSET((vstp)->vst_atime.tv_nsec, VST_TYP_TIME_NSEC)
#define VST_DOESSET_MTIME_SEC(vstp) \
	VST_DOESSET((vstp)->vst_mtime.tv_sec, VST_TYP_TIME_SEC)
#define VST_DOESSET_MTIME_NSEC(vstp) \
	VST_DOESSET((vstp)->vst_mtime.tv_nsec, VST_TYP_TIME_NSEC)

/*
 * Name to inode has *alot* of arguments. Some are value/result. It's
 * just a problem. At least we can bundle the arguments.
 */
struct namei_data {
	unsigned nd_flags;				/* flags (see below) */
	struct vnode *nd_parent;			/* parent dir */
	const char *nd_path;				/* path to translate */
	struct vnode *nd_vp;				/* result */
	rwty_t	nd_lkf;					/* result lock */
	struct creds *nd_crp;				/* credentials */
};

/*
 * Flags in nd_flags, above.
 */
#define ND_NOFOLLOW		0x01			/* ! follow symlinks */
#define ND_WANTPARENT		0x02			/* return parent too */

/*
 * Lock operation argument.
 */
struct vlkop {
	struct vnode *vlkop_vp;				/* vnode */
	rwty_t	vlkop_lkf;				/* lock type */
};

extern unsigned vnodes_max;

extern struct vnode *rootvp;

extern struct vops dead_vops;

extern int namei(struct namei_data *);
extern struct vnode * v_new(struct vfs *, z_ino_t, struct vops *, void *);
extern void v_ref(struct vnode *);
extern void v_rele(struct vnode *);
extern int v_get(struct vnode *, rwty_t, unsigned);
extern void v_put(struct vnode *);
extern void v_gone(struct vnode *);
extern int v_generic_op_lock(struct vnode *, rwty_t, int);
extern void v_generic_op_unlock(struct vnode *);
extern rwty_t v_generic_op_lkty(struct vnode *);
extern int v_generic_op_nosys(void);
extern int v_multilock(struct vlkop *, size_t);
extern int v_lookup(struct vnode *, const char *, struct creds *,
		    rwty_t, struct vnode **);
extern int v_atreclaim(struct vnode *, void (*)(struct vnode *));
#endif  /* defined(_VNODE_H_) */
