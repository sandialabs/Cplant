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
** $Id: me.c,v 1.19 2000/10/13 19:21:23 pumatst Exp $
*/
/*
 * me.c
 *
 * Match Entry management routines
 */

#include <stdio.h>
#include <lib-p30.h>
#include <p30/arg-blocks.h>


static void lib_me_dump( nal_cb_t *nal, lib_me_t *me, int current );

int do_PtlMEAttach( nal_cb_t *nal, void *private, void *v_args, void *v_ret )
{
	PtlMEAttach_in	*args	= v_args;
	PtlMEAttach_out	*ret	= v_ret;

	ptl_handle_ni_t	ni_h	= args->interface_in & ~0xFFFF;
	lib_ni_t	*ni	= &nal->ni;
	lib_me_t	*me	= NULL;
	lib_ptl_t	*tbl	= &ni->tbl;
	
	if( tbl->size < args->index_in || args->index_in < 0 )
		return ret->rc = PTL_INV_PTINDEX;

	/* Should check for valid matchid, but not yet */
	if( 0 )
		return ret->rc = PTL_INV_PROC;

	/* Don't destroy current matchlist, if any */
	if( tbl->tbl[args->index_in] )
		return ret->rc = PTL_INUSE;
		

	me = lib_me_alloc( nal );
	if( !me )
		return ret->rc = PTL_NOSPACE;

	me->match_id	= args->match_id_in;
	me->match_bits	= args->match_bits_in;
	me->ignore_bits	= args->ignore_bits_in;
	me->unlink	= args->unlink_in;
	me->md		= NULL;
	me->next	= NULL;
	me->prev	= &(tbl->tbl[args->index_in]);
	*(me->prev)	= me;

	/* Need to add interface to handle number */
	ret->handle_out = (me - ni->me) | ni_h;
	return ret->rc = PTL_OK;
}


int do_PtlMEInsert( nal_cb_t *nal, void *private, void *v_args, void *v_ret )
{
	PtlMEInsert_in	*args	= v_args;
	PtlMEInsert_out	*ret	= v_ret;

	lib_ni_t	*ni	= &nal->ni;
	ptl_handle_ni_t	ni_h	= args->current_in & ~0xFFFF;
	int		current	= args->current_in & 0xFFFF;
	lib_me_t	*me	= &ni->me[current];
	lib_me_t	*new;

	if( current < 0 || current > MAX_MES || !inuse( me ) )
		return ret->rc = PTL_INV_ME;

	/* Should check for valid matchid, but not yet */
	if( 0 )
		return ret->rc = PTL_INV_PROC;

	/* We never check for length of match lists */
	if( 0 )
		return ret->rc = PTL_ML_TOOLONG;

	new = lib_me_alloc( nal );
	if( !new )
		return ret->rc = PTL_NOSPACE;

	new->match_id	= args->match_id_in;
	new->match_bits	= args->match_bits_in;
	new->ignore_bits= args->ignore_bits_in;
	new->unlink	= args->unlink_in;
	new->md		= NULL;
	new->next	= NULL;
	new->prev	= NULL;

	lib_insert( me, new, args->position_in );
	ret->handle_out = (new - ni->me ) | ni_h;
	return ret->rc = PTL_OK;
}


int do_PtlMEUnlink( nal_cb_t *nal, void *private, void *v_args, void *v_ret ) {
	PtlMEUnlink_in		*args = v_args;
	PtlMEUnlink_out	*ret = v_ret;

	int		current	= args->current_in & 0xFFFF;
	lib_me_t	*me	= &nal->ni.me[current];

	if( current < 0 || current > MAX_MES || !inuse( me ) )
		return ret->rc = PTL_INV_ME;

	lib_me_unlink( nal, me );

	ret->rc = PTL_OK;
	return 0;
}

void lib_me_unlink( nal_cb_t *nal, lib_me_t *me )
{
	lib_ni_t	*ni	= &nal->ni;

	if( ni->debug & PTL_DEBUG_UNLINK )
		nal->cb_printf( nal, "%d: Unlinking ME %d\n",
			ni->rid,
			(long)( me - ni->me )
		);

	lib_unlink( nal, me );

	
	/* unlink advances the pointers for us */
	while( me->md )
		lib_md_unlink( nal, me->md );

	lib_me_free( nal, me );
}



	


int do_PtlTblDump( nal_cb_t *nal, void *private, void *v_args, void *v_ret )
{
	PtlTblDump_in	*args	= v_args;
	PtlTblDump_out	*ret	= v_ret;

	lib_me_t	*me = NULL;
	lib_ptl_t	*tbl = &nal->ni.tbl;
	
	if( tbl->size < args->index_in || args->index_in < 0 )
		return ret->rc = PTL_INV_PTINDEX;

	me = tbl->tbl[ args->index_in ];

	nal->cb_printf( nal, "Portal table index %d\n", args->index_in );

	while( me ) {
		lib_me_dump( nal, me, me - nal->ni.me );
		me = me->next;
	}

	nal->cb_printf( nal, "\n" );

	return ret->rc = PTL_OK;
}

int do_PtlMEDump( nal_cb_t *nal, void *private, void *v_args, void *v_ret )
{
	PtlMEDump_in		*args = v_args;
	PtlMEDump_out	*ret = v_ret;

	int		current	= args->current_in & 0xFFFF;
	lib_me_t	*me	= &nal->ni.me[current];

	if( current < 0 || current > MAX_MES || !inuse( me ) )
		return ret->rc = PTL_INV_ME;

	lib_me_dump( nal, me, current );
	return ret->rc = PTL_OK;
}


static void lib_me_dump(
	nal_cb_t	*nal,
	lib_me_t	*me,
	int		current
)
{
	nal->cb_printf( nal, "Match Entry %p (%d)\n", me, current );

	nal->cb_printf( nal, "\tMatch/Ignore\t= %016lx / %016lx\n",
		me->match_bits, me->ignore_bits );

	nal->cb_printf( nal, "\tMD\t= %p\n", me->md );
	nal->cb_printf( nal, "\tprev\t= %p\n", me->prev );
	nal->cb_printf( nal, "\tnext\t= %p\n", me->next );
}

