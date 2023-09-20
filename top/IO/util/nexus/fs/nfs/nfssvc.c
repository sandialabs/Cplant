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
#include <stdio.h>					/* sprintf */
#include <stdlib.h>					/* malloc/free */
#include <string.h>
#if !defined(notdef)
#include <sys/param.h>
#endif

#include "cmn.h"
#include "rpc/rpc.h"
#include "rpc/pmap_clnt.h"
#include "creds.h"
#include "vnode.h"
#include "vfs.h"
#include "nfssvc.h"
#include "mountsvc.h"

IDENTIFY("$Id: nfssvc.c,v 1.7.2.1 2001/10/19 20:29:43 rklundt Exp $");

/*
 * NFS server side support.
 */

/*
 * The compression algorithm used is assumed. From that, it is
 * known that the original data could not have been larger than
 * 249 bytes.
 */
#define MAX_HANDLE_SIZE	249

/*
 * Flags in NFS handles.
 */
#define NFSID_F_REGEN	0x1				/* can regen i-node */

/*
 * We keep a cache of constructed handles. This is done because we want
 * to minimize the information we add and, when compressed, it is very
 * difficult to deal with them.
 */
struct handle_map_entry {
	nfs_fh	fh;					/* client handle data */
	struct vnode *vp;				/* ptr to vnode */
	struct handle_map_entry **epp;			/* ptr to reverse map */
};

static struct handle_map_entry *handle_map = NULL;	/* fh -> vno map */
static size_t handle_map_len = 0;			/* length of above */
static struct handle_map_entry **vnode_map = NULL;	/* vno -> entry map */

static mutex_t mutex = MUTEX_INITIALIZER;		/* package mutex */

static unsigned himask;					/* hi bit in unsigned */

#if !defined(notdef)
#if NBBY != 8
#error COPY64 asumes 8-bit bytes.
#endif

/*
 * Copy 64 bits, assuming a byte is 8 bits.
 *
 * We wouldn't need this if gcc/egcs 2.91.66 didn't screw up memcpy()
 * when the source was a u_int64_t.
 */
#define COPY64(dstp, srcp) \
	do { \
		unsigned char *_my_dstp = (unsigned char *)dstp; \
		unsigned char *_my_srcp = (unsigned char *)srcp; \
 \
		*_my_dstp++ = *_my_srcp++; \
		*_my_dstp++ = *_my_srcp++; \
		*_my_dstp++ = *_my_srcp++; \
		*_my_dstp++ = *_my_srcp++; \
		*_my_dstp++ = *_my_srcp++; \
		*_my_dstp++ = *_my_srcp++; \
		*_my_dstp++ = *_my_srcp++; \
		*_my_dstp++ = *_my_srcp++; \
	} while (0)
#endif

/*
 * NFS server start routine.
 */
int
nfs_startsvc(const void *arg IS_UNUSED,
	     size_t arglen IS_UNUSED,
	     struct creds *crp)
{
	int	err;
	static int started = 0;
	extern void nfs_program_2(struct svc_req, SVCXPRT *);
	extern void mountprog_1(struct svc_req, SVCXPRT *);

	if (!is_suser(crp))
		return EPERM;

	mutex_lock(&mutex);
	if (started) {
		mutex_unlock(&mutex);
		return EBUSY;
	}
	started = 1;
	mutex_unlock(&mutex);

	nfs_initialize_server();

	/*
	 * Start both NFS and mount services.
	 */
	(void )pmap_unset(NFS_PROGRAM, NFS_VERSION);
	err =
	    service_create(NFS_PROGRAM, NFS_VERSION,
			   nfs_program_2,
			   IPPROTO_UDP,
			   NFS_PORT);
	if (err) {
		LOG(LOG_ERR,
		    "couldn't create (NFS_PROGRAM, NFS_VERSION, udp) service");
		return err;
	}
	return 0;
}

