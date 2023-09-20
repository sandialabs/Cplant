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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmn.h"
#include "vfs.h"
#include "vnode.h"

IDENTIFY("$Id: vfs_subr.c,v 1.7 2001/07/18 18:57:46 rklundt Exp $");

#ifndef VNODES_MAX
#define VNODES_MAX	512
#endif

#ifdef DEBUG
int	vnode_debug = 0;				/* vnode debugging */

/*
 * Check if debug level is high enough.
 */
#define VNODE_DBG_CHECK(lvl)	(vnode_debug >= (lvl))
#endif

static mutex_t mount_mutex = MUTEX_INITIALIZER;		/* {un}mount lock  */

static mutex_t vfsmutex = MUTEX_INITIALIZER;		/* vfs mutex */
TAILQ_HEAD(vfs_list, vfs);
static struct vfs_list mounts =				/* mounted fs list */
    TAILQ_HEAD_INITIALIZER(mounts);
static struct vfs_list free_mountrecs =			/* free records */
    TAILQ_HEAD_INITIALIZER(free_mountrecs);

unsigned vnodes_max = VNODES_MAX;			/* max vnodes */
static mutex_t vmutex = MUTEX_INITIALIZER;		/* vnodes mutex */
static unsigned nvnodes = 0;				/* cur vnode count */
static struct vnode_list vnodes =			/* list of all vnodes */
    TAILQ_HEAD_INITIALIZER(vnodes);

struct vnode *rootvp = NULL;				/* name space root */

/*
 * A vnode cache entry.
 */
struct vnode_cache_entry {
	z_dev_t	vce_dev;				/* device ID */
	z_ino_t	vce_ino;				/* i-num */
	struct vnode *vce_vp;				/* ptr to vnode */
};

/*
 * Set up vcache entry.
 */
#define vcache_entry_setup(vcep, dev, ino, vp) \
	do { \
		(vcep)->vce_dev = (dev); \
		(vcep)->vce_ino = (ino); \
		(vcep)->vce_vp = (vp); \
	} while (0)

enum vcacheop { VCACHE_LOOKUP, VCACHE_INSERT };

#ifdef INSTRUMENT
static struct {
	unsigned long lookups;
	unsigned long hits;
} vcache_instrument = {0, 0};
#endif

static struct vnode_cache_entry *vcache = NULL;		/* vnode cache */
static size_t vcache_len = 0;				/* vcache tbl len */
static rwlock_t vcache_lock = RW_INITIALIZER;		/* vcache r/w lock */

/*
 * Hash a dev-ino pair.
 */
#define vfs_hash(dev, ino) \
	(((dev) << 2) ^ (ino))

/*
 * Wait for vfs record to stop changing.
 *
 * The caller should hold the vfs record mutex.
 */
#define vfs_wait(vfsp) \
	do { \
		(vfsp)->vfs_flags |= VFSXWANT; \
		cond_wait(&(vfsp)->vfs_cond, &(vfsp)->vfs_mutex); \
	} while (0)

/*
 * Wake vfs record waiters.
 *
 * The caller should hold the vfs record mutex.
 */
#define vfs_wakeup(vfsp) \
	do { \
		if ((vfsp)->vfs_flags & VFSXWANT) { \
			cond_broadcast(&(vfsp)->vfs_cond); \
			(vfsp)->vfs_flags &= ~VFSXWANT; \
		} \
	} while (0)

/*
 * Wait for vnode to stop changing.
 *
 * The caller should hold the vnode mutex.
 */
#define v_wait(vp) \
	do { \
		(vp)->v_flags |= VXWANT; \
		cond_wait(&(vp)->v_cond, &(vp)->v_mutex); \
	} while (0)

/*
 * Wake vnode waiters.
 *
 * The caller should hold the vnode mutex.
 */
#define v_wakeup(vp) \
	do { \
		if ((vp)->v_flags & VXWANT) { \
			cond_broadcast(&(vp)->v_cond); \
			(vp)->v_flags &= ~VXWANT; \
		} \
	} while (0)

/*
 * Initialize vfs package.
 *
 * NB: Not thread-safe.
 */
void
vfs_init(void)
{
	unsigned indx;
	struct vnode_cache_entry *vcep;

	vcache_len = vnodes_max + (vnodes_max >> 1) + 1;
	vcache = m_alloc(vcache_len * sizeof(struct vnode_cache_entry));
	if (vcache == NULL)
		panic("vfs_init: can't alloc vcache");
	for (indx = 0, vcep = vcache; indx < vcache_len; indx++, vcep++)
		vcache_entry_setup(vcep, NODEV, NOINO, NULL);
}

/*
 * Allocate new vfs record and perform at alloc initialization.
 */
