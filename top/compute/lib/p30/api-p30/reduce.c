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
 * $Id: reduce.c,v 1.9 2001/03/19 08:39:12 lafisk Exp $
 *
 * coll/reduce.c
 *
 * Fan-in code
 */
#define P30_COLL_INTERNAL

#include <stdio.h>
#include <stdlib.h>
#include <p30.h>
#include <p30/coll.h>
#include <p30/internal.h>
#include <time.h>

typedef struct {
	int			*value;
	ptl_process_id_t	id;
	nal_t			*nal;
} reduce_private_t;

int generic_fanin(
	void	*private,
	int	myrank,
	int	rootrank,
	int	ngroup,
	int	(*as_child)( void *private, int myrank, int partner ),
	int	(*as_root)( void *private, int myrank, int partner )
)
{
	int	i;
	int	maskrank = ( myrank - rootrank + ngroup ) % ngroup;
	int	rc = PTL_OK;

	/*
	 * Standard binary bit twidling fan-in
	 */
	for( i=1 ; i<ngroup ; i<<=1 ) {
		int partner_m	= maskrank ^ i;
		int partner_a	= ( partner_m + rootrank ) % ngroup;

		if( partner_m > maskrank ) {
			if( coll_debug )
				printf( "%d: Root to %d\n", myrank, partner_a );
			if( partner_m < ngroup ) {
				rc = as_root( private, myrank, partner_a );
				if( rc )
					return rc;
			}
		} else
			return as_child( private, myrank, partner_a );
	}

	/* Only the root node ever reaches here */
	return PTL_OK;
}


static int recv( void *private, int myrank, int partner )
{
	reduce_private_t	*reduce		= private;
	int			*value_inout	= reduce->value;
	time_t			start_time, tmout;
	ptl_event_t ev;


        tmout = *(reduce->nal->timeout);

        if (tmout){
            start_time = time(NULL);
        }


	if( coll_debug )
		printf( "%d: Waiting to hear (buf=%d)\n",
			myrank, reduce_buf );

	while( 1 ) {
		int rc;

		rc = PtlEQGet( reduce_eq, &ev );

		if( rc == PTL_EQ_EMPTY ){

                    if( tmout && 
                        (time(NULL) - start_time > tmout) ) {

                        fprintf( stderr,
                                "P30 reduce %d: Timed out (%ld seconds) waiting \n",
                                myrank,
				tmout
                        );
                        return PTL_NO_ACK;
                    }

			continue;
                }
		if( rc == PTL_EQ_DROPPED )
			fprintf( stderr, "%d: Dropped reduce event\n",
				myrank );

		if( ev.match_bits != COLLECTIVE_REDUCE_BITS )
			fprintf( stderr, "%d: Match bits do not match "
				"on reduce %lx\n", 
				myrank, ev.match_bits );

		if( ev.type == PTL_EVENT_PUT )
			break;

		if( coll_debug )
			fprintf( stderr, "%d: Non PUT event: %d\n",
				myrank, ev.type );
	}

	if( reduce_buf != *value_inout )
		fprintf( stderr, "%d: Reduce value %d != %d\n",
			myrank, reduce_buf, *value_inout );

	if( coll_debug )
		printf( "%d: Heard from nid %d rank %d\n", 
		   myrank, ev.initiator.nid, ev.initiator.rid);

	return PTL_OK;
}


static int send( void *private, int myrank, int partner )
{
	reduce_private_t	*reduce		= private;
	ptl_process_id_t	id		= reduce->id;
	int			*value_inout	= reduce->value;
	nal_t			*nal		= reduce->nal;
	int			rc;
	time_t			loop_start;
	time_t			start;

	/*
	 * We have problems with the startup delay in large
	 * runs, so it is necessary to wait for an ACK here.
	 * If the ACK is not received within some length of
	 * time (how long?) we resend the message.
	 */
	if( coll_debug )
		printf( "%d: Sending to %d (buf=%d)\n",
			myrank, partner, reduce_buf );

	id.addr_kind = PTL_ADDR_GID;
	id.rid = partner;
	reduce_buf = *value_inout;

	start = time( NULL );
	
retry:
	rc = PtlPut( reduce_md, PTL_ACK_REQ, id,
		COLLECTIVE_PORTAL, 0,
		COLLECTIVE_REDUCE_BITS, 0,
		0 );

	loop_start = time( NULL );

	while( time(NULL) - loop_start < 2 )
	{
		ptl_event_t ev;
		int rc = PtlEQGet( reduce_eq, &ev );
		if( rc == PTL_EQ_EMPTY ) {
			if( nal->yield )
				nal->yield( nal );
			continue;
		}

		if( rc == PTL_EQ_DROPPED )
			fprintf( stderr, "%d: Dropped reduce event\n",
				myrank );

		if( ev.match_bits != COLLECTIVE_REDUCE_BITS )
			fprintf( stderr,
				"%d: Match bits do not match on reduce %lx\n",
				myrank, ev.match_bits );

		if( ev.type == PTL_EVENT_ACK ) {
			if( coll_debug )
				fprintf( stderr, "%d: Received ACK from %d\n",
					myrank, partner );
			return PTL_OK;
		}

		if( coll_debug )
			fprintf( stderr, "%d: Non ACK event: %d\n",
				myrank, ev.type );

	}

	if( time(NULL) - start < 10 )
		goto retry;

	fprintf( stderr, "%d: Too many time outs waiting for node %d\n",
		myrank, partner );

	return PTL_NO_ACK;
}


int PtlReduce_all(
	ptl_handle_ni_t	ni,
	int		rootrank,
	int		*value_inout
)
{
	ptl_id_t		ngroup;
	reduce_private_t	reduce;
	int			rc;

	if( !coll_did_init ) {
		rc = ptl_coll_init( ni );
		if( rc != PTL_OK )
			return rc;
	}

	PtlGetId( ni, &reduce.id, &ngroup );

	reduce_buf	= *value_inout;
	reduce.value	= value_inout;
	reduce.nal	= ptl_interfaces[ (ni >> 16) & 0xF ];
	
	rc = generic_fanin(
		&reduce,		/* Private data for this reduce */
		reduce.id.rid,		/* My rank */
		rootrank,		/* Root (for this reduce)'s rank */
		ngroup,			/* # of nodes participating */
		send,			/* when a child */
		recv			/* when a parent */
	);

	return rc;
}