/*
 * Initialize for service.
 */
void
nfs_service_init()
{
	size_t	indx;
	static int is_initialized = 0;

	/*
	 * The ENFS protocol uses much of NFS. For instance, it calls
	 * NFS service init. Since this can now be done twice, a check
	 * is added to prevent the work from being done twice. That could
	 * be disastrous.
	 */
	mutex_lock(&mutex);
	if (is_initialized) {
		mutex_unlock(&mutex);
		return;
	}
	is_initialized = 1;
	mutex_unlock(&mutex);

	/*
	 * Allocate the handle map.
	 */
	handle_map_len = 2 * vnodes_max - 1;
	handle_map = m_alloc(handle_map_len * sizeof(struct handle_map_entry));
	if (handle_map == NULL)
		panic("nfs_service_init: can't create handle map");

	/*
	 * Allocate the vnode map.
	 */
	vnode_map = m_alloc(handle_map_len * sizeof(struct handle_map_entry *));
	if (vnode_map == NULL)
		panic("nfs_service_init: can't create vnode map");

	/*
	 * Initialize both the maps.
	 */
	for (indx = 0; indx < handle_map_len; indx++) {
		handle_map[indx].vp = NULL;
		handle_map[indx].epp = NULL;
		vnode_map[indx] = NULL;
	}

	/*
	 * Calculate hi bit in unsigned mask for hash routines below.
	 */
	himask = 1;
	while (himask << 1)
		himask <<= 1;
}

/*
 * Map system error number to NFS status.
 */
nfsstat
nfs_errstat(int err)
{
	unsigned indx;
	struct errmap {
		int	e;
		nfsstat	status;
	};
	struct errmap *mp;
	static struct errmap map[] = {
		{ 0,		NFS_OK },
		{ EPERM,	NFSERR_PERM },
		{ ENOENT,	NFSERR_NOENT },
		{ EIO,		NFSERR_IO },
		{ ENXIO,	NFSERR_NXIO },
		{ EACCES,	NFSERR_ACCES },
		{ EEXIST,	NFSERR_EXIST },
		{ ENODEV,	NFSERR_NODEV },
		{ ENOTDIR,	NFSERR_NOTDIR },
		{ EISDIR,	NFSERR_ISDIR },
		{ EFBIG,	NFSERR_FBIG },
		{ ENOSPC,	NFSERR_NOSPC },
		{ EROFS,	NFSERR_ROFS },
		{ ENAMETOOLONG,	NFSERR_NAMETOOLONG },
		{ ENOTEMPTY,	NFSERR_NOTEMPTY },
		{ EDQUOT,	NFSERR_DQUOT },
		{ ESTALE,	NFSERR_STALE }
	};

	for (indx = 0, mp = map;
	     indx < sizeof(map) / sizeof(map[0]);
	     indx++, mp++)
		if (err == mp->e)
			break;
	if (indx >= sizeof(map) / sizeof(map[0])) {
		LOG(LOG_WARNING, "nfs_errstat: unmapped error # %d", err);
		return NFSERR_IO;
	}

	return mp->status;
}

/*
 * Return hash value of passed data.
 */
static INLINE unsigned
hash(const unsigned char *cp, size_t size)
{
	unsigned acc;
	unsigned hibit;

	acc = 0;
	while (size--) {
		hibit = acc & himask;
		acc <<= 1;
		if (hibit)
			acc |= 1;
		acc ^= *cp++;
	}

	return acc;
}

/*
 * Hash client file handle data.
 */
static INLINE unsigned
hash_handle(nfs_fh *fhp)
{
	const unsigned char *cp;

	cp = fhp->data;
	return hash(cp, NFS_FHSIZE);
}

/*
 * Hash a core address.
 */
static INLINE unsigned
hash_address(void *p)
{

	return hash((const unsigned char *)&p, sizeof(void *));
}

