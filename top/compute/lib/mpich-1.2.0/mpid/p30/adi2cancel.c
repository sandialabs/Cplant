/*
 *  $Id: adi2cancel.c,v 1.1 2000/02/18 03:22:10 rbbrigh Exp $
 *
 *  (C) 1996 by Argonne National Laboratory and Mississipi State University.
 *      All rights reserved.  See COPYRIGHT in top-level directory.
 */

#include "mpiddev.h"

/*
 * This file contains the routines to handle canceling a message
 *
 * Note: for now, cancel will probably only work on unmatched receives.
 * However, this code provides the hooks for supporting more complete
 * cancel implementations.
 */

void MPID_SendCancel( request, error_code )
MPI_Request request;
int         *error_code;
{
    *error_code = MPI_SUCCESS;
    /* This isn't really correct */
}

void MPID_RecvCancel( request, error_code )
MPI_Request request;
int         *error_code;
{
    MPIR_RHANDLE *rhandle = (MPIR_RHANDLE *)request;

    /* First, try to find in pending receives */
    /* Mark the request as cancelled */
    rhandle->s.MPI_TAG = MPIR_MSG_CANCELLED;
    /* Mark it as complete */
    rhandle->is_complete = 1;
    /* Should we call finish to free any space?  cancel? */

    /* If request is a persistent one, we need to mark it as inactive
       as well */
    if (rhandle->handle_type == MPIR_PERSISTENT_RECV) {
	MPIR_PRHANDLE *prhandle = (MPIR_PRHANDLE *)request;
	prhandle->active = 0;
    }

    *error_code = MPI_SUCCESS;
}


