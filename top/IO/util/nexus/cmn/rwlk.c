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
/*
 * Readers/Writers lock support. Writers preferred over readers.
 */
#include <stdlib.h>
#include <string.h>

#include "cmn.h"
#include "smp.h"
#include "rwlk.h"
#include "queue.h"

IDENTIFY("$Id: rwlk.c,v 0.4 2001/07/18 18:57:26 rklundt Exp $");

/*
 * Each active lock held by a thread is recorded in a list held by the
 * threads' specific data area.
 */
struct locks_list_element {
	rwlock_t *rwlp;					/* ptr to lock */
	rwty_t	lkf;					/* type of lock held */
	LIST_ENTRY(locks_list_element) link;		/* next lock in list */
#ifdef DEBUG_RWLK
	LIST_ENTRY(locks_list_element) link2;		/* next in glbl list */
#ifndef SINGLE_THREAD
	thread_t tid;					/* thread ID */
#endif
#endif
};

LIST_HEAD(locks_list_head, locks_list_element);

static mutex_t mutex = MUTEX_INITIALIZER;		/* package mutex */
static struct locks_list_head free_elements =		/* free list elements */
	LIST_HEAD_INITIALIZER(&free_elements);
#ifdef DEBUG_RWLK
static struct locks_list_head locks =			/* all locks in use */
	LIST_HEAD_INITIALIZER(&locks);
#endif

static once_t once_control = ONCE_INIT;			/* for key init */
static thread_key_t key;				/* locks list key */
static int have_locks_list_key = 0;			/* key initialized? */

/*
 * initialize a lock.
 *
 * NB: Not thread safe.
 */
void
rw_init(rwlock_t *rwlp)
{

	if (rwlp->rw_initialized)
		panic("rw_init: attempt to reinit");
	mutex_init(&rwlp->rw_mutex);
	rwlp->rw_writer = 0;
	rwlp->rw_nr = rwlp->rw_nw = 0;
	rwlp->rw_readers = 0;
	rwlp->rw_writer = 0;
	cond_init(&rwlp->rw_rcond);
	cond_init(&rwlp->rw_wcond);
	rwlp->rw_initialized = 1;
}

/*
 * Destroy a lock.
 *
 * NB: Not thread safe.
 */
int
rw_destroy(rwlock_t *rwlp)
{
	int	err;

	err = _mutex_lock(&rwlp->rw_mutex);
	if (err)
		return 0;
	err =
	    rwlp->rw_nr || rwlp->rw_nw || rwlp->rw_readers || rwlp->rw_writer
	      ? EBUSY
	      : 0;
	if (!err)
		rwlp->rw_initialized = 0;
	mutex_unlock(&rwlp->rw_mutex);
	while (_mutex_destroy(&rwlp->rw_mutex) != 0)
		;
	cond_destroy(&rwlp->rw_rcond);
	cond_destroy(&rwlp->rw_rcond);
	return 0;
}

/*
 * Acquire the indicated lock.
 *
 * NB: The caller must hold the record mutex.
 */
static int
get_lock(rwlock_t *rwlp, rwty_t lkf, int dontwait)
{
	int	err;

	if (!rwlp->rw_initialized)
		return EINVAL;
	err = 0;
	switch (lkf) {
	case RWLK_READ:
		assert(++rwlp->rw_nr);
		while (!dontwait &&
		       (rwlp->rw_nw || rwlp->rw_writer))
			cond_wait(&rwlp->rw_rcond, &rwlp->rw_mutex);
		rwlp->rw_nr--;
		if (rwlp->rw_nw || rwlp->rw_writer) {
			err = EBUSY;
			break;
		}
		assert(++rwlp->rw_readers);
#if defined(PARANOID) && defined(SINGLE_THREAD)
		if (rwlp->rw_readers > 1)
			panic("get_lock: recursive read lock");
#endif
		break;
	case RWLK_WRITE:
		assert(++rwlp->rw_nw);
		while (!dontwait &&
		       (rwlp->rw_readers || rwlp->rw_writer))
			cond_wait(&rwlp->rw_wcond, &rwlp->rw_mutex);
		rwlp->rw_nw--;
		if (rwlp->rw_readers || rwlp->rw_writer) {
			err = EBUSY;
			break;
		}
		rwlp->rw_writer = 1;
		break;
	default:
		err = EINVAL;
	}

	return err;
}

/*
 * Clean up associated thread-specific data at thread exit.
 */
static void
cleanup_locks_list(struct locks_list_head *head)
{

	if (LIST_FIRST(head) != NULL)
		panic("cleanup_locks_list: thread still holds locks at exit");
	free(head);
}

/*
 * Create thread-specific data key for lists of locks.
 */
static void
create_locks_list_key(void)
{

	thread_key_create(&key, (void (*)(void *))cleanup_locks_list);
	have_locks_list_key = 1;
}

/*
 * Allocate a locks list element.
 */
struct locks_list_element *
alloc_locks_list_element(void)
{
	struct locks_list_element *elem;
	unsigned indx;
	static size_t nelem = 64;

	mutex_lock(&mutex);
	elem = LIST_FIRST(&free_elements);
	if (elem == NULL) {
		if (nelem > 1024)
			nelem = 1024;
		elem = m_alloc(nelem * sizeof(struct locks_list_element));
		if (elem != NULL) {
#ifdef PARANOID
			(void )memset(elem,
				      0,
				      (nelem *
				       sizeof(struct locks_list_element)));
#endif
			for (indx = 1; indx < nelem; indx++) {
				LIST_INSERT_HEAD(&free_elements,
						 elem,
						 link);
				elem++;
			}
		}
		nelem <<= 1;
	} else
		LIST_REMOVE(elem, link);
	mutex_unlock(&mutex);

	return elem;
}

