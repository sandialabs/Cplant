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
**  This Portals 3.0 program gets an ack from rank 1 to rank 0
*/

#include<stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include"p30.h"

int main( int argc, char *argv[] )
{

    ptl_process_id_t my_id, src_id, dest_id;
    ptl_id_t         rank;
    ptl_id_t         size;
    ptl_handle_ni_t  ni_handle;
    ptl_handle_me_t  me_handle;
    ptl_md_t         md;
    ptl_handle_md_t  md_handle;
    int              token;
    ptl_handle_eq_t  eq_handle;
    ptl_event_t      event;
    int              rc;

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

    /* create an event queue */
    if ( PtlEQAlloc( ni_handle,
		     5,
		     &eq_handle ) != PTL_OK ) {
	fprintf(stderr,"%d: PtlEQAlloc() failed\n",rank);
	exit(1);
    }

    if ( rank == 0 ) {

	/* create a memory descriptor */
	md.start      = &token;         /* start address                       */
	md.length     = sizeof(token);  /* length of buffer                    */
	md.threshold  = 2 ;             /* number of expected operations on md */
	md.max_offset = md.length;
	md.options    = 0;              /* behavior of md                      */
	md.user_ptr   = NULL;           /* nothing to cache                    */
	md.eventq     = eq_handle;      /* event queue handle                  */

	/* build dest portal address */
	dest_id.addr_kind = PTL_ADDR_GID;
	dest_id.rid       = 1;
	dest_id.gid       = my_id.gid;

	if ( PtlMDBind( ni_handle,
			md,
			&md_handle ) != PTL_OK ) {
	    fprintf(stderr,"PtlMDBind() failed\n");
	    exit(1);
	}

	/* wait for 1 to get set up */
	sleep(5);

	fprintf(stderr,"%d: calling PtlPut()\n",rank);

	if ( PtlPut( md_handle,
		     PTL_ACK_REQ,
		     dest_id,
		     4,
		     0,
		     0,
		     0, 14 ) != PTL_OK ) {
	    fprintf(stderr,"PtlPut() failed\n");
	    exit(1);
	}

	if ( PtlEQWait( eq_handle, &event ) != PTL_OK ) {
	    fprintf(stderr,"%d:PtlEQWait() failed\n",rank);
	    exit(1);

	}

	if ( event.type != PTL_EVENT_SENT ) {
	    fprintf(stderr,"%d: expected %d event, got %d\n",rank,PTL_EVENT_SENT,event.type);
	} else {
	    fprintf(stderr,"%d: got PTL_EVENT_SENT\n",rank);
	}
	

	if ( (rc = PtlEQWait( eq_handle, &event )) != PTL_OK ) {
	    fprintf(stderr,"%d: PtlEQWait() failed with error code %d\n",rank,rc);
	    exit(1);
	}

	if ( event.type != PTL_EVENT_ACK ) {
	    fprintf(stderr,"%d: expected %d event, got %d\n",rank,PTL_EVENT_ACK,event.type);
	} else {
	    fprintf(stderr,"%d: got PTL_EVENT_ACK\n",rank);
	}

	if ( PtlMDUnlink( md_handle ) != PTL_OK ) {
	    fprintf(stderr,"PtlMDUnlink() failed\n");
	    exit(1);
	}

    }

    if ( rank == 1 ) {

	src_id.addr_kind = PTL_ADDR_GID;
	src_id.gid       = my_id.gid;
	src_id.rid       = 0;

	/* create a match entry */
	if ( PtlMEAttach( ni_handle,                 /* ni handle                  */
			  4,                         /* portal table index         */
			  src_id,                    /* source address             */
			  0,                         /* expected match bits        */
			  0,                         /* ignore bits to mask        */
			  PTL_UNLINK,                /* unlink when md is unlinked */
			  &me_handle ) != PTL_OK ) {
	    fprintf(stderr,"%d: PtlMEAttach() failed\n",rank);
	    exit(1);
	}

	/* create a memory descriptor */
	md.start      = &token;         /* start address                       */
	md.length     = sizeof(token);  /* length of buffer                    */
	md.threshold  = 1;              /* number of expected operations on md */
	md.max_offset = md.length;
	md.options    = PTL_MD_OP_PUT;  /* behavior of md                      */
	md.user_ptr   = NULL;           /* nothing to cache                    */
	md.eventq     = eq_handle;      /* event queue handle                  */
    
	/* attach the memory descriptor to the match entry */
	if ( PtlMDAttach( me_handle,                  /* me handle                  */
			  md,                         /* md to attach               */
			  PTL_UNLINK,                 /* unlink when threshold is 0 */
			  &md_handle ) != PTL_OK ) {
	    fprintf(stderr,"%d: PtlMDAttach() failed\n",rank);
	    exit(1);
	}

	fprintf(stderr,"%d: waiting for msg\n",rank);

	if ( PtlEQWait( eq_handle, &event ) != PTL_OK ) {
	    fprintf(stderr,"%d: PtlEQWait() failed\n",rank);
	    exit(1);
	}

	if ( event.type != PTL_EVENT_PUT ) {
	    fprintf(stderr,"%d: expected %d event, got %d\n",rank,PTL_EVENT_PUT,event.type);
	} else {
	    fprintf(stderr,"%d: got PTL_EVENT_PUT\n",rank);
	}

	printf("%d: event.hdr_data = %ld\n",rank,event.hdr_data);
	
    }

    printf("%d: All done\n",rank);

    /* close down the network interface */
     (void)PtlNIFini( ni_handle );

    /* finalize library */
    PtlFini(); 

    return 0;
}
