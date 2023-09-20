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
#ifdef SKELFS
#include <stdlib.h>
#include <string.h>
#include "cmn.h"
#include "hash.h"

/*
 * hash: generic bucket hash implementation.
 *
 * Author: Scott Morgan, Abba Technologies (smorgan@sandia.gov)
 *
 * $Log: hash.c,v $
 * Revision 1.4  2001/07/18 18:57:39  rklundt
 * Merge IO code CNX_BUTTON branch into head
 *
 * Revision 1.3.6.1  2001/04/23 21:36:18  rklundt
 * Merge RH7 port branch into CNX_BUTTON
 *
 * Revision 1.3.4.1  2001/03/20 19:13:22  lee
 *
 *
 * Port to redhat 7
 *
 * Revision 1.3  2000/03/15 00:08:35  lward
 *
 *
 * Ported to Alpha architecture.
 *
 * Bug fix: Linux 2.2.14 (at least) doesn't like to see two files with the
 * 	same inode number. It doesn't matter that they have different
 * 	file handles (and it should) and it doesn't matter that they
 * 	have different file system identifiers. It's keying on the inode
 * 	number. To get around it, all file system ID's are the same,
 * 	0xfadedace, and the real FS id number is added to the file id number
 * 	to produce an artificial, hopefully unique, file id.
 *
 * Ported to Alpha architecture.
 *
 * Removed some unused #include directives.
 *
 * Revision 1.2  2000/02/16 01:10:50  smorgan
 *
 *
 * Added conditional compilation for the skeletal file system.
 *
 * Added conditional compilation for the skeletal file system.
 *
 * Revision 1.1  2000/02/16 00:49:39  smorgan
 *
 *
 * Addition of the skeletal file system.
 *
 *
 */

IDENTIFY("$Id: hash.c,v 1.4 2001/07/18 18:57:39 rklundt Exp $");

hash_t *
hash_create(unsigned int hash_size, unsigned int (*hash_func)(),
	    int (*comp_func)(), void (*destroy_func)())
{
	hash_t		*ret = NULL;

	if ((ret = m_alloc(sizeof(hash_t))) == NULL) {
		errno = ENOMEM;
		return ret;
	}

	mutex_init(&ret->ht_lock);

	ret->ht_size = hash_size;
	ret->ht_hash = hash_func;
	ret->ht_comp = comp_func;
	ret->ht_destroy = destroy_func;

	if ((ret->ht_bucket = m_alloc(hash_size * sizeof(hash_ent *))) == NULL) {
		(void ) mutex_destroy(&ret->ht_lock);
		free(ret);
		errno = ENOMEM;
		return NULL;
	}
	(void ) memset(ret->ht_bucket, 0, hash_size * sizeof(hash_ent *));

	return ret;
}

int
hash_insert(hash_t *htab, void *key, void *data)
{
	int			bucket;
	hash_ent		*ent;

	/* sanity check */
	if (htab == NULL || key == NULL || data == NULL) {
		errno = EINVAL;
		return -1;
	}

	if ((ent = m_alloc(sizeof(hash_ent))) == NULL) {
		errno = ENOMEM;
		return -1;
	}

	ent->hte_key = key;
	ent->hte_data = data;

	bucket = (*htab->ht_hash)(key) % htab->ht_size;

	mutex_lock(&htab->ht_lock);

	if (htab->ht_bucket[bucket] == NULL) {
		ent->hte_prev = ent->hte_next = NULL;
		htab->ht_bucket[bucket] = ent;
	}
	else {
		ent->hte_prev = NULL;
		ent->hte_next = htab->ht_bucket[bucket];
		ent->hte_next->hte_prev = ent;
		htab->ht_bucket[bucket] = ent;
	}

	(void ) mutex_unlock(&htab->ht_lock);

	return 0;
}

hash_ent *
hash_locate(hash_t *htab, void *key, unsigned int *bucket)
{
	int			b;
	hash_ent		*ent;

	b = (*htab->ht_hash)(key) % htab->ht_size;

	for (ent = htab->ht_bucket[b]; ent != NULL; ent = ent->hte_next) {
		if (!(*htab->ht_comp)(key, ent->hte_key)) {
			*bucket = b;
			break;
		}
	}

	return ent;
}

void *
hash_remove(hash_t *htab, void *key)
{
	void			*ret = NULL;
	unsigned int		bucket;
	hash_ent		*ent;

	/* sanity check */
	if (htab == NULL || key == NULL) {
		errno = EINVAL;
		return NULL;
	}

	mutex_lock(&htab->ht_lock);

	if ((ent = hash_locate(htab, key, &bucket)) == NULL) {
		errno = ENOENT;
		ret = NULL;
	}
	else {
		if (htab->ht_bucket[bucket] == ent) {
			htab->ht_bucket[bucket] = ent->hte_next;
			if (htab->ht_bucket[bucket] != NULL)
				htab->ht_bucket[bucket]->hte_prev = NULL;
		}
		else {
			ent->hte_prev->hte_next = ent->hte_next;
			if (ent->hte_next != NULL)
				ent->hte_next->hte_prev = ent->hte_prev;
		}
		ret = ent->hte_data;
	}

	free(ent);

	(void ) mutex_unlock(&htab->ht_lock);

	return ret;
}

void *
hash_get(hash_t *htab, void *key)
{
	unsigned int		b;
	void			*ret = NULL;
	hash_ent		*ent;	

	/* sanity check */
	if (htab == NULL || key == NULL) {
		errno = EINVAL;
		return NULL;
	}

	mutex_lock(&htab->ht_lock);

	if ((ent = hash_locate(htab, key, &b)) != NULL)
		ret = ent->hte_data;

	(void ) mutex_unlock(&htab->ht_lock);

	return ret;
}

void
hash_destroy(hash_t *htab)
{
	unsigned int		b;
	hash_ent		*ent;

	/* sanity check */
	if (htab == NULL)
		return;

	mutex_lock(&htab->ht_lock);

	for (b = 0; b < htab->ht_size; b++) {
		if (htab->ht_bucket[b] != NULL) {
			ent = htab->ht_bucket[b];

			while (ent != NULL) {
				hash_ent	*dead;

				if (htab->ht_destroy != NULL)
					(*htab->ht_destroy)(ent->hte_data);

				dead = ent;
				ent = ent->hte_next;
				free(dead);
			}
		}
	}

	return;
}
#endif /* SKELFS */
