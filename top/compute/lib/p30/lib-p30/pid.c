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
** $Id: pid.c,v 1.12 2000/08/28 23:12:07 pumatst Exp $
*/
/*
 * lib-p30/pid.c
 *
 * Process identification routines
 */

/* This should be removed.  The NAL should have the PID information */
#if defined (__KERNEL__)
#	include <linux/kernel.h>
	extern int getpid(void);
#else
#	include <stdio.h>
#	include <unistd.h>
#endif
#include <lib-p30.h>
#include <p30/arg-blocks.h>


int do_PtlGetId( nal_cb_t *nal, void *private, void *v_args, void *v_ret )
{
	/*
	 * Incoming:
	 *	ptl_handle_ni_t handle_in
	 *
	 * Outgoing:
	 *	ptl_process_id_t	* id_out
	 *	ptl_id_t		* gsize_out
	 */
	
	PtlGetId_out	*ret	= v_ret;
	lib_ni_t	*ni	= &nal->ni;

	ret->id_out.addr_kind	= PTL_ADDR_BOTH;
	ret->id_out.gid		= ni->gid;
	ret->id_out.rid		= ni->rid;
	ret->id_out.nid		= ni->nid;
	ret->id_out.pid		= ni->pid;
	ret->gsize_out		= ni->num_nodes;

	return ret->rc = PTL_OK;
}

int lib_trans_id(
	nal_cb_t		*nal,
	ptl_process_id_t	id_in,
	ptl_process_id_t	*id_out
)
{
	lib_ni_t		*ni	= &nal->ni;
	int			rc;


	/* Bozo check */
	if( id_in.addr_kind == PTL_ADDR_BOTH ) {

		id_out->addr_kind	= PTL_ADDR_BOTH;
		id_out->gid		= id_in.gid;
		id_out->rid		= id_in.rid;
		id_out->nid		= id_in.nid;
		id_out->pid		= id_in.pid;

		return PTL_OK;
	}

	/* See if this is us */
	if (((id_in.addr_kind == PTL_ADDR_NID) &&
		(id_in.nid == ni->nid) && (id_in.pid == ni->pid)) ||
	    ((id_in.addr_kind == PTL_ADDR_GID) &&
		(id_in.gid == ni->gid) && (id_in.rid == ni->rid)))   {

		id_out->addr_kind	= PTL_ADDR_BOTH;
		id_out->gid		= ni->gid;
		id_out->rid		= ni->rid;
		id_out->nid		= ni->nid;
		id_out->pid		= ni->pid;

		return PTL_OK;
	}

	if( id_in.addr_kind == PTL_ADDR_GID ) {

		id_out->addr_kind	= PTL_ADDR_BOTH;
		id_out->gid		= id_in.gid;
		id_out->rid		= id_in.rid;

		rc= nal->cb_gidrid2nidpid(nal, id_in.gid, id_in.rid,
				&(id_out->nid), &(id_out->pid));

		if( rc != 0) {
		    return PTL_ADDR_UNKNOWN;
		} else   {
		    return PTL_OK;
		}
	}

	if( id_in.addr_kind == PTL_ADDR_NID ) {

		id_out->addr_kind	= PTL_ADDR_BOTH;
		id_out->nid		= id_in.nid;
		id_out->pid		= id_in.pid;

		rc= nal->cb_nidpid2gidrid(nal, id_in.nid, id_in.pid,
				&(id_out->gid), &(id_out->rid));

		if( rc != 0) {
		    return PTL_ADDR_UNKNOWN;
		} else   {
		    return PTL_OK;
		}
	}

	return PTL_ADDR_UNKNOWN;
}


int do_PtlTransId( nal_cb_t *nal, void *private, void *v_args, void *v_ret )
{
	/*
	 * Incoming:
	 *	ptl_handle_ni_t handle_in
	 *	ptl_process_id_t	id_in
	 *
	 * Outgoing:
	 *	ptl_process_id_t	* id_out
	 */
	
	PtlTransId_in	*args   = v_args;
	PtlTransId_out	*ret	= v_ret;

	return ret->rc = lib_trans_id(nal, args->id_in, &(ret->id_out) );
}
