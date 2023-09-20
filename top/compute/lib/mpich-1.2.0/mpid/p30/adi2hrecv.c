/*
 *  $Id: adi2hrecv.c,v 1.1 2000/02/18 03:22:10 rbbrigh Exp $
 *
 *  (C) 1995 by Argonne National Laboratory and Mississipi State University.
 *      All rights reserved.  See COPYRIGHT in top-level directory.
 */

#include "mpiddev.h"

/***************************************************************************/
/*
 * Multi-protocol, Multi-device support for 2nd generation ADI.
 * This file has support for noncontiguous sends for systems that do not 
 * have native support for complex datatypes.
 */
/***************************************************************************/

void MPID_RecvDatatype( comm_ptr, buf, count, dtype_ptr, src_lrank, tag, 
			context_id, status, error_code )
struct MPIR_COMMUNICATOR *    comm_ptr;
void         *buf;
int          count, src_lrank, tag, context_id, *error_code;
struct MPIR_DATATYPE * dtype_ptr;
MPI_Status   *status;
{
    MPIR_RHANDLE rhandle;
    MPI_Request  request = (MPI_Request)&rhandle;

    *error_code = MPI_SUCCESS;
    MPID_IrecvDatatype( comm_ptr, buf, count, dtype_ptr, src_lrank, tag, 
			context_id, request, error_code );
    if (!*error_code) {
	MPID_RecvComplete( request, status, error_code );
    }
}

void MPID_IrecvDatatype( comm_ptr, buf, count, dtype_ptr, src_lrank, tag, 
			 context_id, request, error_code )
struct MPIR_COMMUNICATOR *    comm_ptr;
void         *buf;
int          count, src_lrank, tag, context_id, *error_code;
struct MPIR_DATATYPE * dtype_ptr;
MPI_Request  request;
{
    MPIR_RHANDLE *rhandle = &request->rhandle;
    int           len;
    void         *mybuf;
    int           contig_size;

    /* Just in case; make sure that finish is 0 */
    rhandle->finish = 0;

    /* See if this is really contiguous */
    contig_size = MPIR_GET_DTYPE_SIZE(datatype,dtype_ptr);

    if (contig_size > 0 ) {
	/* Just drop through into the contiguous send routine 
	   For packed data, the representation format is that in the
	   communicator.
	 */
	len = contig_size * count;
	MPID_IrecvContig( comm_ptr, buf, len, src_lrank, tag, context_id, 
			  request, error_code );
	return;
    }

    /* 
       Follow the same steps as IrecvContig, buf after creating a 
       temporary buffer to hold the incoming data in.
       */
    
    MPID_UnpackMessageSetup( count, dtype_ptr, comm_ptr, src_lrank, 0,
			     (void **)&mybuf, &len, error_code );
    if (*error_code) return;

    rhandle->len	 = len;
    rhandle->buf	 = mybuf;
    rhandle->start       = buf;
    rhandle->count       = count;
    rhandle->datatype    = dtype_ptr;
    MPIR_REF_INCR(dtype_ptr);
    rhandle->is_complete = 0;
    rhandle->wait        = 0;
    rhandle->test        = 0;
    rhandle->finish      = MPID_UnpackMessageComplete;
    rhandle->comm        = comm_ptr;

    MPID_Search_unexpected_queue_and_post( src_lrank, tag, context_id, rhandle );
    *error_code = rhandle->s.MPI_ERROR;
    
    /* if the message was ready and is complete, then it might need unpacking */
    if ( (!*error_code) && (rhandle->is_complete) && (rhandle->finish ) ) {
	*error_code = (*rhandle->finish)( rhandle );
	rhandle->finish = 0;
    }

}