static struct vfs *
vfs_alloc(void)
{
	struct vfs *vfsp;

	vfsp = m_alloc(sizeof(struct vfs));
	if (vfsp == NULL) {
		LOG(LOG_WARNING, "vfs_alloc: m_alloc failed");
		return NULL;
	}
	(void )memset(vfsp, 0, sizeof(struct vfs));
	mutex_init(&vfsp->vfs_mutex);
	cond_init(&vfsp->vfs_cond);
	rw_init(&vfsp->vfs_rwlock);
	vfs_setup(vfsp, 0, NODEV, &dead_vfsops, NULL);
	TAILQ_INIT(&vfsp->vfs_vnodes);
	return vfsp;
}

struct vfs *
vfs_new(z_dev_t dev, struct vfsops *ops, void *private)
{
	struct vfs *vfsp;
	int	err;

	mutex_lock(&vfsmutex);

	/*
	 * Try to grab a free one.
	 */
	vfsp = TAILQ_FIRST(&free_mountrecs);
	if (vfsp != NULL && mutex_trylock(&vfsp->vfs_mutex) == 0) {
		if (vfsp->vfs_ref) {
			mutex_unlock(&vfsp->vfs_mutex);
			vfsp = NULL;
		} else
			TAILQ_REMOVE(&free_mountrecs, vfsp, vfs_link);
	}

	/*
	 * If none free, allocate a new one.
	 */
	if (vfsp == NULL) {
		vfsp = vfs_alloc();
		if (vfsp == NULL) {
			LOG(LOG_ERR, "vfs_new: can't alloc");
			return NULL;
		}
		vfs_setup(vfsp, 0, NODEV, &dead_vfsops, NULL);
	} else
		mutex_unlock(&vfsp->vfs_mutex);

	vfsp->vfs_flags |= VFSXLOCK;
	mutex_unlock(&vfsmutex);

	/*
	 * Associate and reference.
	 */
	vfs_setup(vfsp,
		  vfsp->vfs_flags & (VFSXLOCK|VFSXWANT),
		  dev,
		  ops,
		  private);
	vfsp->vfs_ref = 1;

	err = VFSOP_LOCK(vfsp, RWLK_WRITE, 1);
	if (err)
		panic("vfs_new: can't lock new vfs");

	mutex_lock(&vfsmutex);

	/*
	 * Put the newly associated vfs record on the list at the end
	 * of the active entries.
	 */
	TAILQ_INSERT_TAIL(&mounts, vfsp, vfs_link);

	vfsp->vfs_flags &= ~VFSXLOCK;
	vfs_wakeup(vfsp);

	mutex_unlock(&vfsmutex);

	return vfsp;
}

/*
 * Make a soft reference to a vfs.
 */
void
vfs_ref(struct vfs *vfsp)
{

	mutex_lock(&vfsp->vfs_mutex);
	if (!vfsp->vfs_ref)
		panic("vfs_ref: vfs_ref used where vfs_get should be");
	assert(++vfsp->vfs_ref);
	mutex_unlock(&vfsp->vfs_mutex);
}

/*
 * Release soft reference to a vfs.
 */
void
vfs_rele(struct vfs *vfsp)
{

	mutex_lock(&vfsp->vfs_mutex);

	assert(vfsp->vfs_ref--);
	if (!vfsp->vfs_ref)
		vfs_wakeup(vfsp);

	mutex_unlock(&vfsp->vfs_mutex);
}

/*
 * Get "first" reference to a vfs and potentially lock it.
 */
int
vfs_get(struct vfs *vfsp, rwty_t lkty, unsigned dontwait)
{
	int	err;

	mutex_lock(&vfsp->vfs_mutex);

	/*
	 * If the vfs record is changing, wait for it to finish and return
	 * an error.
	 */
	if (vfsp->vfs_flags & VFSXLOCK) {
		do
			vfs_wait(vfsp);
		while (vfsp->vfs_flags & VFSXLOCK);
		mutex_unlock(&vfsp->vfs_mutex);
		return ENOENT;
	}

	assert(++vfsp->vfs_ref);			/* reference */

	mutex_unlock(&vfsp->vfs_mutex);

	err =
	    (lkty == RWLK_READ || lkty == RWLK_WRITE) ?
	      VFSOP_LOCK(vfsp, lkty, dontwait) :
	      0;
	if (err)
		vfs_rele(vfsp);
	return err;
}

/*
 * Unlock and release reference to a vfs record.
 */
void
vfs_put(struct vfs *vfsp)
{

	VFSOP_UNLOCK(vfsp);
	vfs_rele(vfsp);
}

/*
 * Search the vnode cache table beginning at a given index for an entry
 * corresponding to some dev-ino pair.
 *
 * NB: The caller should hold a lock on the cache.
 */
