/*
 *  $Id: adi2hssend.c,v 1.1 2000/02/18 03:22:46 rbbrigh Exp $
 *
 *  (C) 1996 by Argonne National Laboratory and Mississipi State University.
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

void MPID_SsendDatatype( comm_ptr, buf, count, dtype_ptr, src_lrank, tag, 
			context_id, dest_grank, error_code )
struct MPIR_COMMUNICATOR *    comm_ptr;
struct MPIR_DATATYPE *dtype_ptr;
void         *buf;
int          count, src_lrank, tag, context_id, dest_grank, *error_code;
{
    int             len, contig_size;
    void            *mybuf;

    contig_size = MPIR_GET_DTYPE_SIZE(datatype,dtype_ptr);

    if (contig_size > 0 ) {
	len = contig_size * count;
	MPID_SsendContig( comm_ptr, buf, len, src_lrank, tag, context_id, 
		 dest_grank, MPID_MSGREP_RECEIVER, error_code );
	return;
    }

    mybuf = 0;
    MPID_PackMessage( buf, count, dtype_ptr, comm_ptr, dest_grank, 
		      MPID_MSGREP_RECEIVER, MPID_MSG_OK,
		      (void **)&mybuf, &len, error_code );

    if (*error_code) return;

    MPID_SsendContig( comm_ptr, mybuf, len, src_lrank, tag, context_id, 
		     dest_grank, MPID_MSGREP_RECEIVER, error_code );
    if (mybuf) {
	FREE( mybuf );
    }
}

void MPID_IssendDatatype( comm_ptr, buf, count, dtype_ptr, src_lrank, tag, 
			  context_id, dest_grank, request, error_code )
struct MPIR_COMMUNICATOR *    comm_ptr;
struct MPIR_DATATYPE *dtype_ptr;
void         *buf;
int          count, src_lrank, tag, context_id, dest_grank, *error_code;
MPI_Request  request;
{
    int             len, contig_size;
    char            *mybuf;
    MPID_Msgrep_t   msgrep = MPID_MSGREP_RECEIVER;
    MPID_Msg_pack_t msgact = MPID_MSG_OK;

    contig_size = MPIR_GET_DTYPE_SIZE(datatype,dtype_ptr);

    if (contig_size > 0 ) {
	len = contig_size * count;
	MPID_IssendContig( comm_ptr, buf, len, src_lrank, tag, context_id, 
			  dest_grank, MPID_MSGREP_RECEIVER, request, error_code );
	return;
    }

    mybuf = 0;
    MPID_PackMessage( buf, count, dtype_ptr, comm_ptr, dest_grank, 
		      MPID_MSGREP_RECEIVER, MPID_MSG_OK,
		      (void **)&mybuf, &len, error_code );

    if (*error_code) return;

    MPID_IssendContig( comm_ptr, mybuf, len, src_lrank, tag, context_id, 
		      dest_grank, MPID_MSGREP_RECEIVER, request, error_code );

    if (request->shandle.is_complete) {
	if (mybuf) { 
	    FREE( mybuf ); 
	}
    }
    else {
	request->shandle.start  = mybuf;
	request->shandle.finish = MPID_PackMessageFree;
    }

}
