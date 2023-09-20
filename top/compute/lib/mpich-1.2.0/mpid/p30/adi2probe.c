/*
 *  $Id: adi2probe.c,v 1.1 2000/02/18 03:22:11 rbbrigh Exp $
 *
 *  (C) 1995 by Argonne National Laboratory and Mississipi State University.
 *      All rights reserved.  See COPYRIGHT in top-level directory.
 */

#include "mpiddev.h"

void MPID_Iprobe( comm_ptr, tag, context_id, src_lrank, found, error_code, 
		  status )
struct MPIR_COMMUNICATOR *comm_ptr;
int        tag, context_id, src_lrank, *found, *error_code;
MPI_Status *status;
{
    /* At this time, we check to see if the message has already been received */
    MPID_Search_unexpected_queue( comm_ptr, src_lrank, tag, context_id,
				  found, status );
}

void MPID_Probe( comm_ptr, tag, context_id, src_lrank, error_code, status )
struct MPIR_COMMUNICATOR *comm_ptr;
int        tag, src_lrank, context_id, *error_code;
MPI_Status *status;
{
    int found;

    *error_code = 0;

    while (1) {
	MPID_Iprobe( comm_ptr, tag, context_id, src_lrank, &found, error_code, 
		     status );
	if (found || *error_code) break;
	/* Wait for a message */
	MPID_DeviceCheck( MPID_BLOCKING );
    }

}
