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
** $Id: ni.c,v 1.8 2001/05/27 03:21:39 rbbrigh Exp $
*/
/*
 * lib-p30/ni.c
 *
 * Network status registers and distance functions.
 *
 */

#include <lib-p30.h>
#include <p30/arg-blocks.h>

#define MAX_DIST 18446744073709551615UL

int do_PtlNIDebug(
	nal_cb_t	*nal,
	void		*private,
	void		*v_args,
	void		*v_ret
)
{
	PtlNIDebug_in	*args	= v_args;
	PtlNIDebug_out	*ret	= v_ret;
	lib_ni_t	*ni	= &nal->ni;

	ret->rc		= ni->debug;
	ni->debug	= args->mask_in;

	return 0;
}

int do_PtlNIStatus( nal_cb_t *nal, void *private, void *v_args, void *v_ret )
{
	/*
	 * Incoming:
	 *	ptl_handle_ni_t interface_in
	 *	ptl_sr_index_t register_in
	 *
	 * Outgoing:
	 *	ptl_sr_value_t		* status_out
	 */
	
	PtlNIStatus_in	*args	= v_args;
	PtlNIStatus_out	*ret	= v_ret;
	lib_ni_t	*ni	= &nal->ni;
	lib_counters_t	*count	= &ni->counters;

	if( !args )
		return ret->rc = PTL_SEGV;

	ret->rc		= PTL_OK;
	ret->status_out	= 0;

	/*
	 * I hate this sort of code....  Hash tables, offset lists?
	 * Treat the counters as an array of ints?
	 */
	if( args->register_in == PTL_SR_DROP_COUNT )
		ret->status_out = count->drop_count;

	else if( args->register_in == PTL_SR_DROP_LENGTH )
		ret->status_out = count->drop_length;

	else if( args->register_in == PTL_SR_RECV_COUNT )
		ret->status_out = count->recv_count;

	else if( args->register_in == PTL_SR_RECV_LENGTH )
		ret->status_out = count->recv_length;

	else if( args->register_in == PTL_SR_SEND_COUNT )
		ret->status_out = count->send_count;

	else if( args->register_in == PTL_SR_SEND_LENGTH )
		ret->status_out = count->send_length;

	else if( args->register_in == PTL_SR_MSGS_MAX )
		ret->status_out = count->msgs_max;
	else
		ret->rc = PTL_INV_SR_INDX;

	return ret->rc;
}


int do_PtlNIDist( nal_cb_t *nal, void *private, void *v_args, void *v_ret )
{
	/*
	 * Incoming:
	 *	ptl_handle_ni_t interface_in
	 *	ptl_process_id_t process_in

	 *
	 * Outgoing:
	 *	unsigned long	* distance_out

	 */
	
	PtlNIDist_in	*args	= v_args;
	PtlNIDist_out	*ret	= v_ret;

	unsigned long    dist;
	ptl_process_id_t id_in  = args->process_in;
	ptl_process_id_t new_id;
	ptl_id_t         nid;
	int              rc;

	if ( id_in.addr_kind == PTL_ADDR_GID ) {
	    if ( (rc = lib_trans_id( nal, id_in, &new_id )) != PTL_OK ) {
		ret->distance_out = MAX_DIST;
		return PTL_INV_PROC;
	    }
	    nid = new_id.nid;
	} else {
	    nid = id_in.nid;
	}

	if ( (rc = nal->cb_dist( nal, nid, &dist )) != 0 ) {
	    ret->distance_out = MAX_DIST;
	    return PTL_INV_PROC;
	}

	ret->distance_out = dist;

	return ret->rc = PTL_OK;
}