/*
 * Lookup the previously constructed handle map entry for a given vnode.
 */
static struct handle_map_entry *
lookup_handle(struct vnode *vp)
{
	unsigned hval;
	struct handle_map_entry *ep;
	size_t	indx;

	hval = hash_address(vp);

	ep = NULL;

	mutex_lock(&mutex);

	for (indx = 0; indx < handle_map_len; indx++) {
		ep = vnode_map[hval++ % handle_map_len];
		if (ep != NULL && ep->vp == vp)
			break;
	}

	mutex_unlock(&mutex);

	if (indx >= handle_map_len)
		ep = NULL;

	return ep;
}

/*
 * Remove handle and vnode mappings for the given vnode.
 *
 * Note: This is a callback during vnode reclaim.
 */
static void
remove_handle(struct vnode *vp)
{
	struct handle_map_entry *ep;

	ep = lookup_handle(vp);
#ifndef NDEBUG
	assert(ep != NULL);
#endif

	mutex_lock(&mutex);

	ep->vp = NULL;
	*ep->epp = NULL;
	ep->epp = NULL;

	mutex_unlock(&mutex);
}

/*
 * Cache a constructed file handle.
 */
static void
cache_handle(struct vnode *vp, nfs_fh *fhp)
{
	unsigned hval;
	size_t	indx;
	struct handle_map_entry *ep = NULL;		/* init or GCC whines */

	assert(handle_map_len);

	hval = hash_handle(fhp);

	mutex_lock(&mutex);

	/*
	 * Put the handle into the handle to vnode map.
	 */
	for (indx = 0; indx < handle_map_len; indx++) {
		ep = &handle_map[hval++ % handle_map_len];
		if (ep->vp == NULL || ep->vp == vp)
			break;
	}
	if (ep->vp == NULL) {
#ifdef INSTRUMENT
		mutex_lock(&nfssrv_stats_mutex);
		nfssrv_stats.handle_inserts++;
		if (indx)
			nfssrv_stats.handle_collisions++;
		mutex_unlock(&nfssrv_stats_mutex);
#endif
		assert(ep->vp == NULL && ep->epp == NULL);
		ep->vp = vp;
		ep->fh = *fhp;
	} else if (indx < handle_map_len &&
		   ep->vp == vp &&
		   memcmp(&ep->fh, fhp, sizeof(ep->fh)) == 0 &&
		   ep->epp != NULL &&
		   *ep->epp == ep) {
		/*
		 * Hey! They're mapping a vnode that is already in the
		 * map. This can happen if another thread did the deed
		 * first. Just pretend everything went well.
		 */
		mutex_unlock(&mutex);
		return;
	} else if (indx >= handle_map_len)
		panic("cache_handle: handle map full");
	else
		panic("cache_handle: mapping mapped vnode");

	/*
	 * Put the handle map entry into the vnode to handle map.
	 */
	hval = hash_address(vp);
	for (indx = 0; indx < handle_map_len; indx++) {
		ep->epp = &vnode_map[hval++ % handle_map_len];
		if (*ep->epp == NULL) {
			*ep->epp = ep;
			break;
		}
	}
	if (*ep->epp != ep)
		panic("cache_handle: vnode map full");

	mutex_unlock(&mutex);

	v_atreclaim(vp, remove_handle);

	return;
}

/*
 * Manufacture file handle.
 *
 * File handles are packed and have the following format:
 *	bytes		meaning
 *	-----		-------
 *	0 -  7		Device (network byte order)
 *	8    		flags
 *	9 - 31		data (if NFSID_F_REGEN set)
 *	9 - 16		Inode number (if !NFSID_F_REGEN)
 *
 * I would have liked to use a union for the variant tagged data but the
 * GCC compiler wants to align it like a structure and I loathe considering
 * the "packed" attribute.
 */
