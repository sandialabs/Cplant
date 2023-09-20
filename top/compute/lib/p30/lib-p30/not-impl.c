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
** $Id: not-impl.c,v 1.6 2001/05/24 23:20:33 rbbrigh Exp $
*/
/*
 * boiler plate functions that can be used to write the 
 * library side routines
 */

#include <lib-p30.h>
#include <p30/arg-blocks.h>


int do_PtlACEntry( nal_cb_t *nal, void *private, void *v_args, void *v_ret )
{
	/*
	 * Incoming:
	 *	ptl_handle_ni_t ni_in
	 *	ptl_ac_index_t index_in
	 *	ptl_process_id_t match_id_in
	 *	ptl_pt_index_t portal_in

	 *
	 * Outgoing:

	 */
	
	PtlACEntry_in	*args	= v_args;
	PtlACEntry_out	*ret	= v_ret;

	if( !args )
		return ret->rc = PTL_SEGV;

	return ret->rc = PTL_NOT_IMPLEMENTED;
}