/*
 * Free a locks list element.
 */
static void
free_locks_list_element(struct locks_list_element *elem)
{

#ifdef PARANOID
	(void )memset(elem, 0, sizeof(struct locks_list_element));
#endif
	mutex_lock(&mutex);
	LIST_INSERT_HEAD(&free_elements, elem, link);
	mutex_unlock(&mutex);
}

int
rw_lock(rwlock_t *rwlp, rwty_t lkf, int dontwait)
{
	int	err;
	struct locks_list_head *head;
	struct locks_list_element *elem;

	if (!have_locks_list_key)
		thread_once(&once_control, create_locks_list_key);

	head = thread_getspecific(key);
	if (head == NULL) {
		head = m_alloc(sizeof(struct locks_list_head));
		if (head == NULL)
			return ENOMEM;
		LIST_INIT(head);
		thread_setspecific(key, head);
	}

	elem = alloc_locks_list_element();
	if (elem == NULL)
		return ENOMEM;
	elem->rwlp = rwlp;
	elem->lkf = lkf;
#if defined(DEBUG_RWLK) && !defined(SINGLE_THREAD)
	elem->tid = thread_self();
#endif

	err = _mutex_lock(&rwlp->rw_mutex);
	if (err) {
		free_locks_list_element(elem);
		return err;
	}
	err = get_lock(rwlp, lkf, dontwait);
#ifdef DEBUG_RWLK
	if (!err)
		LIST_INSERT_HEAD(&locks, elem, link2);
#endif
	mutex_unlock(&rwlp->rw_mutex);

	if (!err)
		LIST_INSERT_HEAD(head, elem, link);
	else
		free_locks_list_element(elem);

	return err;
}

/*
 * Search for a given lock in the thread's list.
 */
static struct locks_list_element *
find_locks_list_element(rwlock_t *rwlp)
{
	struct locks_list_head *head;
	struct locks_list_element *elem;

	head = thread_getspecific(key);
	if (head == NULL)
		return NULL;
	for (elem = LIST_FIRST(head);
	     elem != NULL && elem->rwlp != rwlp;
	     elem = LIST_NEXT(elem, link))
		;
	return elem;
}

#ifdef notdef
int
rw_downgrade(rwlock_t *rwlp)
{
	int	err;
	struct locks_list_element *elem;

	elem = find_locks_list_element(rwlp);
	if (elem == NULL)
		return EINVAL;

	err = _mutex_lock(&rwlp->rw_mutex);
	if (err)
		return err;
	if (!rwlp->rw_initialized) {
		mutex_unlock(&rwlp->rw_mutex);
		return EINVAL;
	}
	do {
		if (!rwlp->rw_writer) {
			err = EINVAL;
			break;
		}
		rwlp->rw_writer = 0;
		assert(++rwlp->rw_nr);
	} while (0);
	if (!err) {
		if (rwlp->rw_nw)
			cond_signal(&rwlp->rw_wcond);
		else if (rwlp->rw_nr)
			cond_broadcast(&rwlp->rw_rcond);
	}
	while (rwlp->rw_nw)
		cond_wait(&rwlp->rw_wcond, &rwlp->rw_mutex);
	assert(rwlp->rw_nr--);
	assert(++rwlp->rw_readers);
	mutex_unlock(&rwlp->rw_mutex);

	elem->lkf = RWLK_READ;

	return err;
}
#endif

void
rw_unlock(rwlock_t *rwlp)
{
	struct locks_list_element *elem;

	elem = find_locks_list_element(rwlp);
	if (elem == NULL)
		panic("rw_unlock: attempt to unlock another's lock");

	mutex_lock(&rwlp->rw_mutex);
	if (!rwlp->rw_initialized)
		panic("rw_unlock: attempt to unlock uninitalized lock");
	if (rwlp->rw_writer)
		rwlp->rw_writer = 0;
	else
		assert(rwlp->rw_readers--);
	if (rwlp->rw_nw) {
		if (!rwlp->rw_readers)
			cond_signal(&rwlp->rw_wcond);
	} else if (rwlp->rw_nr)
		cond_broadcast(&rwlp->rw_rcond);
#ifdef DEBUG_RWLK
	LIST_REMOVE(elem, link2);
#endif
	mutex_unlock(&rwlp->rw_mutex);

	LIST_REMOVE(elem, link);
	free_locks_list_element(elem);
}

rwty_t
rw_lkstat(rwlock_t *rwlp)
{
	struct locks_list_element *elem;

	elem = find_locks_list_element(rwlp);
	if (elem == NULL)
		return RWLK_NONE;

	return elem->lkf;
}

#ifdef DEBUG_RWLK
unsigned
rw_dpylks(void)
{
	unsigned count;
	struct locks_list_element *elem;
	static const char *lkfnames[] = {
		"RWLK_NONE",
		"RWLK_READ",
		"RWLK_WRITE"
	};

	mutex_lock(&mutex);
	for (count = 0, elem = LIST_FIRST(&locks);
	     elem != NULL;
	     count++, elem = LIST_NEXT(elem, link2)) {
#ifndef SINGLE_THREAD
		LOG(LOG_DEBUG,
		    "rwlp %p, thread " THREAD_T_FMT ", lkf %s",
		    elem->rwlp,
		    elem->tid,
		    lkfnames[elem->lkf]);
#else
		LOG(LOG_DEBUG,
		    "rwlp %p, thread, lkf %s",
		    elem->rwlp,
		    lkfnames[elem->lkf]);
#endif
	}
	mutex_unlock(&mutex);

	return count;
}
#endif
