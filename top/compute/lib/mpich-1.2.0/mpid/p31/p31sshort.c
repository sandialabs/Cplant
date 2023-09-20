#include "mpiddev.h"


/***************************************************************************

  Synchronous short protocol send  

  ***************************************************************************/

/***************************************************************************

  MPID_P31_Sshort_send

  short protocol blocking synchronous send

  ***************************************************************************/
int MPID_P31_Sshort_send( buf, len, src_lrank, tag, context_id, dest,
				    msgrep, comm )
void *buf;
int  len, tag, context_id, src_lrank, dest, msgrep;
struct MPIR_COMMUNICATOR *comm;
{
    MPIR_SHANDLE shandle;
    int          mpi_errno = MPI_SUCCESS;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P31_Printf("entering MPID_P31_Sshort_send()\n");
    }
#endif

    if ( (mpi_errno = MPID_P31_Sshort_isend( buf, len, src_lrank, tag, 
					      context_id, dest, msgrep,
					      &shandle, comm ) ) != MPI_SUCCESS ) {
	return mpi_errno;
    }

    mpi_errno = shandle.wait( &shandle );

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P31_Printf("leaving MPID_P31_Sshort_send()\n");
    }
#endif

    return mpi_errno;
}


/***************************************************************************
  
  MPID_P31_Sshort_isend

  short protocol nonblocking synchronous send

  ***************************************************************************/
int MPID_P31_Sshort_isend( buf, len, src_lrank, tag, context_id, dest,
			   msgrep, shandle, comm )
void *buf;
int  len, tag, context_id, src_lrank, dest, msgrep;
MPIR_SHANDLE *shandle;
struct MPIR_COMMUNICATOR *comm;
{
    ptl_md_t         md;
    ptl_process_id_t dest_id;
    ptl_match_bits_t match_bits;

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_SEND)
    if (MPID_DebugFlag) {
        P31_Printf("MPID_P31_Sshort_isend\n");
    }
#   endif

    shandle->got_sent = shandle->got_ack = 0;

    if ( (shandle->eq_handle_list_ptr = MPID_P31_Get_eq_handle()) == NULL ) {
	return MPI_ERR_INTERN;
    }

    /* set up an ack me for an unexpected synchronous ack */
    dest_id.addr_kind = PTL_ADDR_GID;
    dest_id.gid       = _mpi_p31_my_id.gid;
    dest_id.rid       = dest;

    /* ack match bits are the send handle */
    match_bits        = (ptl_match_bits_t)shandle;

    /* get an me on the ack portal */
    P31_CALL( PtlMEInsert( _mpi_p31_ack_me_handle,
			   dest_id,
			   match_bits,
			   0,
			   PTL_UNLINK,
			   PTL_INS_AFTER,
			   &shandle->me_handle ), PtlMEInsert );

    md.start      = buf;
    md.length     = len;
    md.threshold  = 2;
    md.max_offset = md.length;
    md.options    = PTL_MD_OP_PUT;
    md.user_ptr   = shandle;
    md.eventq     = shandle->eq_handle_list_ptr->eq_handle;

    /* attach the md to the me */
    P31_CALL( PtlMDAttach( shandle->me_handle,
			   md,
			   PTL_UNLINK,
			   &shandle->md_handle ), PtlMDAttach );
    
    /* destination match bits */
    P31_SET_SHORT_SEND_BITS( match_bits, context_id, src_lrank, tag );

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P31_Printf("  calling PtlPut(\n");
	P31_Printf("    shandle->md_handle   = %d\n",shandle->md_handle);
	P31_Printf("    ack                  = PTL_ACK_REQ\n");
	P31_Printf("    dest_id              =>\n");
	P31_Printf("      addr_kind          = %d\n",dest_id.addr_kind);
	P31_Printf("      gid                = %d\n",dest_id.gid);
	P31_Printf("      rid                = %d\n",dest_id.rid);
	P31_Printf("    _mpi_p31_recv_portal = %d\n",_mpi_p31_recv_portal);
	P31_Printf("    acl                  = 0\n");
	P31_Printf("    match_bits           = 0x%lx\n",match_bits );
	P31_Printf("    offset               = 0\n");
	P31_Printf("    hdr_data             = %ld (0x%lx))\n",(unsigned long)shandle,(unsigned long)shandle);
    }
