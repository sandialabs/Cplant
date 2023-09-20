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
 * $Id: bcast.c,v 1.7 2001/11/06 15:36:56 lafisk Exp $
 *
 * coll/bcast.c
 *
 * Broadcast code
 */
#define P30_COLL_INTERNAL

#include <stdio.h>
#include <stdlib.h>
#include <p30.h>
#include <p30/coll.h>
#include <p30/internal.h>
#include <time.h>

int generic_fanout(
	void	*private,
	int	myrank,
	int	root,
	int	ngroup,
	int	(*as_child)( void *private, int myrank, int partner ),
	int	(*as_root)( void *private, int myrank, int partner )
)
{
	int	rc	= PTL_OK;

	/*
	** Just reverse the fanin from reduce.c.  When fanin/fanout operations happen 
        ** in rapid fire succession, they can fail if the fanin algorithm is not the
        ** same as the fanout algorithm.
	*/

	int recvfrom=-1;
	int sendto[16], nsends=0;

        int     i;
        int     maskrank = ( myrank - root + ngroup ) % ngroup;

        for( i=1 ; i<ngroup ; i<<=1 ) {
                int partner_m   = maskrank ^ i;
                int partner_a   = ( partner_m + root) % ngroup;

                if( partner_m > maskrank ) {
                     if (partner_m < ngroup) sendto[nsends++] = partner_a;
                } else{
                     recvfrom = partner_a;
                     break;
                }
        }

        if (recvfrom >= 0){
            rc = as_child(private, myrank, recvfrom);
        }

        if (rc != PTL_OK) return rc;

        for (i=0; i<nsends; i++){

             rc = as_root(private, myrank, sendto[i]);

	     if (rc != PTL_OK) break;
        }

	return rc;
}

typedef struct {
	ptl_process_id_t	id;
	nal_t                  *nal;
} bcast_private_t;



static int send( void *private, int myrank, int partner )
{
	bcast_private_t		*bcast	= private;
	ptl_process_id_t	id	= bcast->id;
	int			rc;

	if( coll_debug )
		printf( "%d: Sending to %d\n", myrank, partner );

	id.addr_kind = PTL_ADDR_GID;
	id.rid = partner;

	rc = PtlPut( bcast_md, PTL_NOACK_REQ, id,
		COLLECTIVE_PORTAL, 0,
		COLLECTIVE_BCAST_BITS, 0,
		0 /* Header data */ );
	if( rc != PTL_OK ) {
		fprintf( stderr, "%d: Failed on bcast put\n", myrank );
		return rc;
	}

	return PTL_OK;
}

static int recv( void *private, int myrank, int partner )
{
    time_t start_time, tmout;
    bcast_private_t		*bcast	= private;
    ptl_event_t ev;

    tmout = *(bcast->nal->timeout);

    if (tmout){
        start_time = time(NULL);
    }

	/* Receive from the root and then resend */
	if( coll_debug )
		printf( "%d: waiting to receive\n", myrank );

	while( 1 ) {
		int rc = PtlEQGet( bcast_eq, &ev );

		if( rc == PTL_EQ_EMPTY ){

                    if( tmout && 
                        (time(NULL) - start_time > tmout) ) {

                        fprintf( stderr,
                                "P30 broadcast %d: Timed out waiting for incoming\n", myrank);
                        
                        return PTL_NO_ACK;
                    }

		    continue;
                }
		if( rc == PTL_EQ_DROPPED )
			fprintf( stderr, "%d: Dropped bcast eq\n", myrank );
		if( ev.match_bits != COLLECTIVE_BCAST_BITS )
			fprintf( stderr, "%d: Match bits do not "
				"match on bcast %lx\n",
				myrank, ev.match_bits );

		if( ev.type == PTL_EVENT_PUT )
			break;

		if( coll_debug )
			fprintf( stderr, "%d: Non PUT event: %d\n",
				myrank, ev.type );
	}

	if( coll_debug )
		printf( "%d: Received from nid %d rank %d\n", 
                       myrank, ev.initiator.nid, ev.initiator.rid);

	return PTL_OK;
}

int PtlBroadcast_all(
	ptl_handle_ni_t	ni,
	int		root,
	int		*value_inout
)
{
	bcast_private_t		bcast;
	ptl_id_t		ngroup;
	int			rc;

	if( !coll_did_init ) {
		rc = ptl_coll_init( ni );
		if( rc != PTL_OK )
			return rc;
	}

	PtlGetId( ni, &bcast.id, &ngroup );

	bcast.nal = ptl_interfaces[ (ni >> 16) & 0xF ];

        if (bcast.id.rid == root){
	    bcast_buf = *value_inout;
        }

	rc= generic_fanout( &bcast, bcast.id.rid, root, ngroup, recv, send );

	*value_inout = bcast_buf;

	return rc;
}