static struct vnode_cache_entry *
vcache_search(unsigned i,
	       z_dev_t dev,
	       z_ino_t ino,
	       enum vcacheop op)
{
	unsigned oi;
	struct vnode_cache_entry *vcep, *freevcep;

#ifndef NDEBUG
	assert(!(dev == NODEV || ino == NOINO));
#endif

#ifdef INSTRUMENT
	if (op == VCACHE_LOOKUP)
		vcache_instrument.lookups++;
#endif
	i %= vcache_len;
	oi = i;
	vcep = &vcache[i++];
	freevcep = NULL;
	do {
		if (vcep->vce_dev == dev &&
		    vcep->vce_ino == ino)
			break;
		if (freevcep == NULL &&
		    vcep->vce_dev == NODEV &&
		    vcep->vce_ino == NOINO)
			freevcep = vcep;
		i++; vcep++;
		if (i >= vcache_len) {
			i = 0;
			vcep = vcache;
		}
	} while (i != oi);
#ifdef INSTRUMENT
	if (vcep->vce_dev == dev && vcep->vce_ino == ino)
		vcache_instrument.hits++;
	else
#endif
	if (!(vcep->vce_dev == dev && vcep->vce_ino == ino) &&
	    op == VCACHE_INSERT)
		vcep = freevcep;
	return vcep;
}

/*
 * Try to find a vnode by ino and dev in the vnode cache.
 */
int
vfs_vfind(z_dev_t dev,
	  z_ino_t ino,
	  rwty_t lkf,
	  int dontwait,
	  struct vnode **vpp)
{
	int	err;
	struct vnode_cache_entry *vcep;
	struct vnode *vp;
	int	unlock;

	unlock = 0;
	while (1) {
		vp = NULL;
		err = rw_lock(&vcache_lock, RWLK_READ, 0);
		if (err)
			panic("vfs_findv: can't lock");
		vcep =
		    vcache_search(vfs_hash(dev, ino),
				  dev, ino,
				  VCACHE_LOOKUP);
		if (vcep->vce_dev == dev && vcep->vce_ino == ino)
			vp = vcep->vce_vp;
		rw_unlock(&vcache_lock);
		if (vp == NULL)
			break;

		/*
		 * The vnode get is performed external to the cache search
		 * above. Otherwise, the lock could be held for a long, long
		 * time.
		 */
		if ((unlock = (lkf != RWLK_READ && lkf != RWLK_WRITE)))
			lkf = RWLK_READ;
		err = v_get(vp, lkf, dontwait);
		if (err || vp->v_ino != ino || vp->v_vfsp->vfs_dev !=  dev) {
			/*
			 * It changed between the time we found it and did the
			 * "get".
			 */
			if (!err)
				v_put(vp);
			continue;
		}
		break;
	}

	if (err)
		return err;
	if (vp == NULL)
		return ENOENT;
	if (unlock)
		VOP_UNLOCK(vp);
	*vpp = vp;
	return 0;
}

/*
 * Insert a vnode into the vnode cache.
 */
static int
vfs_vcache_insert(z_dev_t dev, z_ino_t ino, struct vnode *vp)
{
	int	err;
	struct vnode_cache_entry *vcep;

	err = rw_lock(&vcache_lock, RWLK_WRITE, 0);
	if (err)
		panic("vfs_vcache_insert: can't lock");
	vcep = vcache_search(vfs_hash(dev, ino), dev, ino, VCACHE_INSERT);
	if (vcep->vce_dev == dev && vcep->vce_ino == ino)
		err = EEXIST;
	else if (vcep->vce_dev != NODEV || vcep->vce_ino != NOINO)
		panic("vfs_cache_insert: table full");
	if (!err)
		vcache_entry_setup(vcep, dev, ino, vp);
	rw_unlock(&vcache_lock);

	LOG_DBG(VNODE_DBG_CHECK(3),
		"vfs_vcache_insert: " FMT_Z_DEV_T "." FMT_Z_INO_T " err %d",
		dev, ino, err);
	return err;
}

/*
 * Remove a vnode from the vnode cache.
 */
static void
vfs_vcache_remove(z_dev_t dev, z_ino_t ino)
{
	int	err;
	struct vnode_cache_entry *vcep;

	err = rw_lock(&vcache_lock, RWLK_WRITE, 0);
	if (err)
		panic("vfs_vcache_remove: can't lock");
	vcep = vcache_search(vfs_hash(dev, ino), dev, ino, VCACHE_LOOKUP);
	if (vcep->vce_dev != dev || vcep->vce_ino != ino)
		panic("vfs_cache_remove: not found");
	vcache_entry_setup(vcep, NODEV, NOINO, NULL);
	rw_unlock(&vcache_lock);

	LOG_DBG(VNODE_DBG_CHECK(3),
		"vfs_vcache_remove: " FMT_Z_DEV_T "." FMT_Z_INO_T " err %d",
		dev, ino, err);
}

/*
 * Start a file system.
 */
int
vfs_mount(const char *type,
	  const void *args,
	  size_t argslen,
	  struct creds *crp,
	  struct vfs **vfspp)
{
	struct vfsconf *vfsconfp;
	int	err;

	if (!is_suser(crp))
		return EPERM;

	err = 0;

	mutex_lock(&mount_mutex);

	for (vfsconfp = vfsconftbl; vfsconfp->vfsconf_type != NULL; vfsconfp++)
		if (strcasecmp(type, vfsconfp->vfsconf_type) == 0)
			break;
	if (vfsconfp->vfsconf_type == NULL)
		err = EINVAL;

