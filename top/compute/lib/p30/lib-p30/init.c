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
** $Id: init.c,v 1.42 2001/02/20 21:01:52 rolf Exp $
*/
/*
 * lib-p30/init.c
 *
 * Start up the internal library and clear all structures
 * Called by the NAL when it initializes.  Safe to call multiple
 * times.
 */

#include <lib-p30.h>
#ifdef __KERNEL__
#	include <linux/string.h>	/* for memset() */
#ifdef KERNEL_ADDR_CACHE
#	include <compute/OS/addrCache/cache.h>
#endif
#else
#	include <string.h>
#endif


int lib_init(
	nal_cb_t	*nal,
	int		nid,
	int		pid,
	int		rid,
	int		gid,
	int		gsize,
	ptl_pt_index_t	ptl_size,
	ptl_ac_index_t	acl_size
)
{
	int i;

	lib_ni_t *ni = &nal->ni;

	/*
	 * Allocate the portal table for this interface
	 * and all per-interface objects.
	 */
	memset( &ni->counters, 0, sizeof( lib_counters_t ) );
	
	ni->nid		= nid;
	ni->rid		= rid;
	ni->gid		= gid;
	ni->pid		= pid;

	ni->num_nodes	= gsize;
	ni->tbl.size	= ptl_size;
	ni->tbl.tbl	= nal->cb_malloc( nal, sizeof( lib_me_t * ) * ptl_size );
	if (!ni->tbl.tbl)
		goto fail_tbl;
	ni->eq		= nal->cb_malloc( nal, sizeof( lib_eq_t ) * MAX_EQS );
	if (!ni->eq)
		goto fail_eq;
	ni->me		= nal->cb_malloc( nal, sizeof( lib_me_t ) * MAX_MES );
	if (!ni->me)
		goto fail_me;
	ni->md		= nal->cb_malloc( nal, sizeof( lib_md_t ) * MAX_MDS );
	if (!ni->md)
		goto fail_md;
	ni->msg		= nal->cb_malloc( nal, sizeof( lib_msg_t ) * MAX_MSGS );
	if (!ni->msg)
		goto fail_msg;

	ni->free_eq	= (void*) 0xDEADBEEF;
	ni->free_me	= (void*) 0xDEADBEEF;
	ni->free_md	= (void*) 0xDEADBEEF;
	ni->free_msg	= (void*) 0xDEADBEEF;

	for( i=0 ; i<ptl_size ; i++ )
		ni->tbl.tbl[i] = NULL;

	for( i=0 ; i < MAX_MES ; i++ ) {
		ni->me[i].next_free	= ni->free_me;
		ni->free_me		= &ni->me[i];
	}

	for( i=0 ; i < MAX_MDS ; i++ ) {
		ni->md[i].next_free	= ni->free_md;
		ni->free_md		= &ni->md[i];
	}

	for( i=0 ; i < MAX_EQS ; i++ ) {
		ni->eq[i].next_free	= ni->free_eq;
		ni->free_eq		= &ni->eq[i];
	}

	for( i=0 ; i < MAX_MSGS ; i++ ) {
		ni->msg[i].next_free	= ni->free_msg;
		ni->free_msg		= &ni->msg[i];
	}

	ni->debug = PTL_DEBUG_NONE;

	ni->up       = 1;

#ifdef __KERNEL__
#ifdef KERNEL_ADDR_CACHE
        addrCache_tblInit(pid);
#endif
#endif
	return PTL_OK;

	/* Failure cleanup */
fail_msg:
	nal->cb_free(nal, ni->md);
fail_md:
	nal->cb_free(nal, ni->me);
fail_me:
	nal->cb_free(nal, ni->eq);
fail_eq:
	nal->cb_free(nal, ni->tbl.tbl);
fail_tbl:
	return PTL_NOSPACE;
	
}


int lib_fini( nal_cb_t *nal )
{
	int		i;

	lib_ni_t	*ni = &nal->ni;

	for( i=0 ; i<MAX_MSGS ; i++ )
		if( inuse( &ni->msg[i] ) ) {
			#ifdef VERBOSE
			nal->cb_printf( nal, "%d: Message %d is still inuse\n",
				nal->ni.rid, i );
			#endif VERBOSE
			lib_msg_free( nal, &ni->msg[i] );
		}

	for( i=0 ; i<MAX_EQS ; i++ )
		if( inuse( &ni->eq[i] ) )
			lib_eq_unlink( nal, &ni->eq[i] );

	for( i=0 ; i<MAX_MDS ; i++ )
		if( inuse( &ni->md[i] ) ) {
			lib_md_unlink( nal, &ni->md[i] );
		}


	for( i=0 ; i<MAX_MES ; i++ )
		if( inuse( &ni->me[i] ) )
			lib_me_free( nal, &ni->me[i] );
	
	nal->cb_free( nal, ni->msg );
	nal->cb_free( nal, ni->md );
	nal->cb_free( nal, ni->me );
	nal->cb_free( nal, ni->eq );
	nal->cb_free( nal, ni->tbl.tbl );

	ni->up = 0;

#ifdef __KERNEL__
#ifdef KERNEL_ADDR_CACHE
        /* clear the address cache */
        addrCache_tblClear(ni->pid);
#endif
#endif

	return 0;
}


/*
 * This really should be elsewhere, but lib-p30/dispatch.c is
 * an automatically generated file.
 */
void lib_dispatch(
	nal_cb_t	*nal,
	void		*private,
	int		index,
	void		*arg_block,
	void		*ret_block )
{
	unsigned long	flags;
	lib_ni_t	*ni	= &nal->ni;

			
	if( index < 0 || index > LIB_MAX_DISPATCH || !dispatch_table[index].fun )
	{
		nal->cb_printf( nal,
			"%d: Invalid API call %d\n",
			ni->rid,
			index
		);
		return;
	}

	if( ni->debug & PTL_DEBUG_API )
		nal->cb_printf( nal,
			"%d: API call %s (%d)\n",
			ni->rid,
			dispatch_table[index].name,
			index
		);

	nal->cb_cli(nal,&flags);
	dispatch_table[index].fun( nal, private, arg_block, ret_block );
	nal->cb_sti(nal,&flags);
}

char *dispatch_name( int index )
{
    return dispatch_table[index].name;
}
