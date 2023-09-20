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
**  This Portals 3.0 program sends a message around a ring.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "p30.h"

static ptl_id_t	rank;
static int	ringtest_debug = 0;

static void test_fail( const char *str, int rc )
{
	fprintf( stderr, "%d: %s: %s (%d)\n",
		rank, str, ptl_err_str[rc], rc );
}

int main( int argc, char *argv[] )
{
	ptl_process_id_t	my_id, prev_id, next_id;
	ptl_id_t		prev,
				next;
	ptl_id_t		size;
	ptl_handle_ni_t		ni_handle;
	ptl_match_bits_t	send_mbits,
				recv_mbits,
				ibits;
	ptl_handle_me_t		me_handle;
	ptl_md_t		md;
	ptl_handle_md_t		md_handle;
	ptl_handle_eq_t		eq_handle;
	ptl_event_t		event;
	int			token,
				have_token,
				rc,
				count = 16;

	/* initialize library */
	if ( PtlInit() != PTL_OK ) {
		fprintf(stderr,"PtlInit() failed\n");
		exit(1);
	}

	/* Initialize the Interface */
	if ( PtlNIInit( PTL_IFACE_DEFAULT, 32, 2, &ni_handle ) != PTL_OK)   {
		fprintf(stderr,"PtlNIInit() failed\n");
		exit( EXIT_FAILURE );
	}

	/* get my id and size */
	if ( PtlGetId( ni_handle, &my_id, &size ) != PTL_OK ) {
		fprintf(stderr,"PtlGetId() failed\n");
		exit(1);
	}

	/* get my rank id */
	rank = my_id.rid;

	/* figure out my next neighbor's rank */
	next = (rank + 1 ) % size;

	/* figure out my previous neighbor's rank */
	prev = (rank + size - 1) % size;

	/* build previous neighbor's portal address */
	prev_id.addr_kind = PTL_ADDR_GID;
	prev_id.rid       = prev;
	prev_id.gid       = my_id.gid;

	/* build next neighbors address */
	next_id.addr_kind = PTL_ADDR_GID;
	next_id.rid       = next;
	next_id.gid       = my_id.gid;

	/* all match bits are significant */
	ibits = 0;

	/* match bits are prev rank */
	recv_mbits = (ptl_match_bits_t)prev;

	/* create a match entry */
	rc = PtlMEAttach( ni_handle,
		4,			/* portal table index         */
		prev_id,		/* source address             */
		recv_mbits,		/* expected match bits        */
		ibits,			/* ignore bits to mask        */
		PTL_UNLINK,		/* unlink when md is unlinked */
		&me_handle );
	if( rc  != PTL_OK ) {
		fprintf(stderr,"%d: PtlMEAttach() failed: %s (%d)\n",
			rank, ptl_err_str[rc], rc );
		exit( EXIT_FAILURE );
	}

	/* create an event queue */
	rc = PtlEQAlloc( ni_handle,
		2,			/* expecting 2 events on this md */
		&eq_handle );
	if( rc != PTL_OK ) {
		fprintf(stderr,"%d: PtlEQAlloc() failed: %s (%d)\n",
			rank, ptl_err_str[rc], rc );
		exit( EXIT_FAILURE );
	}

	/* create a memory descriptor */
	md.start	= &token;         	/* start address */
	md.length	= sizeof(token);  	/* length of buffer */
	md.threshold	= PTL_MD_THRESH_INF;	/* number of expected operations on md */
	md.max_offset   = md.length;
	md.options	= PTL_MD_OP_PUT
			| PTL_MD_MANAGE_REMOTE
			| PTL_MD_TRUNCATE;	/* behavior of md */
	md.user_ptr	= NULL;			/* nothing to cache */
	md.eventq	= eq_handle;		/* event queue handle */

	/* attache the memory descriptor to the match entry */
	rc = PtlMDAttach( me_handle,
		md,				/* md to attach */
		PTL_UNLINK,			/* unlink when threshold is 0 */
		&md_handle );
	if( rc != PTL_OK ) {
		fprintf(stderr,"%d: PtlMDAttach() failed: %s (%d)\n",
			rank, ptl_err_str[rc], rc );
		exit(1);
	}

	/* make sure everybody is here */
	printf( "%d: Setup.  Sync up\n", rank );

	rc = PtlNIBarrier( ni_handle );
	if( rc != PTL_OK ) {
		fprintf( stderr,"%d: PtlNIBarrier() failed: %s (%d).\n",
			rank, ptl_err_str[rc], rc );
		sleep( 2 );
	}

	printf( "%d: Out of barrier\n", rank );

	/* Rank zero gets the token first */
	have_token = rank == 0 ? 1 : 0;

	while( count > 0 ) {
		if( have_token ) {
			if( ringtest_debug || rank == 0 )
				printf( "%d: Sending on to %d (round %d)\n",
					rank, next, count );

			send_mbits = (ptl_match_bits_t)rank;

			rc = PtlPut( md_handle, PTL_NOACK_REQ,
				next_id, 4, 0, send_mbits, 0, 0 );
			if( rc != PTL_OK ) {
				test_fail( "PtlPut", rc );
				exit( EXIT_FAILURE );
			}

			/* wait for the send to complete */
			rc = PtlEQWait( eq_handle, &event );
			if( rc != PTL_OK && rc != PTL_EQ_DROPPED ) {
				test_fail( "PtlEQWait for send", rc );
				exit( EXIT_FAILURE );
			}

			/* check event type */
			if ( event.type != PTL_EVENT_SENT ) {
				fprintf(stderr,"%d: unexpected event type=%d\n",
					rank, event.type );
			}

			have_token = 0;
			count--;

		} else {

			rc = PtlEQWait( eq_handle, &event );
			if( rc != PTL_OK && rc != PTL_EQ_DROPPED ) {
				test_fail( "PtlEQWait for recv", rc );
				exit( EXIT_FAILURE );
			}

			/* check event type */
			if ( event.type != PTL_EVENT_PUT ) {
				fprintf(stderr,"%d: unexpected event type=%d\n",
					rank, event.type );
			} else {
				if( ringtest_debug )
					printf( "%d: received token from %d\n",
						rank, prev );
				have_token = 1;
			}
		}
	}

	/* close down the network interface */
	if( ringtest_debug || rank == 0 ) {
		printf( "%d: Passed all rounds\n", rank );
	}

	PtlNIFini( ni_handle );

	/* finalize library */
	PtlFini();

	return 0;
}