#   endif

    P31_CALL( PtlPut( shandle->md_handle,
		      PTL_ACK_REQ,
		      dest_id,
		      _mpi_p31_recv_portal,
		      0,
		      match_bits,
		      0,
		      (ptl_hdr_data_t)shandle ), PtlPut );

    shandle->is_complete     = 0;
    shandle->errval          = MPI_SUCCESS;
    shandle->comm            = comm;
    shandle->start           = buf;
    shandle->bytes_as_contig = len;
    shandle->wait	     = MPID_P31_Sshort_wait;
    shandle->test	     = MPID_P31_Sshort_test;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P31_Printf("leaving MPID_P31_Sshort_isend()\n");
    }
#endif

    return MPI_SUCCESS;
}


/***************************************************************************

  MPID_P31_Sshort_wait

  wait for short protocol synchronous send request to complete

  ***************************************************************************/
int MPID_P31_Sshort_wait( shandle )
MPIR_SHANDLE *shandle;
{
    int         mpi_errno = MPI_SUCCESS;
    ptl_event_t event1,event2;

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
        P31_Printf("entering MPID_P31_Sshort_wait_send()\n");
    }
#   endif

    if ( shandle->got_sent == 0 ) {

	if ( (_mpi_p31_errno = PtlEQWait( shandle->eq_handle_list_ptr->eq_handle, &event1 )) != PTL_OK ) {
	    fprintf(stderr,"%d: (%s:%d) PtlEQWait() failed : %s\n",
		    MPID_MyWorldRank, __FILE__, __LINE__, ptl_err_str[_mpi_p31_errno]);
	}

#       if defined(MPID_DEBUG_ALL)
	if ( MPID_DebugFlag ) {
	    P31_Printf("  got event1\n");
	    P31_Dump_event( &event1 );
	}
#       endif

	shandle->got_sent = 1;

    }

    if ( shandle->got_ack == 0 ) {

	if ( (_mpi_p31_errno = PtlEQWait( shandle->eq_handle_list_ptr->eq_handle, &event2 )) != PTL_OK ) {
	    fprintf(stderr,"%d: (%s:%d) PtlEQWait() failed : %s\n",
		    MPID_MyWorldRank, __FILE__, __LINE__, ptl_err_str[_mpi_p31_errno]);
	}

#       if defined(MPID_DEBUG_ALL)
	if ( MPID_DebugFlag ) {
	    P31_Printf("  got event2\n");
	    P31_Dump_event( &event2 );
	}
#       endif

	shandle->got_ack = 1;
    }

    MPID_P31_Free_eq_handle( shandle->eq_handle_list_ptr );

    shandle->is_complete = 1;
    
#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P31_Printf("leaving MPID_P31_Sshort_wait_send()\n");
    }
#   endif

    return mpi_errno;
}


/***************************************************************************
  
  MPID_P31_Sshort_test

  test for the completion of a short protocol synchronous send request

  ***************************************************************************/
int MPID_P31_Sshort_test( shandle )
MPIR_SHANDLE *shandle;
{
    int         mpi_errno = MPI_SUCCESS;
    ptl_event_t event;
 
#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P31_Printf("entering MPID_P31_Sshort_test_send()\n");
    }
#endif

    if ( (_mpi_p31_errno = PtlEQGet( shandle->eq_handle_list_ptr->eq_handle, &event )) == PTL_OK ) {

#       if defined(MPID_DEBUG_ALL)
	if ( MPID_DebugFlag ) {
	    P31_Printf("  got event\n");
	    P31_Dump_event( &event );
	}
#       endif

	/* it's possible that the PTL_EVENT_ACK can come in before the PTL_EVENT_SENT */
	if ( event.type == PTL_EVENT_SENT ) {

	    shandle->got_sent = 1;

	} else if ( (event.type == PTL_EVENT_ACK) || (event.type == PTL_EVENT_PUT) ) {

	    shandle->got_ack     = 1;

	}

	if ( shandle->got_sent && shandle->got_ack ) {

	    /* add the eq handle back into the list */
	    MPID_P31_Free_eq_handle( shandle->eq_handle_list_ptr );

	    shandle->is_complete = 1;
	}

    } else {
	if ( _mpi_p31_errno != PTL_EQ_EMPTY ) {
	    fprintf(stderr,"%d: (%s:%d) PtlEQGet() failed : %s\n",
		    MPID_MyWorldRank, __FILE__, __LINE__, ptl_err_str[_mpi_p31_errno]);
	}
    }

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P31_Printf("leaving MPID_P31_Sshort_test_send()\n");
    }
#endif

    return MPI_SUCCESS;
}