void
nfs_mkfh(struct vnode *vp, nfs_fh *fhp)
{
	struct handle_map_entry *ep;
	u_int64_t ui64;
#if defined(Z_DEV_T_IS_64) || defined(Z_INO_T_IS_64)
	u_int32_t ui32;
#endif
	int	err;
	struct vnhndl handle;
	size_t	datalen;

	/*
	 * If we've seen this handle before, it's in the vnode to handle
	 * map. Look for it to save effort.
	 */
	ep = lookup_handle(vp);
	if (ep != NULL) {
		*fhp = ep->fh;
		return;
	}

	if (vp->v_vfsp->vfs_dev == NODEV)
		panic("nfs_mkfh: no device set");
	(void )memset(fhp, 0, sizeof(nfs_fh));
	ui64 = (u_int32_t )htonl((u_int32_t )vp->v_vfsp->vfs_dev);
#if defined(Z_DEV_T_IS_64)
	ui32 = (u_int32_t )(vp->v_vfsp->vfs_dev >> 32);
	ui64 |= (u_int64_t )htonl(ui32) << 32;
#endif
#ifdef notdef
	(void )memcpy(fhp, (char *)&ui64, sizeof(u_int64_t));
#else
	COPY64(fhp, &ui64);
#endif
	err = VOP_HANDLE(vp, &handle);
	if (err) {
		if (err != ENOSYS)
			LOG(LOG_ERR,
			    ("nfs_mkfh: can't get handle of "
			     FMT_Z_DEV_T "." FMT_Z_INO_T),
			    vp->v_vfsp->vfs_dev,
			    vp->v_ino);
	}
#if defined(DEBUG) && NBBY <= 8
	if (!err && NFS_SVCDBG_CHECK(3)) {
		size_t	i;
		char	*buf;

		/*
		 * Most all of this assumes that bytes are 8 bits.
		 */
		buf = m_alloc(4 * handle.vnhnd_len + 1);
		if (buf != NULL) {
			for (i = 0; i < handle.vnhnd_len; i++)
				(void )sprintf(&buf[i << 1],
					       "%02x",
					       handle.vnhnd_data[i]);
			buf[i << 1] = '\0';
			LOG_DBG(1,
				("nfs_mkfh: " FMT_Z_DEV_T "." FMT_Z_INO_T
				 " compressing %s"),
				vp->v_vfsp->vfs_dev,
				vp->v_ino,
				buf);
			free(buf);
		}
	}
#endif
	if (!err) {
#ifdef INSTRUMENT
		mutex_lock(&nfssrv_stats_mutex);
		nfssrv_stats.encodes++;
		mutex_unlock(&nfssrv_stats_mutex);
#endif
		datalen = NFS_FHSIZE - sizeof(u_int64_t) - 1;
		(void )memset((char *)fhp + sizeof(u_int64_t) + 1, 0, datalen);
		err =
		    imcompress(handle.vnhnd_data,
			       handle.vnhnd_len,
			       (char *)fhp + sizeof(u_int64_t) + 1,
			       &datalen);
		if (!err) {
			((unsigned char *)fhp)[sizeof(u_int64_t)] =
			    NFSID_F_REGEN;
		} else {
#ifdef INSTRUMENT
			mutex_lock(&nfssrv_stats_mutex);
			nfssrv_stats.encodes_missed++;
			mutex_unlock(&nfssrv_stats_mutex);
#endif
			LOG_DBG(NFS_SVCDBG_CHECK(2),
				("nfs_mkfh: can't compress handle of "
				 FMT_Z_DEV_T "." FMT_Z_INO_T "; error %d"),
				vp->v_vfsp->vfs_dev,
				vp->v_ino,
				err);
			/*
			 * The compressor wrote the buffer. Clear it again.
			 */
			(void )memset((char *)fhp + sizeof(u_int64_t) + 1,
				      0,
				      NFS_FHSIZE - sizeof(u_int64_t) - 1);
		}
	}
	if (err) {
		/*
		 * Can't get the real handle in there. Well, we'll stuff
		 * in the i-num and hope it doesn't get flushed from
		 * the vnode cache.
		 */
		((unsigned char *)fhp)[sizeof(u_int64_t)] = 0;
		if (vp->v_ino == NOINO)
			panic("nfs_mkfh: no ino set");
		ui64 = (u_int32_t )htonl((u_int32_t )vp->v_ino);
#if defined(Z_INO_T_IS_64)
		ui32 = (u_int32_t )(vp->v_ino >> 32);
		ui64 |= (u_int64_t )htonl(ui32) << 32;
#endif
#ifdef notdef
		(void )memcpy((char *)fhp + sizeof(u_int64_t) + 1,
			      &ui64,
			      sizeof(u_int64_t));
#else
		COPY64((char *)fhp + sizeof(u_int64_t) + 1, &ui64);
#endif
		err = 0;
	}


	if (!err)
		cache_handle(vp, fhp);

	return;
}

