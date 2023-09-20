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
#ifndef _RWLK_H_
#define _RWLK_H_
/*
 * Readers/Writers locking support. This implementation prefers writers
 * over readers.
 *
 * $Id: rwlk.h,v 1.1 2000/03/17 21:08:34 lward Exp $
 */
#include "smp.h"

/*
 * Readers/Writers lock types.
 */
typedef enum {
	RWLK_NONE	= 0,
	RWLK_READ	= 1,
	RWLK_WRITE	= 2
} rwty_t;

/*
 * Readers/Writers lock record.
 */
typedef struct {
	mutex_t	rw_mutex;				/* record mutex */
#ifndef NO_BIT_FIELDS
	unsigned
		rw_initialized : 1,			/* initialized? */
		rw_writer : 1;				/* write lock? */
#else
	unsigned rw_initialized;			/* initialized? */
	unsigned rw_writer;				/* write lock? */
#endif
	unsigned rw_nr;					/* # waiting readers */
	unsigned rw_nw;					/* # waiting writers */
	unsigned rw_readers;				/* # active readers */
	cond_t	rw_rcond;				/* readers cond var */
	cond_t	rw_wcond;				/* writers cond var */
} rwlock_t;

/*
 * Static intialization for a R/W lock.
 */
#define RW_INITIALIZER	{ \
	MUTEX_INITIALIZER, \
	1, \
	0, \
	0, \
	0, \
	0, \
	COND_INITIALIZER, \
	COND_INITIALIZER }

/*
 * Any lock present?
 */
#define rw_islocked(rwp)	rw_lkstat(rwp)

extern void rw_init(rwlock_t *);
extern int rw_destroy(rwlock_t *);
extern int rw_lock(rwlock_t *, rwty_t, int);
extern int rw_trylock(rwlock_t *, rwty_t, int);
extern int rw_downgrade(rwlock_t *);
extern void rw_unlock(rwlock_t *);
extern rwty_t rw_lkstat(rwlock_t *);
#endif /* defined(_RWLK_H_) */