	if (!err)
		err = (*vfsconfp->vfsconf_mount)(args, argslen, crp, vfspp);

	mutex_unlock(&mount_mutex);

	return err;
}

int
vfs_umount(struct vfs *vfsp, struct creds *crp)
{
	int	err;
	int	isroot;
	int	iscovered;
	struct vnode *vp;

	if (!is_suser(crp))
		return EPERM;

	isroot = 0;
	iscovered = 0;

	mutex_lock(&mount_mutex);

	/*
	 * Try to shed all the vnodes but the root. We do this without
	 * preventing others from activating new vnodes. However, it's a race we
	 * are prepared to lose. This means that the file system must
	 * stay quiescent in order for this to succeed. Anything else means
	 * it's busy and we err out.
	 */
	err = mutex_trylock(&vfsp->vfs_mutex);
	if (err) {
		mutex_unlock(&mount_mutex);
		return err;
	}
	while (!err) {
		isroot = 0;
		if ((vp = TAILQ_FIRST(&vfsp->vfs_vnodes)) == NULL)
			break;
		err = v_get(vp, RWLK_READ, 1);
		if (err) {
			if (err == ENOENT)
				continue;
			vp = NULL;
			break;
		}
		isroot = vp->v_mountpoint != NULL;
		iscovered = vp->v_cover != NULL;
		if (iscovered ||
		    (isroot && vp == TAILQ_LAST(&vfsp->vfs_vnodes, vnode_list)))
			break;
		err = mutex_trylock(&vp->v_mutex);
		if (err)
			break;
		/*
		 * The root vnode belongs at the end. If this is it, move
		 * it there now and go after the next vnode.
		 */
		if (isroot) {
			TAILQ_REMOVE(&vfsp->vfs_vnodes, vp, v_fslink);
			TAILQ_INSERT_TAIL(&vfsp->vfs_vnodes,
					  vp,
					  v_fslink);
			mutex_unlock(&vp->v_mutex);
			v_put(vp);
			continue;
		}
		/*
		 * Referenced vnodes are busy. So would the file system be. If
		 * that's so, we're done now.
		 *
		 * We can lose a race between this check and when we call
		 * vgone below. However, it's not a disaster, only inconvenient,
		 * if we do.
		 *
		 * The race where we are unmounting a FS that a mount is being
		 * attempted on is handled by allowing only one mount/unmount
		 * process to be in progress (the mount_mutex is held) at any
		 * given moment.
		 */
		if (vp->v_ref > 1)
			err = EBUSY;
		mutex_unlock(&vp->v_mutex);
		if (err)
			break;
		/*
		 * Must drop the lock for the v_gone so that vfs_remv can
		 * acquire it while altering the list of vnodes active on
		 * this vfs.
		 */
		mutex_unlock(&vfsp->vfs_mutex);
		v_gone(vp);
		err = mutex_trylock(&vfsp->vfs_mutex);
		if (err) {
			mutex_unlock(&mount_mutex);
			return err;
		}
	}
	if (vp != NULL &&
	    (iscovered ||
	     (isroot &&
	     vp != TAILQ_LAST(&vfsp->vfs_vnodes, vnode_list)))) {
		v_put(vp);
		mutex_unlock(&vfsp->vfs_mutex);
		mutex_unlock(&mount_mutex);
		return EBUSY;
	}

	/*
	 * Need to clean the root of this FS off the directory on which
	 * it is mounted. To do that, we need to lock against all
	 * access.
	 */
	err = v_get(vp->v_mountpoint, RWLK_WRITE, 0);
	if (err)
		panic("vfs_umount: can't get lock on underlying mount-point");
	vp->v_mountpoint->v_cover = NULL;
	v_put(vp->v_mountpoint);

	/*
	 * Commit to unmounting this FS now.
	 */
	vfsp->vfs_flags |= VFSXLOCK;
	mutex_unlock(&vfsp->vfs_mutex);

	/*
	 * Get rid of the root finally. Have to release the soft reference
	 * because it was a root vnode.
	 */
	if (vp != NULL) {
		v_rele(vp);
		v_gone(vp);
	}

	assert(TAILQ_FIRST(&vfsp->vfs_vnodes) == NULL);

	/*
	 * Drop the lock (below) held on the FS.
	 */
	VFSOP_UNLOCK(vfsp);

	/*
	 * Disassociate and wake waiters.
	 */
	VFSOP_UNMOUNT(vfsp);
	vfs_setup(vfsp,
		  vfsp->vfs_flags & VFSXWANT,
		  NODEV,
		  &dead_vfsops,
		  NULL);

	/*
	 * Put it on the free list at the beginning (where it is
	 * a prime candidate for reuse) and wake waiters.
	 */
	mutex_lock(&vfsmutex);
	TAILQ_REMOVE(&mounts, vfsp, vfs_link);
	TAILQ_INSERT_HEAD(&free_mountrecs, vfsp, vfs_link);
	mutex_unlock(&vfsmutex);

	mutex_unlock(&mount_mutex);
	vfs_rele(vfsp);
	return 0;
}

