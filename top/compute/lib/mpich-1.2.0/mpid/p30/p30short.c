#include "mpiddev.h"


/***************************************************************************

  Short protocol send
  

  ***************************************************************************/

/***************************************************************************

  MPID_P30_Short_send

  blocking short protocol send

  ***************************************************************************/
int MPID_P30_Short_send( buf, len, src_lrank, tag, context_id, dest,
				    msgrep, comm )
void *buf;
int  len, tag, context_id, src_lrank, dest, msgrep;
struct MPIR_COMMUNICATOR *comm;
{
    MPIR_SHANDLE shandle;
    int          mpi_errno = MPI_SUCCESS;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P30_Printf("entering MPID_P30_Short_send()\n");
    }
#endif

    if ( (mpi_errno = MPID_P30_Short_isend( buf, len, src_lrank, tag, 
					     context_id, dest, msgrep, 
					     &shandle, comm ) ) != MPI_SUCCESS ) {
	return mpi_errno;
    }

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P30_Printf("waiting for send to complete\n");
    }
#endif

    mpi_errno = shandle.wait( &shandle );

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P30_Printf("leaving MPID_P30_Short_send()\n");
    }
#endif

    return mpi_errno;
}


/***************************************************************************

  MPID_P30_Short_isend
  
  nonblocking short protocol send

  ***************************************************************************/
int MPID_P30_Short_isend( buf, len, src_lrank, tag, context_id, dest,
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
        P30_Printf("P30_Post_send_short\n");
	P30_Printf("  buf        = %p\n",buf);
	P30_Printf("  len        = %d\n",len);
	P30_Printf("  src_lrank  = %d\n",src_lrank);
	P30_Printf("  tag        = %d\n",tag);
	P30_Printf("  context_id = %d\n",context_id);
	P30_Printf("  dest       = %d\n",dest);
    }
#   endif

    if ( (shandle->eq_handle_list_ptr = MPID_P30_Get_eq_handle() ) == NULL ) {
	return MPI_ERR_INTERN;
    }

    md.start      = buf;
    md.length     = len;
    md.threshold  = 1;
    md.max_offset = md.length;
    md.options    = 0;
    md.user_ptr   = shandle;
    md.eventq     = shandle->eq_handle_list_ptr->eq_handle;

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P30_Printf("  calling PtlMDBind(\n");
	P30_Printf("    _mpi_p30_ni_handle = %d\n",_mpi_p30_ni_handle);
	P30_Printf("    md =>\n");
	P30_Printf("      start             = %p\n",md.start);
	P30_Printf("      length            = %d\n",md.length);
	P30_Printf("      threshold         = %d\n",md.threshold);
	P30_Printf("      max_offset        = %d\n",md.max_offset);
	P30_Printf("      options           = %d\n",md.options);
	P30_Printf("      user_ptr          = %p\n",md.user_ptr);
	P30_Printf("      eventq            = %d\n",md.eventq);
	P30_Printf("    &shandle->md_handle = %p)\n",&shandle->md_handle);
    }
#   endif

    /* bind the memory descriptor to the interface */
    P30_CALL( PtlMDBind( _mpi_p30_ni_handle,
			 md,
			 &shandle->md_handle ), PtlMDBind );

    dest_id.addr_kind = PTL_ADDR_GID;
    dest_id.gid       = _mpi_p30_my_id.gid;
    dest_id.rid       = dest;
    
    /* fill these in */
    P30_SET_SHORT_SEND_BITS( match_bits, context_id, src_lrank, tag );

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P30_Printf("  calling PtlPut(\n");
	P30_Printf("    shandle->md_handle   = %d\n",shandle->md_handle);
	P30_Printf("    ack                  = PTL_NOACK_REQ\n");
	P30_Printf("    dest_id              =>\n");
	P30_Printf("      addr_kind          = %d\n",dest_id.addr_kind);
	P30_Printf("      gid                = %d\n",dest_id.gid);
	P30_Printf("      rid                = %d\n",dest_id.rid);
	P30_Printf("    _mpi_p30_recv_portal = %d\n",_mpi_p30_recv_portal);
	P30_Printf("    acl                  = 0\n");
	P30_Printf("    match_bits           = 0x%lx\n",match_bits );
	P30_Printf("    offset               = 0\n");
	P30_Printf("    hdr_data             = 0 )\n");
    }
#   endif

    P30_CALL( PtlPut( shandle->md_handle,
		      PTL_NOACK_REQ,
		      dest_id,
		      _mpi_p30_recv_portal,
		      0,
		      match_bits,
		      0,
		      0 ), PtlPut );

    shandle->is_complete     = 0;
    shandle->errval          = MPI_SUCCESS;
    shandle->comm            = comm;
    shandle->start           = buf;
    shandle->bytes_as_contig = len;
    shandle->test	     = MPID_P30_Short_test_send;
    shandle->wait	     = MPID_P30_Short_wait_send;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P30_Printf("leaving MPID_P30_Short_isend()\n");
    }
#endif

    return MPI_SUCCESS;


}

/***************************************************************************

  MPID_P30_Short_bsend
  
  nonblocking short protocol buffered send

  ***************************************************************************/
int MPID_P30_Short_bsend( buf, len, src_lrank, tag, context_id, dest,
			   shandle, comm )
