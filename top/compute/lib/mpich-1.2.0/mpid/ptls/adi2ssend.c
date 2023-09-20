/*
 *  $Id: adi2ssend.c,v 1.1 2000/02/18 03:22:47 rbbrigh Exp $
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
 * 
 * For the synchronous send, we always use a Rendezvous send.
 */
/***************************************************************************/

void MPID_SsendContig( comm_ptr, buf, len, src_lrank, tag, context_id, 
			      dest_grank, msgrep, error_code )
struct MPIR_COMMUNICATOR *     comm_ptr;
void          *buf;
int           len, src_lrank, tag, context_id, dest_grank, *error_code;
MPID_Msgrep_t msgrep;
{
    int (*fcn) ANSI_ARGS(( void *, int, int, int, int, int, int, 
			   struct MPIR_COMMUNICATOR * ));

    if (buf == 0 && len > 0) {
	*error_code = MPI_ERR_BUFFER;
	return;
    }

    /* Choose the function based on the message length in bytes */
    fcn         = ( len < MPID_devset->dev->long_len ) ?
                  MPID_devset->dev->short_msg->ssend   :
                  MPID_devset->dev->long_msg->ssend;

    *error_code = fcn( buf, len, src_lrank, tag, context_id, dest_grank, 
		      msgrep, comm_ptr );

}
void MPID_IssendContig( comm_ptr, buf, len, src_lrank, tag, context_id, 
			      dest_grank, msgrep, request, error_code )
struct MPIR_COMMUNICATOR *     comm_ptr;
void          *buf;
int           len, src_lrank, tag, context_id, dest_grank, *error_code;
MPID_Msgrep_t msgrep;
MPI_Request   request;
{
    int (*fcn) ANSI_ARGS(( void *, int, int, int, int, int, int, 
			   MPIR_SHANDLE *, struct MPIR_COMMUNICATOR * ));

    if (buf == 0 && len > 0) {
	*error_code = MPI_ERR_BUFFER;
	return;
    }

    /* Choose the function based on the message length in bytes */
    fcn         = ( len < MPID_devset->dev->long_len ) ?
                  MPID_devset->dev->short_msg->issend  :
                  MPID_devset->dev->long_msg->issend;

    *error_code = fcn( buf, len, src_lrank, tag, context_id, dest_grank, 
		       msgrep, (MPIR_SHANDLE *)request, comm_ptr );

}