/*
 * Return vfs record given device ID.
 */
int
getvfs(z_dev_t dev, struct vfs **vfspp)
{
	struct vfs *vfsp;

	mutex_lock(&vfsmutex);

	for (vfsp = TAILQ_FIRST(&mounts);
	     vfsp != NULL;
	     vfsp = TAILQ_NEXT(vfsp, vfs_link)) {
		/*
		 * We can't properly "get" the vfs record we want to exmaine.
		 * To do so could cause us to sleep with the mount mutex
		 * held. Do a quick, light, check then.
		 */
		mutex_lock(&vfsp->vfs_mutex);
		if (!(vfsp->vfs_flags & VFSXLOCK) && vfsp->vfs_dev == dev)
			break;
		mutex_unlock(&vfsp->vfs_mutex);
	}

	mutex_unlock(&vfsmutex);

	if (vfsp != NULL) {
		assert(++vfsp->vfs_ref);
		mutex_unlock(&vfsp->vfs_mutex);
		*vfspp = vfsp;
		return 0;
	}

	return ENOENT;
}

/*
 * Record a new active vnode on a file system.
 */
static int
vfs_addv(struct vfs *vfsp, struct vnode *vp)
{
	int	err;

	err = 0;
	mutex_lock(&vfsp->vfs_mutex);
	err = vfs_vcache_insert(vfsp->vfs_dev, vp->v_ino, vp);
	if (!err)
		TAILQ_INSERT_TAIL(&vfsp->vfs_vnodes, vp, v_fslink);
	mutex_unlock(&vfsp->vfs_mutex);

	return err;
}

/*
 * Remove vnode from file system active vnodes list.
 */
static void
vfs_remv(struct vnode *vp)
{
	struct vfs *vfsp;

#ifndef NDEBUG
	mutex_lock(&vp->v_mutex);
	assert(vp->v_flags & VXLOCK);
	mutex_unlock(&vp->v_mutex);
#endif
	vfsp = vp->v_vfsp;
	mutex_lock(&vfsp->vfs_mutex);
	TAILQ_REMOVE(&vfsp->vfs_vnodes, vp, v_fslink);
	vfs_vcache_remove(vfsp->vfs_dev, vp->v_ino);
	mutex_unlock(&vfsp->vfs_mutex);
}

/*
 * Generic vfs operation lock for those that don't want to implement
 * it themselves.
 */
int
vfs_generic_op_lock(struct vfs *vfsp, rwty_t lkf, int dontwait)
{

#ifndef NDEBUG
	assert(vfsp->vfs_ref);
#endif

	return rw_lock(&vfsp->vfs_rwlock, lkf, dontwait);
}

/*
 * Generic vfs operation unlock for those that don't want to implement
 * it themselves.
 */
void
vfs_generic_op_unlock(struct vfs *vfsp)
{

#ifndef NDEBUG
	assert(vfsp->vfs_ref);
#endif

	return rw_unlock(&vfsp->vfs_rwlock);
}

/*
 * Generic vfs operation lkty for those that don't want to implement
 * it themselves.
 */
rwty_t
vfs_generic_op_lkty(struct vfs *vfsp)
{

#ifndef NDEBUG
	assert(vfsp->vfs_ref);
#endif

	return rw_lkstat(&vfsp->vfs_rwlock);
}

/*
 * Allocate new free vnodes and put them on the list.
 *
 * NB: Assumes the caller holds vmutex.
 */
static int
v_morev(void)
{
	struct vnode *vp;
	size_t	count;
	static size_t n = 4;

	n <<= 1;
	if (n + nvnodes > vnodes_max)
		n = vnodes_max - nvnodes;
	if (!n)
		return 0;
	vp = m_alloc(n * sizeof(struct vnode));
	if (vp == NULL) {
		LOG(LOG_WARNING, "v_morev: m_alloc failed");
		return 0;
	}
	nvnodes += n;
	(void )memset(vp, 0, n * sizeof(struct vnode));
	count = n;
	while (count--) {
		v_setup(vp, 0, NULL, NOINO, &dead_vops, NULL);
		mutex_init(&vp->v_mutex);
		cond_init(&vp->v_cond);
		rw_init(&vp->v_rwlock);
		TAILQ_INSERT_TAIL(&vnodes, vp, v_link);
		vp++;
	}
	return 1;
}

/*
 * Reclaim vnode for reuse.
 */
