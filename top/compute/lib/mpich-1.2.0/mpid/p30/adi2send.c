/*
 *  $Id: adi2send.c,v 1.1 2000/02/18 03:22:11 rbbrigh Exp $
 *
 *  (C) 1995 by Argonne National Laboratory and Mississipi State University.
 *      All rights reserved.  See COPYRIGHT in top-level directory.
 */

#include "mpiddev.h"

/***************************************************************************/
/*
 * Multi-protocol, Multi-device support for 2nd generation ADI.
 * We start with support for blocking, contiguous sends.
 * Note the 'msgrep' field; this gives a hook for heterogeneous systems
 * which can be ignored on homogeneous systems.
 */
/***************************************************************************/

void MPID_SendContig( comm_ptr, buf, len, src_lrank, tag, context_id, 
			      dest_grank, msgrep, error_code )
struct MPIR_COMMUNICATOR *comm_ptr;
void     *buf;
int      len, src_lrank, tag, context_id, dest_grank, *error_code;
MPID_Msgrep_t msgrep;
{
    MPID_Device *dev = MPID_devset->dev;
    int (*fcn) ANSI_ARGS(( void *, int, int, int, int, int, MPID_Msgrep_t,
			  struct MPIR_COMMUNICATOR * ));

     if (buf == 0 && len > 0) {
	*error_code = MPI_ERR_BUFFER;
	return;
    }
    /* Choose the function based on the message length in bytes */
    if (len < dev->long_len)
	fcn = dev->short_msg->send;
    else if (len < dev->vlong_len ) 
	fcn = dev->long_msg->send;
    else
	fcn = dev->vlong_msg->send;

    *error_code = (*(fcn))( buf, len, src_lrank, tag, context_id, dest_grank, 
			    msgrep, comm_ptr );
}

void MPID_IsendContig( comm_ptr, buf, len, src_lrank, tag, context_id, 
		       dest_grank, msgrep, request, error_code )
struct MPIR_COMMUNICATOR *   comm_ptr;
void        *buf;
int         len, src_lrank, tag, context_id, dest_grank, *error_code;
MPID_Msgrep_t msgrep;
MPI_Request request;
{
    MPID_Device *dev = MPID_devset->dev;
    int (*fcn) ANSI_ARGS(( void *, int, int, int, int, int, MPID_Msgrep_t, 
			   MPIR_SHANDLE *, struct MPIR_COMMUNICATOR * ));

    if (buf == 0 && len > 0) {
	*error_code = MPI_ERR_BUFFER;
	return;
    }

    /* Just in case; make sure that finish is 0 */
    request->shandle.finish = 0;

    /* Choose the function based on the message length in bytes */
    if (len < dev->long_len)
	fcn = dev->short_msg->isend;
    else if (len < dev->vlong_len ) 
	fcn = dev->long_msg->isend;
    else
	fcn = dev->vlong_msg->isend;

    *error_code = (*(fcn))( buf, len, src_lrank, tag, context_id, dest_grank, 
			    msgrep, (MPIR_SHANDLE *)request, comm_ptr );
}


/* Bsend is just a test for short send */
void MPID_BsendContig( comm_ptr, buf, len, src_lrank, tag, context_id, 
			 dest_grank, msgrep, error_code )
struct MPIR_COMMUNICATOR *comm_ptr;
void     *buf;
int      len, src_lrank, tag, context_id, dest_grank, *error_code;
MPID_Msgrep_t msgrep;
{
    MPID_Device *dev = MPID_devset->dev;
    int rc;

    if (len < dev->long_len) {

	rc = (*dev->short_msg->send)( buf, len, src_lrank, tag, context_id, 
				      dest_grank, msgrep, comm_ptr );
    }
    else
	rc = MPIR_ERR_MAY_BLOCK;
    *error_code = rc;
}

int MPID_SendIcomplete( request, error_code )
MPI_Request request;
int         *error_code;
{
    MPIR_SHANDLE *shandle = &request->shandle;
    int lerr;

    if (shandle->is_complete) {
	if (shandle->finish) 
	    (shandle->finish)( shandle );
	return 1;
    }

    if (shandle->test) 
	*error_code = 
	    (*shandle->test)( shandle );

    if (shandle->is_complete && shandle->finish) 
	(shandle->finish)( shandle );

    return shandle->is_complete;
}

void MPID_SendComplete( request, error_code )
MPI_Request request;
int         *error_code;
{
    MPIR_SHANDLE *shandle = &request->shandle;

    if (! shandle->is_complete) {
	*error_code = shandle->wait( shandle );
    }

    if (shandle->finish) 
	(shandle->finish)( shandle );

}