/*
 * Lookup a vnode given handle data from the client.
 */
static int
lookup_vnode(nfs_fh *fhp, struct vnode **vpp, rwty_t lkf)
{
	unsigned hval;
	struct vnode *vp;
	size_t	indx;
	volatile struct handle_map_entry *ep = NULL;	/* init or GCC whines */
	int	err;

	vp = NULL;
	do {
		hval = hash_handle(fhp);

		mutex_lock(&mutex);
		for (indx = 0; indx < handle_map_len; indx++) {
			ep = &handle_map[hval++ % handle_map_len];
			if (ep->vp != NULL &&
			    memcmp(fhp->data,
				   (char *)ep->fh.data,
				   NFS_FHSIZE) == 0) {
				vp = ep->vp;
				break;
			}
		}
		mutex_unlock(&mutex);

		if (vp == NULL) {
			err = ENOENT;
			break;
		}

		err = v_get(vp, lkf, 0);
		if (!err) {
			int	loser;

			/*
			 * Couldn't do the v_get inside the atomic section
			 * above without risking deadlock. We just had a
			 * race then. Did we win?
			 */
			loser = 0;
			mutex_lock(&mutex);
			if (ep->vp == NULL ||
			    ep->vp != vp ||
			    memcmp(fhp->data,
				   (char *)ep->fh.data,
				   NFS_FHSIZE) != 0)
				loser = 1;
			mutex_unlock(&mutex);
			if (loser) {
				if (lkf != RWLK_NONE) VOP_UNLOCK(vp);
				v_rele(vp);
				vp = NULL;
			}
		}
	} while (!err && vp == NULL);
#ifdef INSTRUMENT
	mutex_lock(&nfssrv_stats_mutex);
	nfssrv_stats.handle_lookups++;
	if (vp == NULL)
		nfssrv_stats.handle_misses++;
	mutex_unlock(&nfssrv_stats_mutex);
#endif
#ifdef DEBUG
	if (NFS_SVCDBG_CHECK(99)) {
		char	*s, *cp;
		size_t	indx;
		unsigned char uc;

		s = malloc((sizeof(*fhp) << 1) + 1);
		if (s != NULL) {
			cp = s;
			for (indx = 0; indx < sizeof(*fhp); indx++) {
				uc = ((unsigned char *)fhp)[indx];
				*cp++ = "0123456789ABCDEF"[(uc >> 4) & 0xf];
				*cp++ = "0123456789ABCDEF"[(uc     ) & 0xf];
			}
			*cp = '\0';
			LOG(LOG_DEBUG,
			    "lookup_vnode: %s err %d vp %s set",
			    s, err, vp != NULL ? "is" : "not");
			free(s);
		} else
			LOG(LOG_DEBUG, "lookup_vnode: can't alloc string buf");
	}
#endif
	if (err)
		return err;
	if (vp == NULL)
		return ENOENT;
	*vpp = vp;
	return 0;
}

