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
#include "cmn.h"
#include "vfs.h"
#include "vnode.h"
#include "direct.h"
#include "rpcsvc/mount.h"
#include "rpcsvc/nfs_prot.h"
#include "client.h"

/*
 * NFS file system types and definitions.
 *
 * $Id: nfsfs.h,v 1.5 2001/07/18 18:57:32 rklundt Exp $
 */

/*
 * All servers encode some information that is common in their handles.
 * Enabling the following compiles in code that attempts to eliminate
 * it in a way that allows it to be recovered.
 */
#define ENHANCE_NFS_HANDLE_COMPRESSION

/*
 * RPC calls time out. For NFS, they generally should not as we want to
 * wait for the server to come back. By default, then, set it to something
 * large.
 */
#define NFS_CALL_TIMEOUT	(15552000)		/* 180 days */

/*
 * NFS file system private information.
 */
struct nfsfs {
	const char *nfsfs_host;				/* remote host name */
	const char *nfsfs_path;				/* remote path */
	struct server *nfsfs_server;			/* client context */
	nfs_fh	nfsfs_rootfh;				/* root file handle */
	struct vfs *nfsfs_vfsp;				/* ptr to common part */
	struct timeval nfsfs_rpctimo;			/* RPC call timeout */
	time_t nfsfs_attrtimeo;				/* attrs life-time */
};

/*
 * Return ptr to vfs private data.
 */
#define VFS2NFS(vfsp)	((struct nfsfs *)(vfsp)->vfs_private)

struct nfsino {
	z_ino_t	nfsi_ino;				/* inum */
	nfs_fh	nfsi_fh;				/* handle */
#ifdef ENHANCE_NFS_HANDLE_COMPRESSION
	nfs_fh	nfsi_vfh;				/* virtual handle */
#endif
	struct vnode *nfsi_vp;				/* ptr to vnode */
	rwlock_t nfsi_alock;				/* attrs lock */
	time_t nfsi_atstamp;				/* attrs time-stamp */
	fattr nfsi_fattr;				/* cached attrs */
};

/*
 * Return ptr to vnode private data.
 */
#define V2NFS(vp)	((struct nfsino *)(vp)->v_private)

#define NFSATSTAMP_NONE	((time_t )-1)

/*
 * Invalidate cached attributes.
 *
 * NB: Assumes nfsi_alock is held for writing by the caller.
 */
#define INVALIDATE_ATTRCACHE(nfsip) \
	do { \
		assert(rw_lock(&(nfsip)->nfsi_alock, RWLK_WRITE, 0) == 0); \
		(nfsip)->nfsi_atstamp = NFSATSTAMP_NONE; \
		rw_unlock(&(nfsip)->nfsi_alock); \
	} while (0)

#ifndef ENHANCE_NFS_HANDLE_COMPRESSION
/*
 * Initialize a new NFS inode record.
 */
#define NFSI_INIT(nfsip, nfsfsp, ino, fhp) \
	do { \
		(nfsip)->nfsi_ino = (ino); \
		(void )memcpy(&(nfsip)->nfsi_fh, (fhp), sizeof(nfs_fh)); \
		(nfsip)->nfsi_vp = NULL; \
		rw_init(&(nfsip)->nfsi_alock); \
		INVALIDATE_ATTRCACHE(nfsip); \
		(void )memset(&(nfsip)->nfsi_fattr, 0, sizeof(fattr)); \
	} while (0)
#else /* defined(ENHANCE_NFS_HANDLE_COMPRESSION) */
/*
 * Initialize a new NFS inode record.
 */
#define NFSI_INIT(nfsip, nfsfsp, ino, fhp) \
	do { \
		(nfsip)->nfsi_ino = (ino); \
		(void )memcpy(&(nfsip)->nfsi_fh, (fhp), sizeof(nfs_fh)); \
		nfs_fhtovfh((nfsfsp), (fhp), &(nfsip)->nfsi_vfh); \
		(nfsip)->nfsi_vp = NULL; \
		rw_init(&(nfsip)->nfsi_alock); \
		INVALIDATE_ATTRCACHE(nfsip); \
		(void )memset(&(nfsip)->nfsi_fattr, 0, sizeof(fattr)); \
	} while (0)

/*
 * Create virtual file handle given ptr to NFS fs and the real file handle.
 */
#define nfs_fhtovfh(nfsfsp, fhp, vfhp) \
	(nfs_permutefh(&(nfsfsp)->nfsfs_rootfh, (fhp), (vfhp)))

/*
 * Create real file handle given ptr to NFS fs and the virtual file handle.
 */
#define nfs_vfhtofh(nfsfsp, vfhp, fhp) \
	(nfs_permutefh(&(nfsfsp)->nfsfs_rootfh, (vfhp), (fhp)))

#endif /* !defined(ENHANCE_NFS_HANDLE_COMPRESSION) */

/*
 * Make an NFS call to the server of the given FS.
 */
#define nfs_call(nfsfsp, f, argp, results, crp) \
	nfs_docall((nfsfsp)->nfsfs_server, \
		   (f), \
		   (argp), \
		   (results), \
		   &(nfsfsp)->nfsfs_rpctimo, \
		   (crp))

/*
 * Ceate pseudo-inode number.
 */
#if Z_INO_T_IS_64
#define NFS_MKPSI(fhp, ino)	nfs_mki64((fhp), (ino))
#else
#define NFS_MKPSI(fhp, ino)	(ino)
#endif

extern struct vfsops nfs_vfsops;
extern struct vops nfs_vops;

extern int nfs_findi(struct nfsfs *,
		     z_ino_t,
		     nfs_fh *,
		     fattr *,
		     rwty_t,
		     struct nfsino **);
extern void nfs_reclaimi(struct nfsino *);
extern int nfs_docall(struct server *,
		      bool_t (*)(void *, void *, CLIENT *),
		      void *, void *,
		      const struct timeval *,
		      struct creds *);
extern int nfs_simple_call(const char *,
			   u_long, u_long,
			   const char *,
			   bool_t (*)(void *, void *, CLIENT *),
			   void *, void *,
			   struct timeval *,
			   struct creds *);
extern int nfs_errxlate(nfsstat);
extern int nfs_igetattr(struct nfsino *, fattr *);
extern void nfs_attrxlate(fattr *, z_dev_t, z_ino_t, struct vstat *);
extern int nfs_sattrload(struct vstat *, sattr *);
#ifdef ENHANCE_NFS_HANDLE_COMPRESSION
extern void nfs_permutefh(nfs_fh *, nfs_fh *, nfs_fh *);
#endif
extern void nfs_cacheattrs(struct nfsino *, fattr *);
#if Z_INO_T_IS_64
extern z_ino_t nfs_mki64(nfs_fh *, u_int32_t);
#endif
