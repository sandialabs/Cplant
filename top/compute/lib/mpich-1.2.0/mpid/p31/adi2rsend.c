
#include "mpiddev.h"

/***************************************************************************/
/*
 * Multi-protocol, Multi-device support for 2nd generation ADI.
 * We start with support for blocking, contiguous sends.
 * Note the 'msgrep' field; this gives a hook for heterogeneous systems
 * which can be ignored on homogeneous systems.
 */
/***************************************************************************/

void MPID_RsendContig( comm, buf, len, src_lrank, tag, context_id, 
			      dest_grank, msgrep, error_code )
struct MPIR_COMMUNICATOR *comm;
void     *buf;
int      len, src_lrank, tag, context_id, dest_grank, msgrep, *error_code;
{
    if (buf == 0 && len > 0) {
	*error_code = MPI_ERR_BUFFER;
	return;
    }

    *error_code = 
      MPID_devset->dev->ready->send( buf, len, src_lrank, tag, context_id, 
				     dest_grank, msgrep, comm );
}

void MPID_IrsendContig( comm, buf, len, src_lrank, tag, context_id, 
		       dest_grank, msgrep, request, error_code )
struct MPIR_COMMUNICATOR *comm;
void        *buf;
int         len, src_lrank, tag, context_id, dest_grank, msgrep, *error_code;
MPI_Request request;
{

    if (buf == 0 && len > 0) {
	*error_code = MPI_ERR_BUFFER;
	return;
    }

    *error_code = 
      MPID_devset->dev->ready->isend( buf, len, src_lrank, tag, context_id, 
				      dest_grank, msgrep, (MPIR_SHANDLE *)request, 
				      comm );

}



