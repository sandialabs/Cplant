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
#include <stdio.h>					/* need NULL */

#include "cmn.h"
#include "smp.h"
#include "vfs.h"
#include "vnode.h"
#include "buffer.h"

/*
 * Buffer memory allocated.
 */
static size_t buf_mem = 0;

TAILQ_HEAD(buffer_headers_list, buffer);

/*
 * Free buffer data pages.
 */
static void *pages = NULL;

/*
 * Free buffer headers.
 */
static struct buffer_headers_list free_bufs = TAILQ_HEAD_INITIALIZER(free_bufs);

/*
 * In-use buffers.
 */
static struct buffer_headers_list bufs = TAILQ_HEAD_INITIALIZER(bufs);

/*
 * The buffers hash table.
 */
static struct buffer_headers_list *bufstbl = NULL;

static mutex_t mutex = MUTEX_INITIALIZER;

/*
 * Generate hash key from  pointer to vnode and buf page index.
 */
#define buf_hash(vp, bpgno) \
	(((unsigned )(vp) << 8) + (bpgno))

void
buf_init(void *p, size_t heap_size)
{
	void	*op;

	pages = p;
	buf_mem = heap_size;

#ifndef NDEBUG
	assert(p != NULL && heap_size);
#endif
	buf_mem = rnddwn(buf_mem, VMPAGESIZE);
	op = NULL;
	do {
		*(void **)p = op;
		op = p;
		((caddr_t )p) += VMPAGESIZE;
	} while (p < pages + buf_mem);
}

/*
 * Reclaim some buffers from the in-use queue.
 *
 * NB: Assumes the package mutex is held by the caller.
 */
static void
buf_reclaim(void)
{
	size_t	reclaimed;
	struct buffer *bp;

	reclaimed = 0;
	bp = TAILQ_LAST(&bufs, buffer_headers_list);
	while (reclaimed < buf_mem / 10 && bp != NULL) {
		struct buffer *nxtbp;

		if (!(bp->b_flags & B_IODONE) ||
		    mutex_trylock(&bp->b_mutex) != 0) {
			bp = TAILQ_PREV(bp, buffer_headers_list, b_link);
			continue;
		}
		if (!(bp->b_flags & B_IODONE)) {
			mutex_unlock(&bp->b_mutex);
			bp = TAILQ_PREV(bp, buffer_headers_list, b_link);
			continue;
		}
		if (bp->b_data != NULL) {
			*(void **)bp->b_data = pages;
			pages = bp->b_data;
			bp->b_data = NULL;
			reclaimed += VMPAGESIZE;
		}
		nxtbp = TAILQ_PREV(bp, buffer_headers_list, b_link);
		TAILQ_REMOVE(&bufs, bp, b_link);
		TAILQ_INSERT_HEAD(&free_bufs, bp, b_link);
		bp = nxtbp;
	}
}

void *
buf_pgalloc(void)
{
	void	*p;

	p = NULL;
	mutex_lock(&mutex);
	while (p == NULL) {
		p = &pages;
		if (p == NULL)
			buf_reclaim();
		pages = *(void **)p;
		break;
	}
	mutex_unlock(&mutex);

	return p;
}

/*
 * Allocate and return a single buf header.
 */
static struct buffer *
buf_hdralloc(void)
{
	static mutex_t m = MUTEX_INITIALIZER;
	static unsigned busy = 0;
	static unsigned waiters = 0;
	static cond_t c = COND_INITIALIZER;
	struct buffer *bp;
	unsigned n;

	/*
	 * Begin exclusive.
	 */
	mutex_lock(&m);
	while (busy) {
		assert(++waiters);
		cond_wait(&c, &m);
		assert(waiters--);
	}
	busy = 1;
	mutex_unlock(&m);

	/*
	 * Can we return one from the free list?
	 */
	mutex_lock(&mutex);
	bp = TAILQ_FIRST(&bufs);
	if (bp != NULL)
		TAILQ_REMOVE(&bufs, bp, b_link);
	mutex_unlock(&mutex);

	if (bp == NULL) {
		/*
		 * Need to convert a page into headers. Save the last
		 * one for the caller.
		 */
		bp = buf_pgalloc();
		n = VMPAGESIZE / sizeof(struct buffer);
#ifndef NDEBUG
		assert(n > 1);
#endif
		mutex_lock(&mutex);
		do {
			BUF_SETUP(bp);
			TAILQ_INSERT_HEAD(&bufs, bp, b_link);
			bp++;
		} while (--n > 1);
		mutex_unlock(&mutex);
	}

	mutex_lock(&m);
	busy = 0;
	if (waiters)
		cond_signal(&c);
	mutex_unlock(&m);

	return bp;
}

/*
 * Find and return the buffer that does, or should, hold the information
 * for a device "block" of data.
 */
struct buffer *
buf_getblk(struct vnode *vp, z_off_t bpgno)
{
	unsigned slot;
	struct buffer *bp, *abp, *newbp;

	newbp = NULL;
	slot = buf_hash(vp, bpgno);
	mutex_lock(&mutex);
	do {
		/*
		 * Try to find a buffer that currently maps the
		 * desired buf page.
		 */
		for (bp = TAILQ_FIRST(&bufstbl[slot]);
		     bp != NULL &&
		     bp->b_pgno < bpgno &&
		     !(bp->b_dev == vp->v_vfsp->vfs_dev ||
		       bp->b_ino == vp->v_ino);
		     bp = TAILQ_NEXT(bp, b_link))
			;
		abp = bp;
		if (bp->b_pgno != bpgno ||
		    !(bp->b_dev == vp->v_vfsp->vfs_dev ||
		      bp->b_ino == vp->v_ino))
			bp = NULL;
		if (bp != NULL)
			break;

		/*
		 * No buffer currently maps the desired region. Create
		 * the mapping now.
		 */
		if (newbp == NULL)
			newbp = TAILQ_FIRST(&bufs);
		if (newbp == NULL) {
			mutex_unlock(&mutex);
			newbp = buf_hdralloc();
			mutex_lock(&mutex);
			/*
			 * Releasing the mutex while we acquired a buf header
			 * allowed changes to be made to the buckets in the
			 * chain. We must search the chain again to make
			 * sure we are in the right place.
			 */
			continue;
		}
		bp = newbp;
		newbp = NULL;
		BUF_INIT(bp, vp, bpgno, NULL, 0, 0);
		if (abp == NULL)
			TAILQ_INSERT_TAIL(&bufstbl[slot], bp, b_link);
		else
			TAILQ_INSERT_BEFORE(abp, bp, b_link);
	} while (bp == NULL);
	if (newbp != NULL)
		TAILQ_INSERT_HEAD(&free_bufs, newbp, b_link);
	mutex_unlock(&mutex);

	return bp;
}

/*
 * Wait for IO to complete on a buffer.
 */
void
buf_iowait(struct buffer *bp)
{

	mutex_lock(&bp->b_mutex);
	while (!(bp->b_flags & B_IODONE)) {
		bp->b_flags |= B_IOWAIT;
		cond_wait(&bp->b_cond, &bp->b_mutex);
	}
	mutex_unlock(&bp->b_mutex);
}

/*
 * Signal completion of IO on a buffer.
 */
void
buf_iodone(struct buffer *bp)
{

	mutex_lock(&bp->b_mutex);
#ifndef DEBUG
	assert(!(bp->b_flags & B_IODONE));
#endif
	bp->b_flags |= B_IODONE;
	if (bp->b_flags & B_IOWAIT)
		cond_broadcast(&bp->b_cond);
	mutex_unlock(&bp->b_mutex);
}
