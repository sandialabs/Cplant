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
#ifndef _HASH_H_
#define _HASH_H_

#include "smp.h"

/*
 * Hash table types and definitions
 *
 * Author: Scott Morgan, Abba Technologies (smorgan@sandia.gov)
 *
 * $Id: hash.h,v 1.1 2000/02/16 00:49:39 smorgan Exp $
 *
 * $Log: hash.h,v $
 * Revision 1.1  2000/02/16 00:49:39  smorgan
 *
 *
 * Addition of the skeletal file system.
 *
 *
 */

struct hash_entry {
	void			*hte_key;
	void			*hte_data;

	/* linkage */
	struct hash_entry	*hte_prev;
	struct hash_entry	*hte_next;
};

typedef struct hash_entry	hash_ent;

struct hash_table {
	mutex_t			ht_lock;

	unsigned int		ht_size;

	/* helper functions */
	unsigned int		(*ht_hash)(void *);
	int			(*ht_comp)(void *, void *);
	void			(*ht_destroy)(void *);

	hash_ent		**ht_bucket;
};

typedef struct hash_table	hash_t;

/* function interface */
extern hash_t			*hash_create(unsigned int,
					     unsigned int (*)(void *),
					     int (*)(void *, void *),
					     void (*)(void *));
extern int			hash_insert(hash_t *, void *, void *);
extern void			*hash_remove(hash_t *, void *);
extern void			*hash_get(hash_t *, void *);
extern void			hash_destroy(hash_t *);

#endif /* _HASH_H_ */