static void
v_reclaim(struct vnode *vp)
{
	struct vncb *vncbp;

	assert(vp->v_flags & VXLOCK);

	LOG_DBG(VNODE_DBG_CHECK(2),
		("v_reclaim: " FMT_Z_DEV_T "." FMT_Z_INO_T
		 " flags 0x%x ref %u"),
		vp->v_vfsp->vfs_dev, vp->v_ino, vp->v_flags, vp->v_ref);
	/*
	 * Run the callbacks.
	 */
	while ((vncbp = LIST_FIRST(&vp->v_callbacks)) != NULL) {
		LIST_REMOVE(vncbp, vncb_next);
		(*vncbp->vncb_f)(vp);
		free(vncbp);
	}

	if (vp->v_vfsp != NULL)
		vfs_remv(vp);
	VOP_RECLAIM(vp);
	v_setup(vp,
		vp->v_flags & (VXLOCK|VXWANT),
		NULL,
		NOINO,
		&dead_vops,
		NULL);
}

/*
 * Acquire and initialize a new vnode.
 */
struct vnode *
v_new(struct vfs *vfsp, z_ino_t ino, struct vops *ops, void *private)
{
	struct vnode *vp;
	int	avoid_reclaim;
	int	thrashing;
	int	err;

	/*
	 * If we've not yet reached our maximum number of vnodes, create
	 * a new one. Otherwise, reclaim an existing one and reuse it.
	 */
	vp = NULL;
	avoid_reclaim = 1;
	thrashing = 0;
	mutex_lock(&vmutex);
	while (1) {
		for (vp = TAILQ_LAST(&vnodes, vnode_list);
		     vp != NULL;
		     vp = TAILQ_PREV(vp, vnode_list, v_link)) {
			if (mutex_trylock(&vp->v_mutex) != 0)
				continue;
			if (vp->v_ref || vp->v_flags & VXLOCK) {
				mutex_unlock(&vp->v_mutex);
				continue;
			}
			break;
		}
		if (vp == NULL || !v_isdead(vp)) {
			if (avoid_reclaim) {
				if (vp != NULL)
					mutex_unlock(&vp->v_mutex);
				avoid_reclaim = 0;
				if (nvnodes < vnodes_max)
					(void )v_morev();
				continue;
			}
		}
		if (vp != NULL)
			break;
		mutex_unlock(&vmutex);
		/* We should never get here in a properly configured
		 * system. If we do, we're thrashing.
		 *
		 * Spin, but give other threads a chance to change
		 * something too. Ok, spinning is not the most clever
		 * thing to do but this won't happen often, right?
		 */
		if (!thrashing)
			LOG(LOG_WARNING, "v_new: thrashing");
		thrashing = 1;
#ifndef SINGLE_THREAD
		thread_yield();
#else
		panic("v_new: no reclaimable vnodes");
#endif
		mutex_lock(&vmutex);
	}
	/*
	 * Have a candidate. Lock the record so others can't change
	 * it too. Then, remove it from the list of vnodes and
	 * reclaim.
	 */
	vp->v_flags |= VXLOCK;
	mutex_unlock(&vp->v_mutex);
	TAILQ_REMOVE(&vnodes, vp, v_link);
	TAILQ_INSERT_HEAD(&vnodes, vp, v_link);
	mutex_unlock(&vmutex);				/* no longer needed */
	if (!v_isdead(vp))
		v_reclaim(vp);

	/*
	 * At this point, we have a locked (above), unassociated, unreferenced
	 * vnode. Associate it now.
	 */
	v_setup(vp, vp->v_flags & (VXLOCK|VXWANT), vfsp, ino, ops, private);
	vp->v_ref = 1;
	err = vfs_addv(vfsp, vp);
	if (err) {
		/*
		 * Can't just reclaim it. Potentially, it's not
		 * complete yet.
		 */
		assert(vp->v_ref--);
		vp->v_vfsp = NULL;			/* assoc failed above */
		mutex_lock(&vmutex);
		TAILQ_REMOVE(&vnodes, vp, v_link);
		TAILQ_INSERT_TAIL(&vnodes, vp, v_link);
		mutex_unlock(&vmutex);			/* no longer needed */
		v_setup(vp,
			vp->v_flags & (VXLOCK|VXWANT),
			NULL,
			NOINO,
			&dead_vops,
			NULL);
		mutex_lock(&vp->v_mutex);
		vp->v_flags &= ~VXLOCK;
		v_wakeup(vp);
		mutex_unlock(&vp->v_mutex);
		return NULL;
	}
	err = VOP_LOCK(vp, RWLK_WRITE, 1);
	if (err)
		panic("v_new: can't lock new vnode");

	/*
	 * Put it back on the list of all vnodes and notify waiters that we're
	 * done changing it.
	 */
	mutex_lock(&vp->v_mutex);
	vp->v_flags &= ~VXLOCK;
	v_wakeup(vp);
	mutex_unlock(&vp->v_mutex);

	return vp;
}

/*
 * Make a soft reference to a vnode.
 */
void
v_ref(struct vnode *vp)
{

	mutex_lock(&vp->v_mutex);
	if (!vp->v_ref)
		panic("v_ref: v_ref used where v_get should be");
	assert(++vp->v_ref);
	mutex_unlock(&vp->v_mutex);

	mutex_lock(&vmutex);
	TAILQ_REMOVE(&vnodes, vp, v_link);
	TAILQ_INSERT_HEAD(&vnodes, vp, v_link);
	mutex_unlock(&vmutex);
}

