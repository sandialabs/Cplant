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
#include <stdio.h>				/* for NULL */
#ifdef PROCFS
#include <string.h>
#endif
#include <unistd.h>				/* for getpaqgesize() */

#include "cmn.h"
#include "vfs.h"
#include "vnode.h"
#include "buffer.h"

#ifdef PROCFS
#include "procfs.h"
#endif

IDENTIFY("$Id: vfs_bio.c,v 1.5 2001/04/18 15:42:02 rklundt Exp $");

static mutex_t mutex = MUTEX_INITIALIZER;	/* package mutex */

/*
 * Free buffer heads.
 */
static TAILQ_HEAD(, buffer) bufhds = TAILQ_HEAD_INITIALIZER(bufhds);
static unsigned nbufhds = 0;		/* total bufhds */
static unsigned freebufhds;			/* free bufhds */

/*
 * Active buffers.
 */
static TAILQ_HEAD(activ_bufs, buffer) bufs = TAILQ_HEAD_INITIALIZER(bufs);

/*
 * Number of data bytes allocated.
 */
static size_t bufmemsiz = 0;

#ifdef PROCFS
static int buf_procread(procfs_ino *, z_off_t, char *, u_int32_t *);
static int buf_procwrite(procfs_ino *, z_off_t, char *, u_int32_t *);

static procfs_ino_ops buf_procops = {
	procfs_ino_create,
	buf_procread,
	buf_procwrite,
	procfs_ino_gattr,
	procfs_ino_sattr,
	procfs_ino_destroy
};
#endif

/*
 * Create more buffer heads for the allocator.
 */
static void
buf_morehds(void)
{
	static mutex_t mymutex = MUTEX_INITIALIZER;
	static size_t pgsiz;
	static size_t m = 0;
	size_t	n, r;
	struct buffer *bp;
	static size_t nbytes = 0;

	/*
	 * Only one at a time please.
	 */
	mutex_lock(&mymutex);

	if (!m) {
		m = pgsiz = getpagesize();
#ifdef PARANOID
		BUG_CHECK(m > sizeof(struct buffer));
#endif
		/*
		 * Use Euclid's algorithm to find the greatest common
		 * divisor. We want to use memory efficiently.
		 */
		n = sizeof(struct buffer);
		do {
			r = m % n;
			m = n;
			n = r;
		} while (n);
		if (m >> 4) {
			/*
			 * Ouch! Too big. Be fast about it then.
			 */
			m = sizeof(struct buffer);
		}
	}
	bp = malloc(m * pgsiz);
	if (bp == NULL)
		panic("buf_morehds: malloc");

	n = m * pgsiz;
	r = n / sizeof(struct buffer);
#if 1
	LOG(LOG_INFO,
	    "%s %lu buffer headers (%lu bytes)",
	    nbufhds ? "Grow to" : "Reserved",
	    nbufhds + r,
	    nbytes + n);
#endif
	nbufhds += r;
	nbytes += n;

	mutex_lock(&mutex);
	freebufhds += r;
	while (r--) {
		mutex_init(&bp->b_mutex);
		cond_init(&bp->b_cond);
		BUF_INIT(bp, NODEV, NOINO, 0);
		TAILQ_INSERT_HEAD(&bufhds, bp, b_link);
		bp++;
	}
	mutex_unlock(&mutex);

	mutex_unlock(&mymutex);
}

/*
 * Initialize buffer system.
 */
void
buf_init(void)
{
	int	err;

	buf_morehds();
#ifdef PROCFS
	err = procfs_register("/fs/bufs", &buf_procops, &procfs_defattr_file);
	if (err != 0)
		LOG(LOG_ERR,
		    "buf_init: cannot register procfs-bufs entry: %s",
		    strerror(err));
#endif /* PROCFS */
}

/*
 * Allocate a buffer head.
 */
struct buffer *
buf_hdralloc(z_dev_t dev, z_ino_t ino, z_blk_t blkno)
{
	struct buffer *bp;

	mutex_lock(&mutex);
	do {
		bp = TAILQ_FIRST(&bufhds);
		if (bp == NULL) {
			mutex_unlock(&mutex);
			buf_morehds();
			mutex_lock(&mutex);
		}
	} while (bp == NULL);
	TAILQ_REMOVE(&bufhds, bp, b_link);
	assert(freebufhds--);
	mutex_unlock(&mutex);

#ifdef PARANOID
	(void )memset(bp, 0, sizeof(struct buffer));
#endif
	BUF_INIT(bp, dev, ino, blkno);
	return bp;
}

/*
 * Free a buffer head.
 */
void
buf_hdrfree(struct buffer *bp)
{
	BUF_INIT(bp, NODEV, NOINO, 0);

	mutex_lock(&mutex);
	TAILQ_INSERT_HEAD(&bufhds, bp, b_link);
	assert(++freebufhds);
	mutex_unlock(&mutex);
}


