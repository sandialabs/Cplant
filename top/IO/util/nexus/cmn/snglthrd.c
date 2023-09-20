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
#ifdef SINGLE_THREAD
#include <stdlib.h>

#include "cmn.h"
#include "smp.h"

IDENTIFY("$Id: snglthrd.c,v 0.2 2001/07/18 18:57:26 rklundt Exp $");

struct _thread_key {
	void	*p;
};

int
_single_thread_key_create(thread_key_t *keyp,
			  void (*f)(void *) __attribute__((unused)))
{
	thread_key_t key;

	key = m_alloc(sizeof(struct _thread_key));
	if (key == NULL)
		return errno;
	key->p = NULL;
	*keyp = key;
	return 0;
}

int
_single_thread_key_delete(thread_key_t key)
{

	free(key);

	return 0;
}

int
_single_thread_setspecific(thread_key_t key, void *p)
{

	key->p = p;
	return 0;
}

void *
_single_thread_getspecific(thread_key_t key)
{

	return key->p;
}
#endif