void *buf;
int  len, tag, context_id, src_lrank, dest;
MPIR_SHANDLE *shandle;
struct MPIR_COMMUNICATOR *comm;
{
    
    MPID_P30_Short_isend( buf, len, src_lrank, tag, context_id, dest,
			 0, shandle, comm );

    return MPID_P30_Short_wait_send( shandle );
    
}


/***************************************************************************

  MPID_P30_Short_test_send

  test for the completion of short protocol send request

  ***************************************************************************/
int MPID_P30_Short_test_send( shandle )
MPIR_SHANDLE *shandle;
{
    ptl_event_t event;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P30_Printf("entering MPID_P30_Short_test_send()\n");
	P30_Printf("  calling PtlEQGet(\n");
	P30_Printf("    eq_handle = %d\n",shandle->eq_handle_list_ptr->eq_handle);
	P30_Printf("    &event    = %p )\n",&event);
    }
#endif
    
    if ( (_mpi_p30_errno = PtlEQGet( shandle->eq_handle_list_ptr->eq_handle, &event )) == PTL_OK ) {

#       if defined(MPID_DEBUG_ALL)
	if ( MPID_DebugFlag ) {
	    P30_Printf("  got an event");
	    P30_Dump_event( &event );
	    P30_Printf("calling MPID_P30_Free_eq_handle( ptr = %p )\n",
		       shandle->eq_handle_list_ptr);
	    P30_Printf("send with handle %p is done\n",shandle);
	}
#       endif

	/* add the eq handle back into the list */
	MPID_P30_Free_eq_handle( shandle->eq_handle_list_ptr );

	shandle->is_complete = 1;

    } else {
	if ( _mpi_p30_errno != PTL_EQ_EMPTY ) {
	    fprintf(stderr,"%d: (%s:%d) PtlEQGet() failed : %s\n",
		    MPID_MyWorldRank, __FILE__, __LINE__, ptl_err_str[_mpi_p30_errno]);
	}
    }
    
#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
	P30_Printf("leaving MPID_P30_Short_test_send()\n");
    }
#endif

    return MPI_SUCCESS;

}


/***************************************************************************

  MPID_P30_Short_wait_send

  wait for the completion of short protocol send request

  ***************************************************************************/
int MPID_P30_Short_wait_send( shandle )
MPIR_SHANDLE *shandle;
{

    ptl_event_t event;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P30_Printf("entering MPID_P30_Short_wait_send()\n");
	P30_Printf("  calling PtlEQWait(\n");
	P30_Printf("    eq_handle = %d\n",shandle->eq_handle_list_ptr->eq_handle);
	P30_Printf("    &event    = %p )\n",&event);
    }
#endif

    if ( (_mpi_p30_errno = PtlEQWait( shandle->eq_handle_list_ptr->eq_handle, &event )) != PTL_OK ) {
	fprintf(stderr,"%d: (%s:%d) PtlEQWait() failed : %s\n",
		MPID_MyWorldRank, __FILE__, __LINE__, ptl_err_str[_mpi_p30_errno]);
    }

    if( event.type != PTL_EVENT_SENT ) {
	P30_Printf("ERROR: was expecting a PTL_EVENT_SENT\n");
	P30_Dump_event( &event );
    }

#   if defined(MPI_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P30_Printf("calling MPID_P30_Free_eq_handle( ptr = %p )\n",
		   shandle->eq_handle_list_ptr);
	P30_Printf("send with handle %p is done\n",shandle);
    }
#   endif

    /* add the eq handle back into the list */
    MPID_P30_Free_eq_handle( shandle->eq_handle_list_ptr );

    shandle->is_complete = 1;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
	P30_Printf("leaving MPID_P30_Short_wait_send()\n");
    }
#endif

    return MPI_SUCCESS;
}



/***************************************************************************

  MPID_P30_Short_delete

  cleanup

  ***************************************************************************/
void MPID_P30_Short_delete( MPID_Protocol *p )
{

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P30_Printf("entering MPID_P30_Short_delete()\n");
    }
#endif

    FREE( p );

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P30_Printf("leaving MPID_P30_Short_delete()\n");
    }
#endif

}



/***************************************************************************

  MPID_P30_Short_setup

  set up the short protocol structure

  ***************************************************************************/
MPID_Protocol *MPID_P30_Short_setup()
{
    MPID_Protocol *p;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P30_Printf("entering MPID_P30_Short_setup()\n");
    }
#endif

    p = (MPID_Protocol *) MALLOC( sizeof(MPID_Protocol) );
    if (!p) return 0;
    p->send	   = MPID_P30_Short_send;
    p->recv	   = 0;
    p->isend	   = MPID_P30_Short_isend;
    p->ssend	   = MPID_P30_Sshort_send;
    p->issend	   = MPID_P30_Sshort_isend;
    p->wait_send   = 0;
    p->push_send   = 0;
    p->cancel_send = 0;
    p->irecv	   = 0;
    p->wait_recv   = 0;
    p->push_recv   = 0;
    p->cancel_recv = 0;
    p->do_ack      = 0;
    p->unex        = 0;
    p->delete      = MPID_P30_Short_delete;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P30_Printf("leaving MPID_P30_Short_setup()\n");
    }
#endif

    return p;
}
