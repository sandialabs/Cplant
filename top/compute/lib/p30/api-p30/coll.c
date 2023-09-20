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
 * api-p30/coll.c
 *
 * Setup collective communication portals and a few generic routines
 * 
 */
#define P30_COLL_INTERNAL

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <p30.h>
#include <p30/coll.h>

int		coll_did_init	= 0;
int		coll_debug	= 0;
volatile int	bcast_buf;
volatile int	reduce_buf;

ptl_handle_md_t	bcast_md, reduce_md;
ptl_handle_eq_t	bcast_eq, reduce_eq;
ptl_handle_me_t	bcast_me, reduce_me;

int ptl_coll_init( ptl_handle_ni_t ni )
{
	int			rc;
	ptl_md_t		md_real;
	ptl_process_id_t	id;
	ptl_id_t		nodes;
	int myrank;

	if( coll_did_init )
		return PTL_OK;

	PtlGetId( ni, &id, &nodes );
	myrank = id.rid;
	id.addr_kind = PTL_ADDR_GID;
	id.rid = PTL_ID_ANY;

	/*
	 * Setup for broadcast operations
	 */
	rc = PtlMEAttach( ni,
		COLLECTIVE_PORTAL,
		id,
		COLLECTIVE_BCAST_BITS, 0x0,
		PTL_RETAIN, &bcast_me );
	if( rc )
		goto fail_bcast_me;

	rc = PtlEQAlloc( ni, nodes * 4, &bcast_eq );
	if( rc )
		goto fail_bcast_eq;

	md_real.start		= (void*) &bcast_buf;
	md_real.length		= sizeof( bcast_buf );
	md_real.threshold	= -1;
	md_real.options		= PTL_MD_OP_PUT
				| PTL_MD_MANAGE_REMOTE
				| PTL_MD_TRUNCATE;
	md_real.user_ptr	= NULL;
	md_real.eventq		= bcast_eq;

	rc = PtlMDAttach( bcast_me, md_real, PTL_RETAIN, &bcast_md );
	if( rc )
		goto fail_bcast_md;

	/*
	 * Setup for reduce operations
	 */
	rc = PtlMEInsert( bcast_me,
		id,
		COLLECTIVE_REDUCE_BITS, 0x0,
		PTL_RETAIN, PTL_INS_AFTER,
		&reduce_me );
	if( rc )
		goto fail_reduce_me;

	rc = PtlEQAlloc( ni, nodes * 4, &reduce_eq );
	if( rc )
		goto fail_reduce_eq;

	md_real.start		= (void*) &reduce_buf;
	md_real.length		= sizeof( reduce_buf );
	md_real.eventq		= reduce_eq;

	rc = PtlMDAttach( reduce_me, md_real, PTL_RETAIN, &reduce_md );
	if( rc )
		goto fail_reduce_md;

	coll_did_init++;
	if( coll_debug )
		printf( "%d: bcast: buf=%p eq=%d reduce: buf=%p eq=%d\n",
			myrank, &bcast_buf, bcast_eq, &reduce_buf, reduce_eq );
	return PTL_OK;

	/*
	 * Failure cleanup
	 */
	PtlMDUnlink( reduce_md );
fail_reduce_md:
	PtlEQFree(   reduce_eq );
fail_reduce_eq:
	PtlMEUnlink( reduce_me );
fail_reduce_me:
	PtlMDUnlink( bcast_md );
fail_bcast_md:
	PtlEQFree(   bcast_eq );
fail_bcast_eq:
	PtlMEUnlink( bcast_me );
fail_bcast_me:

	return rc;
}


int ptl_coll_fini( ptl_handle_ni_t ni )
{
	coll_did_init = 0;
	PtlMDUnlink( reduce_md );
	PtlEQFree(   reduce_eq );
	PtlMEUnlink( reduce_me );
	PtlMDUnlink( bcast_md );
	PtlEQFree(   bcast_eq );
	PtlMEUnlink( bcast_me );

	return PTL_OK;
}


int PtlNIBarrier( ptl_handle_ni_t ni )
{
	static int	barrier_count	= 0;
	int		rc;

	rc = PtlReduce_all( ni, 0, &barrier_count );
	if( rc ) {
		fprintf( stderr, "Reduce operation failed in barrier!\n" );
		return rc;
	}

	rc = PtlBroadcast_all( ni, 0, &barrier_count );

	return rc;
}