/*
 * Release soft reference to a vnode.
 */
void
v_rele(struct vnode *vp)
{

	mutex_lock(&vp->v_mutex);

	assert(vp->v_ref--);
	if (!vp->v_ref)
		v_wakeup(vp);

	mutex_unlock(&vp->v_mutex);
}

/*
 * Get "first" reference to a vnode and potentially lock it.
 */
int
v_get(struct vnode *vp, rwty_t lkty, unsigned dontwait)
{
	int	err;

	mutex_lock(&vp->v_mutex);
	/*
	 * If the vnode is changing, wait for it to finish and return
	 * an error.
	 */
	if (vp->v_flags & VXLOCK) {
		do
			v_wait(vp);
		while (vp->v_flags & VXLOCK);
		mutex_unlock(&vp->v_mutex);
		return ENOENT;
	}

	assert(++vp->v_ref);				/* reference */
	mutex_unlock(&vp->v_mutex);

	/*
	 * Move it to the front. It's the least likely candidate now.
	 */
	mutex_lock(&vmutex);
	TAILQ_REMOVE(&vnodes, vp, v_link);
	TAILQ_INSERT_HEAD(&vnodes, vp, v_link);
	mutex_unlock(&vmutex);

	err =
	    (lkty == RWLK_READ || lkty == RWLK_WRITE) ?
	      VOP_LOCK(vp, lkty, dontwait) :
	      0;
	if (err)
		v_rele(vp);
	return err;
}

/*
 * Unlock and release a vnode.
 */
void
v_put(struct vnode *vp)
{

	VOP_UNLOCK(vp);
	v_rele(vp);
}

/*
 * Remove a vnode association from the system.
 */
void
v_gone(struct vnode *vp)
{

	assert(VOP_LKTY(vp) == RWLK_WRITE);

	mutex_lock(&vp->v_mutex);

	if (vp->v_flags & VXLOCK) {
		/*
		 * Another is attempting a v_gone. Just wait for it to
		 * finish and return.
		 */
		v_wait(vp);
		mutex_unlock(&vp->v_mutex);
		return;
	}
	vp->v_flags |= VXLOCK;
	mutex_unlock(&vp->v_mutex);

	/*
	 * Wait for vnode to become inactive and unlink it from the list of
	 * all vnodes. Ugh! Must drop our reference in order to wait for
	 * others to clear off. Then, we have to regain it for the reclaim.
	 */
	v_put(vp);
	mutex_lock(&vp->v_mutex);
	while (vp->v_ref)
		v_wait(vp);
	mutex_unlock(&vp->v_mutex);

#if 0
	assert(!vp->v_ref++);
#endif
	v_reclaim(vp);

	/*
	 * Put it back on the list of all vnodes at the end (where it is
	 * a prime candidate for reuse) and wake waiters.
	 */
	mutex_lock(&vmutex);
	TAILQ_REMOVE(&vnodes, vp, v_link);
	TAILQ_INSERT_TAIL(&vnodes, vp, v_link);
	mutex_unlock(&vmutex);

	mutex_lock(&vp->v_mutex);
	vp->v_flags &= ~VXLOCK;
	v_wakeup(vp);
	mutex_unlock(&vp->v_mutex);

	return;
}

/*
 * Add a function to be called to the reclaim callback chain.
 */
int
v_atreclaim(struct vnode *vp, void (*f)(struct vnode *))
{
	struct vncb *vncbp;

	if (v_isdead(vp))
		panic("v_atreclaim: can't add callback on dead vnode");

	vncbp = m_alloc(sizeof(struct vncb));
	if (vncbp == NULL)
		return ENOMEM;
	vncbp->vncb_f = f;
	mutex_lock(&vp->v_mutex);
	assert(vp->v_ref);
	LIST_INSERT_HEAD(&vp->v_callbacks, vncbp, vncb_next);
	mutex_unlock(&vp->v_mutex);

	return 0;
}

/*
 * Generic vnode operation lock for those that don't want to implement
 * it themselves.
 */
int
v_generic_op_lock(struct vnode *vp, rwty_t lkf, int dontwait)
{

#ifndef NDEBUG
	assert(vp->v_ref);
#endif

	return rw_lock(&vp->v_rwlock, lkf, dontwait);
}

/*
 * Generic vnode operation unlock for those that don't want to implement
 * it themselves.
 */
void
v_generic_op_unlock(struct vnode *vp)
{

#ifndef NDEBUG
	assert(vp->v_ref);
#endif

	return rw_unlock(&vp->v_rwlock);
}

/*
 * Generic vnode operation lkty for those that don't want to implement
 * it themselves.
 */
rwty_t
v_generic_op_lkty(struct vnode *vp)
{

#ifndef NDEBUG
	assert(vp->v_ref);
#endif

	return rw_lkstat(&vp->v_rwlock);
}

