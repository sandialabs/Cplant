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
**  Test the semantics of the md max_offset field. 
*/

#include<stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include"p30.h"

#define SIZE 16

#define PORTAL 4

int data1[SIZE];
int data2[SIZE];

int main( int argc, char *argv[] )
{

    ptl_process_id_t my_id, src_id, dest_id;
    ptl_id_t         rank;
    ptl_id_t         size;
    ptl_handle_ni_t  ni_handle;
    ptl_handle_me_t  me_handle0,me_handle1;
    ptl_md_t         md;
    ptl_handle_md_t  md_handle0,md_handle1;
    ptl_handle_eq_t  eq_handle;
    ptl_event_t      event;
    int              i;
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
		     32,
		     &eq_handle ) != PTL_OK ) {
	fprintf(stderr,"%d: PtlEQAlloc() failed\n",rank);
	exit(1);
    }

    if ( rank == 0 ) {

	data1[0] = -1;

	/* create a memory descriptor */
	md.start      = data1;
	md.length     = sizeof(int);
	md.threshold  = PTL_MD_THRESH_INF;
	md.max_offset = 0;
	md.options    = 0;
	md.user_ptr   = NULL;
	md.eventq     = eq_handle;

	/* build dest portal address */
	dest_id.addr_kind = PTL_ADDR_GID;
	dest_id.rid       = 1;
	dest_id.gid       = my_id.gid;

	if ( PtlMDBind( ni_handle,
			md,
			&md_handle0 ) != PTL_OK ) {
	    fprintf(stderr,"PtlMDBind() failed\n");
	    exit(1);
	}

	/* wait for 1 to get set up */
	if ( PtlNIBarrier( ni_handle ) != PTL_OK ) {
	    fprintf(stderr,"PtlNIBarrier() failed\n");
	    exit(1);
	}

	for ( i=0; i<SIZE; i++ ) {

	    if ( PtlPut( md_handle0,
			 PTL_NOACK_REQ,
			 dest_id,
			 PORTAL,
			 0,
			 0,
			 0,
			 0 ) != PTL_OK ) {
		fprintf(stderr,"PtlPut() failed\n");
		exit(1);
	    }

	    if ( PtlEQWait( eq_handle, &event ) != PTL_OK ) {
		fprintf(stderr,"%d:PtlEQGet() failed\n",rank);
		exit(1);

	    }

	    if ( event.type != PTL_EVENT_SENT ) {
		fprintf(stderr,"%d: expected %d event, got %d\n",rank,PTL_EVENT_SENT,event.type);
	    };

	}

	
	if ( PtlMDUnlink( md_handle0 ) != PTL_OK ) {
	    fprintf(stderr,"PtlMDUnlink() failed\n");
	    exit(1);
	}

    }

    if ( rank == 1 ) {

	for ( i=0; i<SIZE; i++ ) {
	    data1[i] = i;
	    data2[i] = i;
	}

	src_id.addr_kind = PTL_ADDR_GID;
	src_id.gid       = my_id.gid;
	src_id.rid       = 0;

	/* create a match entry */
	if ( PtlMEAttach( ni_handle,                 /* ni handle                  */
			  PORTAL,                         /* portal table index         */
			  src_id,                    /* source address             */
			  0,                /* expected match bits        */
			  0,                     /* ignore bits to mask        */
			  PTL_UNLINK,                /* unlink when md is unlinked */
			  &me_handle0 ) != PTL_OK ) {
	    fprintf(stderr,"%d: PtlMEAttach() failed\n",rank);
	    exit(1);
	}

	if ( PtlMEInsert( me_handle0,
			  src_id,
			  0,
			  0,
			  PTL_UNLINK,
			  PTL_INS_AFTER,
			  &me_handle1 ) != PTL_OK ) {
	    fprintf(stderr,"%d: PtlMEInsert() failed\n",rank);
	    exit(1);
	}

	/* create a memory descriptor */
	md.start      = data1;
	md.length     = sizeof(int) * SIZE;
	md.threshold  = PTL_MD_THRESH_INF;
	md.max_offset = sizeof(int) * (SIZE - 2);
	md.options    = PTL_MD_OP_PUT;
	md.user_ptr   = NULL;
	md.eventq     = eq_handle;
    
	/* attach the memory descriptor to the match entry */
	if ( PtlMDAttach( me_handle0,                  /* me handle                  */
			  md,                         /* md to attach               */
			  PTL_UNLINK,                 /* unlink when threshold is 0 */
			  &md_handle0 ) != PTL_OK ) {
	    fprintf(stderr,"%d: PtlMDAttach() failed\n",rank);
	    exit(1);
	}

	/* create a memory descriptor */
	md.start      = data2;
	md.length     = sizeof(int) * SIZE;
	md.threshold  = PTL_MD_THRESH_INF;
	md.max_offset = sizeof(int) * SIZE;
	md.options    = PTL_MD_OP_PUT;
	md.user_ptr   = NULL;
	md.eventq     = eq_handle;
    
	/* attach the memory descriptor to the match entry */
	if ( PtlMDAttach( me_handle1,
			  md,
			  PTL_UNLINK,
			  &md_handle1 ) != PTL_OK ) {
	    fprintf(stderr,"%d: PtlMDAttach() failed\n",rank);
	    exit(1);
	}

	if ( PtlNIBarrier( ni_handle ) != PTL_OK ) {
	    fprintf(stderr,"PtlNIBarrier() failed\n");
	    exit(1);
	}

	for ( i=0; i<SIZE; i++ ) {

	    if ( PtlEQWait( eq_handle, &event ) != PTL_OK ) {
		fprintf(stderr,"%d: PtlEQWait() failed\n",rank);
		exit(1);
	    }

	    if ( event.type != PTL_EVENT_PUT ) {
		fprintf(stderr,"%d: expected %d event, got %d\n",rank,PTL_EVENT_GET,event.type);
	    }

	}

	if ( (data1[SIZE-1] == SIZE-1) && (data2[0] == -1) ) {
	    printf("Passed all tests\n");
	} else {
	    printf("data1[%d] = %d  data2[0] = %d\n",SIZE,data1[SIZE],data2[0]);
	}
	    
    }

    fprintf(stderr,"%d: All done\n",rank);

    /* close down the network interface */
     (void)PtlNIFini( ni_handle );

    /* finalize library */
    PtlFini(); 

    return 0;
}
