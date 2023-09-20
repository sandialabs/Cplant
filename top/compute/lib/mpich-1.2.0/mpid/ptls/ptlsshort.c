#include "ptlsdev.h"


/***************************************************************************

  Short protocol send
  

  ***************************************************************************/

/***************************************************************************

  MPID_PTLS_Short_send

  blocking short protocol send

  ***************************************************************************/
int MPID_PTLS_Short_send( buf, len, src_lrank, tag, context_id, dest,
				    msgrep, comm )
void *buf;
int  len, tag, context_id, src_lrank, dest, msgrep;
struct MPIR_COMMUNICATOR *comm;
{
    MPIR_SHANDLE shandle;
    int          mpi_errno = MPI_SUCCESS;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("entering MPID_PTLS_Short_send()\n");
    }
#endif

    if ( (mpi_errno = MPID_PTLS_Short_isend( buf, len, src_lrank, tag, 
					     context_id, dest, msgrep, 
					     &shandle, comm ) ) != MPI_SUCCESS ) {
	return mpi_errno;
    }

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("waiting for send to complete\n");
    }
#endif

    mpi_errno = shandle.wait( &shandle );

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("leaving MPID_PTLS_Short_send()\n");
    }
#endif

    return mpi_errno;
}


/***************************************************************************

  MPID_PTLS_Short_isend
  
  nonblocking short protocol send

  ***************************************************************************/
int MPID_PTLS_Short_isend( buf, len, src_lrank, tag, context_id, dest,
			   msgrep, shandle, comm )
void *buf;
int  len, tag, context_id, src_lrank, dest, msgrep;
MPIR_SHANDLE *shandle;
struct MPIR_COMMUNICATOR *comm;
{
    SEND_INFO_TYPE *send_info;

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_SEND)
    if (MPID_DebugFlag) {
        PTLS_Printf("PTLS_Post_send_short\n");
    }
#   endif

    send_info = &shandle->send_info;

    /* fill in the send info structure */
    PTLS_SEND_SET_MATCHBITS( send_info->dst_matchbits, context_id, src_lrank,
			     PTLS_SHORT_MSG_MASK, tag );

    send_info->dst_offset    = 0;
    send_info->reply_offset  = 0;
    
    memcpy( &send_info->return_matchbits, &shandle, sizeof(shandle) );

    send_info->reply_len     = 0;
    send_info->user_data[0]  = PTLS_MSG_SHORT;
    send_info->acl_index     = 0;
    send_info->return_portal = PTLS_PORTAL_NO_ACK;
    send_info->send_flag     = 0;


#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_SEND)
    if (MPID_DebugFlag) {
	PTLS_Printf("PTLS_Post_send_short-calling portal send-\n");
	PTLS_Printf("   bytes          = %d\n",len);
	PTLS_Printf("   dest           = %d\n",dest);
	PTLS_Printf("   portal         = %d\n",ptls_recv_portal);
	PTLS_Printf("   context id     = %d\n",context_id);
	PTLS_Printf("   local src rank = %d\n",src_lrank);
	PTLS_Printf("   message tag    = %d\n",tag);
	PTLS_Printf("   match_bits     =\n");
	PTLS_Printf("       ints.i0    = 0x%x\n",send_info->dst_matchbits.ints.i0);
	PTLS_Printf("       ints.i1    = 0x%x\n",send_info->dst_matchbits.ints.i1);
	PTLS_Printf("   ret portal     = %d\n",send_info->return_portal);
	PTLS_Printf("   ret match_bits =\n");
	PTLS_Printf("       ints.i0    = 0x%x\n",send_info->return_matchbits.ints.i0);
	PTLS_Printf("       ints.i1    = 0x%x\n",send_info->return_matchbits.ints.i1);
    }