/*
 * Generic "not supported" operation.
 */
int
v_generic_op_nosys(void)
{

	return ENOSYS;
}

/*
 * Acquire locks on multiple vnodes in a manner that prevents deadlock.
 *
 * NB: It is assumed that all vnodes mentioned are referenced and are
 * on the same file system.
 */
int
v_multilock(struct vlkop *lkops, size_t nlkops)
{
	size_t	i, j;
	struct vlkop lkop;
	int	order;
	int	err;

	/*
	 * Sort them into ascending order.
	 */
	for (i = 0; i < nlkops; i++)
		for (j = i + 1; j < nlkops; j++) {
			assert(lkops[i].vlkop_vp->v_vfsp ==
			       lkops[j].vlkop_vp->v_vfsp);
			order = v_cmp(lkops[i].vlkop_vp, lkops[j].vlkop_vp);
			assert(order);
			if (order > 0) {
				lkop = lkops[i];
				lkops[i] = lkops[j];
				lkops[j] = lkop;
			}
		}

	/*
	 * Now lock them, in order.
	 */
	err = 0;
	for (i = 0; i < nlkops; i++) {
		err = VOP_LOCK(lkops[i].vlkop_vp, lkops[i].vlkop_lkf, 0);
		if (err)
			break;
	}
	if (!err)
		return 0;

	while (i--)
		VOP_UNLOCK(lkops[i].vlkop_vp);
	return err;
}

/*
 * Do some common things around a VOP_LOOKUP. This should always be used in
 * place of VOP_LOOKUP unless you know what you are doing.
 */
int
v_lookup(struct vnode *dvp,
	 const char *name,
	 struct creds *crp,
	 rwty_t lkf,
	 struct vnode **vpp)
{
	int	isdotdot;
	struct vnode *vp;
	int	err;
	int	intermediate;

	if (strchr(name, '/') != NULL) {
		VOP_UNLOCK(dvp);			/* even on error */
		return EINVAL;
	}
	assert(v_islocked(dvp));
	isdotdot = 0;
	if (name[0] == '.') {
		if (name[1] == '\0') {
			/*
			 * It's just `.' and we're done.
			 */
			VOP_UNLOCK(dvp);
			return v_get(dvp, lkf, 0);
		}
		if (name[1] == '.' && name[2] == '\0')
			isdotdot = 1;
	}

	/*
	 * Special case: Check for ref of `..' through a mount point.
	 */
	intermediate = 0;
	err = 0;
	while (isdotdot &&
	       !err &&
	       dvp->v_mountpoint != NULL &&
	       dvp->v_mountpoint != dvp) {
		vp = dvp->v_mountpoint;
		if (intermediate)
			v_put(dvp);
		else
			VOP_UNLOCK(dvp);
		dvp = vp;
		err = v_get(dvp, RWLK_READ, 0);
		if (err) {
			v_put(dvp);
			dvp = NULL;
		}
		intermediate = 1;
	}
#if 0
	/*
	 * Special case: Check for cross-device lookup.
	 */
	while (!isdotdot &&
	       !err &&
	       dvp->v_cover != NULL &&
	       dvp->v_cover != dvp) {
		vp = dvp->v_cover;
		if (intermediate)
			v_put(dvp);
		else
			VOP_UNLOCK(dvp);
		dvp = vp;
		err = v_get(dvp, RWLK_READ, 0);
		if (err) {
			v_put(dvp);
			dvp = NULL;
		}
		intermediate = 1;
	}
#endif
	if (!err)
		err = VOP_LOOKUP(dvp, name, crp, RWLK_READ, &vp);
	/*
	 * Newly found vnode mounted on?
	 */
	while (!err && vp->v_cover != NULL) {
		struct vnode *cvp;

		cvp = vp->v_cover;
		VOP_UNLOCK(vp);
		err = v_get(cvp, RWLK_READ, 0);
		v_rele(vp);
		vp = cvp;
	}
	if (!err) {
		/*
		 * If our read lock isn't what was requested, drop it and
		 * get the requested lock.
		 */
		if (lkf != VOP_LKTY(vp))
			VOP_UNLOCK(vp);
		if (lkf == RWLK_WRITE)
			err = VOP_LOCK(vp, lkf, 0);
		if (err)
			v_rele(vp);
	}
	if (!err)
		*vpp = vp;
	if (intermediate)
		v_rele(dvp);

	return err;
}

#ifdef VNODE_DEBUG
void
v_print(struct vnode *vp)
{

	LOG(LOG_DEBUG, "v_print: vp %p, dev " FMT_Z_DEV_T ", ino " FMT_Z_INO_T,
			vp,
			vp->v_vfsp == NULL ? NODEV : vp->v_vfsp->vfs_dev,
			vp->v_ino);
	LOG(LOG_DEBUG, "v_print: v_flags %u, v_ref %u, private %p",
			vp->v_flags,
			vp->v_ref,
			vp->v_private);
}
#endif