static void
buf_gc(size_t desired)
{
	size_t	n;
	struct buffer *bp;
	struct buffer *nxtbp;
	LIST_HEAD(, buffer) blist = LIST_HEAD_INITIALIZER(blist);

	mutex_lock(&mutex);
	if (desired > bufmemsiz)
		desired = bufmemsiz;
	n = 0;
	for (;;) {
		/*
		 * Move enough old buffers to the private list
		 * to satisfy the requested amount of memory to free.
		 */
		bp = TAILQ_LAST(&bufs, activ_bufs);
		nxtbp = NULL;
		while (n < desired && bp != NULL) {
			int	err;

			nxtbp = TAILQ_PREV(bp, activ_bufs, b_link);
			err = mutex_trylock(&bp->b_mutex);
			if (err) {
				bp = nxtbp;
				continue;
			}
			if (bp->b_refs) {
				mutex_unlock(&bp->b_mutex);
				bp = nxtbp;
				continue;
			}
			n += bp->b_size;
			v_invalbuf(bp);
			LIST_INSERT_HEAD(&blist, bp, b_cache_link);
			bp = nxtbp;
		}
		mutex_unlock(&mutex);
		/*
		 * Free the buffers on the private list.
		 */
		while ((bp = LIST_FIRST(&blist)) != NULL) {
			LIST_REMOVE(bp, b_cache_link);
			buf_free(bp);
			mutex_unlock(&bp->b_mutex);
		}
		if (n >= desired)
			break;
		/*
		 * Give others a chance to do something to release
		 * the buffers we couldn't free.
		 */
		sched_yield();
		mutex_lock(&mutex);
	}
}

struct buffer *
buf_alloc(struct vnode *vp, z_blk_t blkno)
{
	int	err;
	struct vstat vstbuf;
	size_t	desired;
	void	*p;
	struct buffer *bp;

	err = VOP_GETATTR(vp, &vstbuf);
	if (err)
		panic("buf_alloc: can't get vnode attrs");

	mutex_lock(&mutex);
	/* 
	 * If bufmemsize has grown to within 1M of
	 * available heap size, time for garbage collection
	 * 'desired' is the amount to try to free
	 */
	desired =
	    bufmemsiz > heap_size - 1024 * 1024 ? (bufmemsiz >> 2) : 0;
/* 	    bufmemsiz > heap_size - 1024 * 1024 ? (bufmemsiz >> 4) : 0; */
	bufmemsiz += vstbuf.vst_blocksize;

	mutex_unlock(&mutex);
	for (;;) {
		if (desired)
			buf_gc(desired);
		/*
		 * Try for the needed size.
		 */
		p = malloc(vstbuf.vst_blocksize);
		if (p != NULL)
			break;
		/*
		 * It's a real busy system or we're not freeing
		 * enough *big* buffers. Double what we want to
		 * free and try again.
		 */
		desired <<= 1;
		if (!desired)
			panic("buf_gc: not enough memory to fill request");
	}

	bp = buf_hdralloc(vp->v_vfsp->vfs_dev, vp->v_ino, blkno);
	bp->b_blkno = blkno;
	bp->b_data = p;
	bp->b_size = vstbuf.vst_blocksize;

	mutex_lock(&mutex);
	TAILQ_INSERT_HEAD(&bufs, bp, b_link);
	mutex_unlock(&mutex);

	return bp;
}

void
buf_free(struct buffer *bp)
{

	BUG_CHECK(bp->b_refs == 0 && bp->b_size && bp->b_data != NULL);

	free(bp->b_data);
	mutex_lock(&mutex);
	bufmemsiz -= bp->b_size;
	TAILQ_REMOVE(&bufs, bp, b_link);
	mutex_unlock(&mutex);

	buf_hdrfree(bp);
}

/*
 * Lock and reference a buffer.
 */
int
buf_get(struct buffer *bp)
{

	BUF_LOCK(bp);
	assert(++bp->b_refs);

#if 0
	/*
	 * Move to front.
	 */
	mutex_lock(&mutex);
	TAILQ_REMOVE(&bufs, bp, b_link);
	TAILQ_INSERT_HEAD(&bufs, bp, b_link);
	mutex_unlock(&mutex);
#endif

	return 0;
}

/*
 * Wait for IO operation to complete on a buffer.
 *
 * Note:
 *
 * Assumes the buffer is locked.
 */
int
buf_iowait(struct buffer *bp)
{

	while (bp->b_flags & B_BUSY) {
		bp->b_flags |= B_WANT;
		cond_wait(&bp->b_cond, &bp->b_mutex);
	}
	return bp->b_flags & B_ERROR ? bp->b_error : 0;
}

/*
 * Signal completion of IO operation on a buffer.
 *
 * Note:
 *
 * Assumes the buffer is locked.
 */
int
buf_iodone(struct buffer *bp, int error)
{

	bp->b_flags &= ~B_BUSY;
	if (error) {
		bp->b_error = error;
		bp->b_flags |= B_ERROR;
	}
	if (bp->b_flags & B_WANT) {
		cond_broadcast(&bp->b_cond);
		bp->b_flags &= ~B_WANT;
	}
	bp->b_crp = NULL;				/* drop credentials */
	return error;
}

