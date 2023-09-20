/*
 *  $Id: adi2req.c,v 1.1 2000/02/18 03:22:47 rbbrigh Exp $
 *
 *  (C) 1996 by Argonne National Laboratory and Mississipi State University.
 *      All rights reserved.  See COPYRIGHT in top-level directory.
 */

#include "mpiddev.h"

/***************************************************************************
 *
 * Multi-protocol, Multi-device support for 2nd generation ADI.
 * This file handles freed but not completed requests
 *
 ***************************************************************************/

void MPID_Request_free( request )
MPI_Request request;
{
	MPI_Request rq = request;
	int         mpi_errno = MPI_SUCCESS;

	switch (rq->handle_type) {
	case MPIR_SEND:
	    if (MPID_SendIcomplete( request, &mpi_errno )) {
	        MPIR_FORGET_SEND( &rq->shandle );
		MPID_SendFree( rq );
		rq = 0;
	    }
	    break;
	case MPIR_RECV:
	    if (MPID_RecvIcomplete( request, (MPI_Status *)0, &mpi_errno )) {
		MPID_RecvFree( rq );
		rq = 0;
	    }
	    break;
	case MPIR_PERSISTENT_SEND:
	    MPID_Abort( (MPI_Comm)0, 1, "MPI internal", 
			"Unimplemented operation - persistent send free" );
	    break;
	case MPIR_PERSISTENT_RECV:
	    MPID_Abort( (MPI_Comm)0, 1, "MPI internal", 
			"Unimplemented operation - persistent recv free" );
	    break;
	}

    if (rq) {
        rq->chandle.ref_count--;
/*      PRINTF( "Setting ref count to %d for %x\n",
                rq->chandle.ref_count, (long)rq ); */
        /* MPID_devset->req_pending = 0; */
    }
}
