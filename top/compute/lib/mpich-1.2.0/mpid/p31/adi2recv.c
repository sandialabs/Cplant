/*
 *  $Id: adi2recv.c,v 1.1 2001/12/18 00:34:41 rbbrigh Exp $
 *
 *  (C) 1995 by Argonne National Laboratory and Mississipi State University.
 *      All rights reserved.  See COPYRIGHT in top-level directory.
 */

#include "mpiddev.h"

/* 
  This file contains the routines to handle receiving a message
 */

/***************************************************************************/
/*
 * Despite the apparent symmetry, receives are fundamentally different from
 * sends.  All receives happen by processing an incoming item of information
 * and checking it against known receives.
 *
 * Eventually, we may want to make RecvContig a special case (as in the
 * first generation ADI) to avoid the routine calls.
 */
/***************************************************************************/
/* Does this need to return msgrep if heterogeneous? */

void MPID_RecvContig( comm_ptr, buf, maxlen, src_lrank, tag, context_id, 
		      status, error_code )
struct MPIR_COMMUNICATOR *comm_ptr;
void       *buf;
int        maxlen, src_lrank, tag, context_id, *error_code;
MPI_Status *status;
{
    MPIR_RHANDLE rhandle;
    MPI_Request  request = (MPI_Request)&rhandle;

    /* Just in case; make sure that finish is 0 */
    rhandle.finish = 0;

    *error_code = MPI_SUCCESS;

    MPID_IrecvContig( comm_ptr, buf, maxlen, src_lrank, tag, context_id, 
		      request, error_code );
    if (!*error_code) {
	MPID_RecvComplete( request, status, error_code );
    }
}

void MPID_IrecvContig( comm_ptr, buf, maxlen, src_lrank, tag, context_id, 
		       request, error_code )
struct MPIR_COMMUNICATOR *comm_ptr;
void        *buf;
int         maxlen, src_lrank, tag, context_id, *error_code;
MPI_Request request;
{
    MPIR_RHANDLE *dmpi_unexpected, *rhandle = &request->rhandle;

    *error_code = MPI_SUCCESS;

    if (buf == 0 && maxlen > 0) {
	*error_code = MPI_ERR_BUFFER;
	return;
    }

    rhandle->len	 = maxlen;
    rhandle->buf	 = buf;
    rhandle->is_complete = 0;
    rhandle->wait        = 0;
    rhandle->test        = 0;
    rhandle->finish      = 0;
    rhandle->comm        = comm_ptr;

    MPID_Search_unexpected_queue_and_post( src_lrank, tag, context_id, rhandle );
    *error_code = rhandle->s.MPI_ERROR;
}

int MPID_RecvIcomplete( request, status, error_code )
MPI_Request request;
MPI_Status  *status;
int         *error_code;
{
    MPIR_RHANDLE *rhandle = &request->rhandle;
    MPID_Device *dev;
    int         lerr;

    *error_code = MPI_SUCCESS;
	    
    if (rhandle->is_complete) {
	if (rhandle->finish) 
	    (rhandle->finish)( rhandle );
	if (status) 
	    *status = rhandle->s;
	*error_code = rhandle->s.MPI_ERROR;
	return 1;
    }

    if (rhandle->test) {
	*error_code = 
	    (*rhandle->test)( rhandle );
    }

    if (rhandle->is_complete) {
	if (rhandle->finish) 
	    (rhandle->finish)( rhandle );
	if (status) 
	    *status = rhandle->s;
	*error_code = rhandle->s.MPI_ERROR;
	return 1;
    }
    return 0;
}

void MPID_RecvComplete( request, status, error_code )
MPI_Request request;
MPI_Status  *status;
int         *error_code;
{
    MPIR_RHANDLE *rhandle = &request->rhandle;

    if ( (! rhandle->is_complete) && (rhandle->wait) ) {
	*error_code = rhandle->wait( rhandle );
    }

    if ( rhandle->finish ) {
	rhandle->finish( rhandle );
    }

    if ( status ) {
	*status = rhandle->s;
    }

    *error_code = rhandle->s.MPI_ERROR;

}