#if 0
/*
 * Insert buffer into vnode's pending IO queue.
 *
 * A localized elevator sort is used.
 *
 * NB: Assumes the vnode butex is held by the caller.
 */
static void
q4io(struct vnode *vp, struct buffer *bp)
{
	struct buffer *b1p, *b2p;

	if ((b2p = TAILQ_LAST(&vp->v_ioq, ioq)) == NULL)
		TAILQ_INSERT_HEAD(&vp->v_ioq, bp, b_ioq_link);
	else if ((b1p = TAILQ_PREV(b2p, ioq, b_ioq_link)) == NULL)
		TAILQ_INSERT_AFTER(&vp->v_ioq, b2p, bp, b_ioq_link);
	else if ((b1p->b_blkno < bp->b_blkno && bp->b_blkno < b2p->b_blkno) ||
	         (b1p->b_blkno > bp->b_blkno && bp->b_blkno > b2p->b_blkno))
			TAILQ_INSERT_AFTER(&vp->v_ioq, b1p, bp, b_ioq_link);
	else
		TAILQ_INSERT_AFTER(&vp->v_ioq, b2p, bp, b_ioq_link);
}
#endif

/*
 * Setup and enqueue an asynchronous IO operation.
 *
 * NB:
 * This routine is byte granular. It will completely fill a buffer if
 * any part is requested and not already present. However, for writes
 * it assumes the underlying file system does not require block level
 * updates. If it does, the caller must use it correctly.
 */
int
buf_aio(struct vnode *vp,
	struct buffer *bp,
	unsigned flags,
	unsigned off,
	size_t len,
	struct creds *crp)
{
	int	err;

	BUG_CHECK(vp != NULL &&
		  vp->v_vfsp != NULL &&
		  bp->b_dev == vp->v_vfsp->vfs_dev &&
		  bp->b_ino == vp->v_ino);
	/*
	 * Must wait for any current operation to complete. We can only
	 * have one request outstanding.
	 */
	err = buf_iowait(bp);
	if (err) {
		v_invalbuf(bp);
		return err;
	}
	flags &= B_READ;
	flags |= B_BUSY;
	if (flags & B_READ) {
		/*
		 * If the requested portion is valid, just return now.
		 */
		if (bp->b_off <= off && bp->b_len - (off - bp->b_off) >= len)
			return 0;
		BIO_SETUP(bp, flags, 0, bp->b_size, crp);
	} else
		BIO_SETUP(bp, flags, off, len, crp);
	/*
	 * Normally, we never want to lock a buffer, followed by the
	 * vnode. However, we are assuming the buffer is not already queued
	 * and so cannot be found on the vnode's list of buffers queued for
	 * IO. As long as that's true, this won't deadlock.
	 */
	return VOP_STRATEGY(vp, bp);
}

/*
 * Perform IO operation.
 */
int
buf_io(struct vnode *vp,
       struct buffer *bp,
       unsigned direction,
       unsigned off,
       size_t len,
       struct creds *crp)
{
	int	err;

	err = buf_aio(vp, bp, direction, off, len, crp);
	if (err)
		return err;
	return buf_iowait(bp);
}

#ifdef PROCFS
static int
buf_procread(procfs_ino *inode IS_UNUSED,
	     z_off_t offset,
	     char *buf,
	     u_int32_t *buflen)
{
	int	cc;
	int	count;

	if (offset)
		return EINVAL;
	count = cc =
	    snbprintf(buf, *buflen,
		      "mem: heap %u buf %u\n",
		      heap_size, bufmemsiz);
	if (cc) {
		cc =
		    snbprintf(buf + cc, *buflen - cc,
			      "bufhds: n %u, free %u\n",
			      nbufhds, freebufhds);
		if (cc)
			count += cc;
	}

	if (cc < 0) {
		*buflen = 0;
		return errno;
	}
	*buflen = count;
	return 0;
}

static int
buf_procwrite(procfs_ino *inode IS_UNUSED,
	      z_off_t offset IS_UNUSED,
	      char *buf,
	      u_int32_t *buflen)
{
	size_t	cc;
	char	*cp;
	char	*s;
	long	value;

	if (offset)
		return EINVAL;
	for (cc = 0, cp = buf;
	     cc < *buflen && !(*cp == '\n' || * cp == '\r' || *cp == '\0');
	     cc++, cp++)
		;
	if (!cc) {
		if (*buflen)
			return EINVAL;
		return 0;
	}
	s = malloc(cc + 1);
	if (s == NULL) {
		LOG(LOG_ERR, "buf_procwrite: malloc failed");
		*buflen = 0;
		return ENOMEM;
	}
	(void )memcpy(s, buf, cc);
	s[cc] = '\0';
	value = strtol(s, &cp, 0);
	free(s);
	if (*cp != '\0' || value <= 0) {
		*buflen = 0;
		return EINVAL;
	}
	heap_size = value;
	return 0;
}
#endif /* PROCFS */