/*
 * Return ptr to vnode, given a regeneratable file handle.
 */
static int
regeneratable_getv(nfs_fh *fhp, struct vnode **vpp, rwty_t lkf)
{
	struct vnhndl handle;
	int	err;
	struct vfs *vfsp;
	u_int64_t ui64;
#if defined(Z_DEV_T_IS_64)
	u_int32_t ui32;
#endif
	z_dev_t	dev;

	assert(((unsigned char *)fhp)[sizeof(u_int64_t)] & NFSID_F_REGEN);

	handle.vnhnd_len = MAX_HANDLE_SIZE;
	handle.vnhnd_data = m_alloc(handle.vnhnd_len);
	if (handle.vnhnd_data == NULL)
		return ENOMEM;
	err =
	    imuncompress(((char *)fhp) + sizeof(u_int64_t) + 1,
			 NFS_FHSIZE - sizeof(u_int64_t) - 1,
			 (char *)handle.vnhnd_data,
			 &handle.vnhnd_len);

	vfsp = NULL;
	if (!err) {
#ifdef notdef
		(void )memcpy(&ui64, fhp, sizeof(u_int64_t));
#else
		COPY64(&ui64, fhp);
#endif
		dev = (u_int32_t )ntohl((u_int32_t )ui64);
#if defined(Z_DEV_T_IS_64)
		ui32 = ui64 >> 32;
		dev |= (u_int64_t )ntohl(ui32) << 32;
#endif
		err = getvfs(dev, &vfsp);
	}
	if (!err)
		err = VFSOP_GETV_BY_HANDLE(vfsp, &handle, vpp, lkf);
	if (vfsp != NULL)
		vfs_rele(vfsp);
	free((char *)handle.vnhnd_data);

	if (!err)
		cache_handle(*vpp, fhp);

#ifdef DEBUG
	if (NFS_SVCDBG_CHECK(99)) {
		char	*s, *cp;
		size_t	indx;
		unsigned char uc;

		s = malloc((sizeof(*fhp) << 1) + 1);
		if (s != NULL) {
			cp = s;
			for (indx = 0; indx < sizeof(*fhp); indx++) {
				uc = ((unsigned char *)fhp)[indx];
				*cp++ = "0123456789ABCDEF"[(uc >> 4) & 0xf];
				*cp++ = "0123456789ABCDEF"[(uc     ) & 0xf];
			}
			*cp = '\0';
			LOG(LOG_DEBUG, "regeneratable_getv: %s err %d", s, err);
			free(s);
		} else
			LOG(LOG_DEBUG,
			    "regeneratable_getv: can't alloc string buf");
	}
#endif
	return err;
}

/*
 * NFS version of getv() using the file handle as identifier.
 */
