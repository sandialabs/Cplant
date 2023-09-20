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
** $Id: eq.c,v 1.14 2000/12/05 23:21:47 jsotto Exp $
*/
/*
 * lib-p30/eq.c
 *
 * Library level Event queue management routines
 */

#include <lib-p30.h>
#include <p30/arg-blocks.h>


int do_PtlEQAlloc_internal( nal_cb_t *nal, void *private, void *v_args, void *v_ret )
{
	/*
	 * Incoming:
	 *	ptl_handle_ni_t ni_in
	 *	ptl_size_t count_in
	 *	void			* base_in
	 *
	 * Outgoing:
	 *	ptl_handle_eq_t		* handle_out
	 */
	
	PtlEQAlloc_internal_in	*args	= v_args;
	PtlEQAlloc_internal_out	*ret	= v_ret;

	lib_ni_t		*ni	= &nal->ni;
	ptl_handle_ni_t		ni_h	= (args->ni_in) & ~0xFFFF;
	lib_eq_t		*eq;

	if( !(eq = lib_eq_alloc( nal )) ) {
		nal->cb_invalidate( nal,
			args->base_in,
			args->count_in * sizeof(ptl_event_t), NULL  );
		return ret->rc = PTL_NOSPACE;
	}
		
	eq->sequence	= 1;
	eq->base	= args->base_in;
	eq->size	= args->count_in;
	eq->pending	= 0;

	ret->handle_out = (eq - ni->eq) | ni_h;
	return ret->rc = PTL_OK;
}


int do_PtlEQFree_internal( nal_cb_t *nal, void *private, void *v_args, void *v_ret )
{
	/*
	 * Incoming:
	 *	ptl_handle_eq_t eventq_in
	 *
	 * Outgoing:
	 */
	
	PtlEQFree_internal_in	*args	= v_args;
	PtlEQFree_internal_out	*ret	= v_ret;

	int 		current	= args->eventq_in & 0xFFFF;
	lib_eq_t	*eq	= &nal->ni.eq[current];

	if( current < 0 || current > MAX_EQS || !inuse( eq ) )
		return ret->rc = PTL_INV_EQ;

	lib_eq_unlink( nal, eq );

	return ret->rc = PTL_OK;
}

void lib_eq_unlink( nal_cb_t *nal, lib_eq_t *eq )
{
	nal->cb_invalidate( nal, eq->base, eq->size * sizeof( ptl_event_t ), NULL );
	lib_eq_free( nal, eq );
}