#   endif
    
    if ( send_user_msg2( (CHAR *)buf, len, dest, ptls_recv_portal, 0, send_info ) ) {
	shandle->errval     = MPI_ERR_INTERN;
	MPIR_RETURN( comm, MPI_ERR_INTERN, "send_user_msg2() failed" );
    }


    shandle->is_complete     = 0;
    shandle->errval          = MPI_SUCCESS;
    shandle->comm            = comm;
    shandle->start           = buf;
    shandle->bytes_as_contig = len;
    shandle->test	     = MPID_PTLS_Short_test_send;
    shandle->wait	     = MPID_PTLS_Short_wait_send;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("leaving MPID_PTLS_Short_isend()\n");
    }
#endif

    return MPI_SUCCESS;


}

/***************************************************************************

  MPID_PTLS_Short_bsend
  
  nonblocking short protocol buffered send

  ***************************************************************************/
int MPID_PTLS_Short_bsend( buf, len, src_lrank, tag, context_id, dest,
			   shandle, comm )
void *buf;
int  len, tag, context_id, src_lrank, dest;
MPIR_SHANDLE *shandle;
struct MPIR_COMMUNICATOR *comm;
{
    
    MPID_PTLS_Short_isend( buf, len, src_lrank, tag, context_id, dest,
			 0, shandle, comm );

    return MPID_PTLS_Short_wait_send( shandle );
    
}


/***************************************************************************

  MPID_PTLS_Short_test_send

  test for the completion of short protocol send request

  ***************************************************************************/
int MPID_PTLS_Short_test_send( shandle )
MPIR_SHANDLE *shandle;
{

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("entering MPID_PTLS_Short_test_send()\n");
    }
#endif

    shandle->is_complete = shandle->send_info.send_flag;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        if ( shandle->is_complete ) {
   	    PTLS_Printf("kernel has processed send\n");
	    PTLS_Printf("request is complete\n");
	}
	else {
   	    PTLS_Printf("kernel has not processed send\n");
	    PTLS_Printf("request is not complete\n");
	}
	PTLS_Printf("leaving MPID_PTLS_Short_test_send()\n");
    }
#endif

    return MPI_SUCCESS;

}


/***************************************************************************

  MPID_PTLS_Short_wait_send

  wait for the completion of short protocol send request

  ***************************************************************************/
int MPID_PTLS_Short_wait_send( shandle )
MPIR_SHANDLE *shandle;
{

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("entering MPID_PTLS_Short_wait_send()\n");
	PTLS_Printf("waiting for kernel to process send\n");
    }
#endif

    while ( shandle->send_info.send_flag == 0 );

    shandle->is_complete = 1;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("kernel has processed send\n");
	PTLS_Printf("request is complete\n");
	PTLS_Printf("leaving MPID_PTLS_Short_wait_send()\n");
    }
#endif

    return MPI_SUCCESS;
}



/***************************************************************************

  MPID_PTLS_Short_delete

  cleanup

  ***************************************************************************/
void MPID_PTLS_Short_delete( MPID_Protocol *p )
{

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("entering MPID_PTLS_Short_delete()\n");
    }
#endif

    FREE( p );

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("leaving MPID_PTLS_Short_delete()\n");
    }
#endif

}



/***************************************************************************

  MPID_PTLS_Short_setup

  set up the short protocol structure

  ***************************************************************************/
MPID_Protocol *MPID_PTLS_Short_setup()
{
    MPID_Protocol *p;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("entering MPID_PTLS_Short_setup()\n");
    }
#endif

    p = (MPID_Protocol *) MALLOC( sizeof(MPID_Protocol) );
    if (!p) return 0;
    p->send	   = MPID_PTLS_Short_send;
    p->recv	   = 0;
    p->isend	   = MPID_PTLS_Short_isend;
    p->ssend	   = MPID_PTLS_Sshort_send;
    p->issend	   = MPID_PTLS_Sshort_isend;
    p->wait_send   = 0;
    p->push_send   = 0;
    p->cancel_send = 0;
    p->irecv	   = 0;
    p->wait_recv   = 0;
    p->push_recv   = 0;
    p->cancel_recv = 0;
    p->do_ack      = 0;
    p->unex        = 0;
    p->delete      = MPID_PTLS_Short_delete;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("leaving MPID_PTLS_Short_setup()\n");
    }
#endif

    return p;
}
