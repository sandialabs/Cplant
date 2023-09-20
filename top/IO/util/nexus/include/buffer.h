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
#include "smp.h"
#include "queue.h"
#include "ztypes.h"

/*
 * In-core data buffer header.
 */
struct buffer {
	TAILQ_ENTRY(buffer) b_link;			/* list link */
	unsigned b_flags;				/* flags (see below) */
	z_dev_t b_dev;					/* owner device */
	z_ino_t	b_ino;					/* owner inode */
	z_off_t	b_pgno;					/* file page number */
	size_t	b_off;					/* region offset */
	size_t	b_len;					/* region length */
	char	*b_data;				/* data */
	size_t	b_size;					/* size of b_data */
	int	b_error;				/* error code */
	mutex_t	b_mutex;				/* lock */
	cond_t	b_lkcnd;				/* lock waiters */
	cond_t	b_iocnd;				/* IO waiters */
};

/*
 * Flags values found in b_flags above.
 */
#define B_LOCKED	0x0001				/* buffer locked */
#define B_IODONE	0x0002				/* IO complete */
#define B_IOWAIT	0x0004				/* has IO waiters */
#define B_DIRTY		0x0010				/* dirty buffer */

/*
 * (Re)initialize a buffer header.
 */
#define BUF_INIT(bp, vp, dev, ino, bpgno, data, size) \
	do { \
		(bp)->b_flags = 0; \
		(bp)->b_dev = (dev); \
		(bp)->b_ino = (ino); \
		(bp)->b_pgno = (bpgno); \
		(bp)->b_off = 0; \
		(bp)->b_len = 0; \
		(bp)->b_data = (data); \
		(bp)->b_size = (size); \
		(bp)->b_error = 0; \
	} while (0)

/*
 * Set up a new buffer header.
 */
#define BUF_SETUP(bp) \
	do { \
		BUF_INIT((bp), NULL, NODEV, NOINO, 0, NULL, 0); \
		mutex_init(&(bp)->b_mutex); \
		cond_init(&(bp)->b_lkcnd); \
		cond_init(&(bp)->b_iocnd); \
	} while (0)

/*
 * Lock a buffer.
 */
#define buf_lock(bp) \
	do { \
		mutex_lock(&(bp)->b_mutex); \
		while ((bp)->b_flags & B_LOCKED) \
			cond_wait(&(bp)->b_lkcnd, &(bp)->b_mutex); \
		mutex_unlock(&(bp)->b_mutex); \
	} while (0)

/*
 * Unlock a buffer.
 */
#define buf_unlock(bp) \
	do { \
		mutex_lock(&(bp)->b_mutex); \
		cond_signal(&(bp)->b_lkcnd); \
		mutex_unlock(&(bp)->b_mutex); \
	} while (0)

/*
 * Wait for IO to complete on a buffer.
 */
#define buf_iowait(bp) \
	do { \
		mutex_lock(&(bp)->b_mutex); \
		while (!((bp)->b_flags & B_IODONE)) \
			cond_wait(&(bp)->b_iocnd, &(bp)->b_mutex); \
		mutex_unlock(&(bp)->b_mutex); \
	} while (0)

/*
 * Signal completion of IO to waiters.
 */
#define buf_iodone(bp) \
	do { \
		mutex_lock(&(bp)->b_mutex); \
		cond_broadcast(&(bp)->b_iocnd); \
		mutex_unlock(&(bp)->b_mutex); \
	} while (0)

struct vnode;

extern void buf_init(void *, size_t);
extern struct buffer *buf_getblk(struct vnode *, z_off_t);