int
nfs_getv(nfs_fh *fhp, struct vnode **vpp, rwty_t lkf)
{
	u_int64_t ui64;
#if defined(Z_DEV_T_IS_64) || defined(Z_INO_T_IS_64)
	u_int32_t ui32;
#endif
	z_dev_t	dev;
	z_ino_t	ino;
	int	err;
	struct vfs *vfsp;

	/*
	 * Try the handle to vnode map first.
	 */
	if (lookup_vnode(fhp, vpp, lkf) == 0)
		return 0;

	if (((unsigned char *)fhp)[sizeof(u_int64_t)] & NFSID_F_REGEN) {
		/*
		 * A handle with compressed information present. It needs
		 * special attentions.
		 */
		return regeneratable_getv(fhp, vpp, lkf);
	}

#ifdef DEBUG
	if (NFS_SVCDBG_CHECK(99)) {
		char	*s, *cp;
		size_t	indx;
		unsigned char uc;

		s = malloc((sizeof(*fhp) << 1) + 1);
		if (s != NULL) {
			cp = s;
			for (indx = 0; indx < sizeof(*fhp); indx++) {
				uc = ((unsigned char *)fhp)[indx];
				*cp++ = "0123456789ABCDEF"[(uc >> 4) & 0xf];
				*cp++ = "0123456789ABCDEF"[(uc     ) & 0xf];
			}
			*cp = '\0';
			LOG(LOG_DEBUG, "nfs_getv: last resort for %s", s);
			free(s);
		} else
			LOG(LOG_DEBUG, "nfs_getv: can't alloc string buf");
	}
#endif

#ifdef notdef
	(void )memcpy(&ui64, fhp, sizeof(u_int64_t));
#else
	COPY64(&ui64, fhp);
#endif
	dev = (u_int32_t )ntohl((u_int32_t )ui64);
#if defined(Z_DEV_T_IS_64)
	ui32 = ui64 >> 32;
	dev |= (u_int64_t )ntohl(ui32) << 32;
#endif
#ifdef notdef
	(void )memcpy(&ui64,
		      (char *)fhp + sizeof(u_int64_t) + 1,
		      sizeof(u_int64_t));
#else
	COPY64(&ui64, (char *)fhp + sizeof(u_int64_t) + 1);
#endif
	ino = (u_int32_t )ntohl((u_int32_t )ui64);
#if defined(Z_INO_T_IS_64)
	ui32 = ui64 >> 32;
	ino |= (u_int64_t )ntohl(ui32) << 32;
#endif

	if (dev == NODEV || ino == NOINO) {
		LOG(LOG_ERR, "nfs_getv: corrupt handle");
		return ENOENT;
	}

	/*
	 * Try the vnode cache.
	 */
	err = vfs_vfind(dev, ino, lkf, 0, vpp);
	if (!err)
		return 0;

	/*
	 * If it isn't in the cache, we have to ask the underlying file system
	 * to activate it.
	 */
	err = getvfs(dev, &vfsp);
	if (!err) {
		err = VFSOP_GETV(vfsp, ino, vpp, lkf);
		/*
		 * If the file system tells us it can't find it, force
		 * the client to look it up again -- it's probably changed.
		 * However, it could be that the file system is lazy.
		 */
		if (err == ENOENT)
			err = ESTALE;
		vfs_rele(vfsp);
	}

	if (!err)
		cache_handle(*vpp, fhp);

	return err;
}

/*
 * Return NFS fattr's for a given vnode.
 *
 * This server really only supports the propagation of directories, regular
 * files and symbolic links. All others are mapped to a bad type to enforce
 * that.
 */
int
nfs_getattrs(struct vnode *vp, fattr *fap)
{
	int	err;
	struct vstat vstbuf;
	static ftype ftymap[] = {
		0,
		0,
		0,
		0,
		NFDIR,
		0,
		0,
		0,
		NFREG,
		0,
		NFLNK
	};

	err = VOP_GETATTR(vp, &vstbuf);
	if (err)
		return err;

	fap->type = ftymap[(vstbuf.vst_mode & VS_IFMT) >> 12];
	fap->mode = vstbuf.vst_mode;
	fap->nlink = vstbuf.vst_nlink;
	fap->uid = vstbuf.vst_uid;
	fap->gid = vstbuf.vst_gid;
	fap->size = vstbuf.vst_size;
	fap->blocksize = vstbuf.vst_blocksize;
	fap->rdev = vstbuf.vst_rdev;
	fap->blocks = vstbuf.vst_blocks;
#ifndef AGGREGATE_IDENTIFIER
	fap->fsid = vstbuf.vst_fsid;
	fap->fileid = vstbuf.vst_fileid;
#else
	fap->fsid = AGGREGATE_IDENTIFIER;
	fap->fileid = vstbuf.vst_fsid + vstbuf.vst_fileid;
#endif
	fap->atime.seconds = vstbuf.vst_atime.tv_sec;
	fap->atime.useconds = (unsigned ) vstbuf.vst_atime.tv_nsec / 1000;
	fap->mtime.seconds = vstbuf.vst_mtime.tv_sec;
	fap->mtime.useconds = (unsigned ) vstbuf.vst_mtime.tv_nsec / 1000;
	fap->ctime.seconds = vstbuf.vst_ctime.tv_sec;
	fap->ctime.useconds = (unsigned ) vstbuf.vst_ctime.tv_nsec / 1000;

	if (fap->nlink != vstbuf.vst_nlink ||
	    fap->uid != vstbuf.vst_uid ||
	    fap->gid != vstbuf.vst_gid ||
	    fap->size != vstbuf.vst_size ||
	    fap->blocksize != vstbuf.vst_blocksize ||
	    fap->rdev != vstbuf.vst_rdev ||
	    fap->blocks != vstbuf.vst_blocks ||
#ifndef AGGREGATE_IDENTIFIER
	    fap->fsid != vstbuf.vst_fsid ||
	    fap->fileid != vstbuf.vst_fileid ||
#endif
	    fap->atime.seconds != (u_int )vstbuf.vst_atime.tv_sec ||
	    fap->atime.useconds != (u_int )vstbuf.vst_atime.tv_nsec / 1000 ||
	    fap->mtime.seconds != (u_int )vstbuf.vst_mtime.tv_sec ||
	    fap->mtime.useconds != (u_int )vstbuf.vst_mtime.tv_nsec / 1000 ||
	    fap->ctime.seconds != (u_int )vstbuf.vst_ctime.tv_sec ||
	    fap->ctime.useconds != (u_int )vstbuf.vst_ctime.tv_nsec / 1000)
		err = EINVAL;

	return err;
}

/*
 * Trnaslate NFS sattr to equivalent vstat.
 */
void
nfs_sattrxlate(sattr *sap, struct vstat *vstp)
{

	vstp->vst_mode = -1;
	vstp->vst_nlink = -1;
	vstp->vst_uid = -1;
	vstp->vst_gid = -1;
	vstp->vst_size = -1;
	vstp->vst_blocksize = -1;
	vstp->vst_rdev = -1;
	vstp->vst_blocks = -1;
	vstp->vst_fsid = -1;
	vstp->vst_fileid = -1;
	vstp->vst_atime.tv_sec = -1;
	vstp->vst_atime.tv_nsec = -1;
	vstp->vst_mtime.tv_sec = -1;
	vstp->vst_mtime.tv_nsec = -1;
	vstp->vst_ctime.tv_sec = -1;
	vstp->vst_ctime.tv_nsec = -1;

	if ((int )sap->mode != -1)
		vstp->vst_mode = sap->mode;
        if ((int )sap->uid != -1)
		vstp->vst_uid = sap->uid;
        if ((int )sap->gid != -1)
		vstp->vst_gid = sap->gid;
        if ((int )sap->size != -1)
		vstp->vst_size = sap->size;
        if ((int )sap->atime.seconds != -1)
		vstp->vst_atime.tv_sec = sap->atime.seconds;
        if ((int )sap->atime.useconds != -1) {
		vstp->vst_atime.tv_nsec = sap->atime.useconds;
		vstp->vst_atime.tv_nsec *= 1000;
	}
        if ((int )sap->mtime.seconds != -1)
		vstp->vst_mtime.tv_sec = sap->mtime.seconds;
        if ((int )sap->mtime.useconds != -1) {
		vstp->vst_mtime.tv_nsec = sap->mtime.useconds;
		vstp->vst_mtime.tv_nsec *= 1000;
	}
}

/*
 * Set vnode attributes from NFS equivalent.
 */
int
nfs_setattrs(struct vnode *vp, sattr *sap, struct creds *crp)
{
	struct vstat vstbuf;

	nfs_sattrxlate(sap, &vstbuf);
	return VOP_SETATTR(vp, &vstbuf, crp);
}
