#include "ptlsdev.h"


/***************************************************************************

  Ready send
  

  ***************************************************************************/


/***************************************************************************

  MPID_PTLS_Ready_send

  ***************************************************************************/
int MPID_PTLS_Ready_send( buf, len, src_lrank, tag, context_id, dest,
			  msgrep, comm )
void *buf;
int  len, tag, context_id, src_lrank, dest, msgrep;
struct MPIR_COMMUNICATOR *comm;
{
    MPIR_SHANDLE shandle;
    int          mpi_errno = MPI_SUCCESS;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("entering MPID_PTLS_Ready_send()\n");
	PTLS_Printf("calling MPID_PTLS_Ready_isend()\n");
    }
#endif

    if ( (mpi_errno = MPID_PTLS_Ready_isend( buf, len, src_lrank, tag,
					     context_id, dest, msgrep,
					     &shandle, comm ) ) != MPI_SUCCESS ) {
	return mpi_errno;
    }

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("calling MPID_PTLS_Ready_wait_send()\n");
    }
#endif

    mpi_errno = shandle.wait( &shandle );

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("leaving MPID_PTLS_Ready_send()\n");
    }
#endif

    return mpi_errno;
}


/***************************************************************************

  MPID_PTLS_Ready_isend
  
  nonblocking ready send

  ***************************************************************************/
int MPID_PTLS_Ready_isend( buf, len, src_lrank, tag, context_id, dest,
			   msgrep, shandle, comm )
void *buf;
int  len, tag, context_id, src_lrank, dest, msgrep;
MPIR_SHANDLE *shandle;
struct MPIR_COMMUNICATOR *comm;
{
    SEND_INFO_TYPE *send_info;

    send_info = &shandle->send_info;

    /* fill in the send info structure */
    PTLS_SEND_SET_MATCHBITS( send_info->dst_matchbits, context_id, src_lrank,
			     PTLS_READY_MSG_MASK, tag );

    send_info->dst_offset               =
    send_info->reply_offset             =
    send_info->return_matchbits.ints.i0 =
    send_info->return_matchbits.ints.i1 =
    send_info->reply_len                = 
    send_info->user_data[0]             = PTLS_MSG_READY;
    send_info->acl_index                = 0;
    send_info->return_portal            = PTLS_PORTAL_NO_ACK;
    send_info->send_flag                = 0;

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_SEND)
    if (MPID_DebugFlag) {
	PTLS_Printf("PTLS_Post_send_ready-calling portal send-\n");
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
    
    if ( send_user_msg2( (CHAR *)buf,
			 len,
			 dest,
			 ptls_recv_portal,
			 0,
			 &shandle->send_info ) ) {
	shandle->errval  = MPI_ERR_INTERN;
	MPIR_RETURN( comm, MPI_ERR_INTERN, "send_user_msg2() failed" );
    }

    shandle->is_complete     = 0;
    shandle->errval          = MPI_SUCCESS;
    shandle->comm            = comm;
    shandle->start           = buf;
    shandle->bytes_as_contig = len;
    shandle->wait	     = MPID_PTLS_Short_wait_send;
    shandle->test	     = MPID_PTLS_Short_test_send;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("leaving MPID_PTLS_Ready_isend()\n");
    }
#endif

    return MPI_SUCCESS;
}


/***************************************************************************

  MPID_PTLS_Ready_test_send

  ***************************************************************************/
int MPID_PTLS_Ready_test_send( shandle )
MPIR_SHANDLE *shandle;
{

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("entering MPID_PTLS_Ready_test_send()\n");
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
	PTLS_Printf("leaving MPID_PTLS_Short_wait_send()\n");
    }
#endif

    return MPI_SUCCESS;
}

/***************************************************************************

  MPID_PTLS_Ready_wait_send

  ***************************************************************************/
int MPID_PTLS_Ready_wait_send( shandle )
MPIR_SHANDLE *shandle;
{

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("entering MPID_PTLS_Ready_wait_send()\n");
	PTLS_Printf("waiting for kernel to process send\n");
    }
#endif

    while ( shandle->send_info.send_flag == 0 ) {
#ifdef SCHED_YIELD
        sched_yield();
#endif
    }

    shandle->is_complete = 1;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("kernel has processed send\n");
	PTLS_Printf("request is complete\n");
	PTLS_Printf("leaving MPID_PTLS_Ready_wait_send()\n");
    }
#endif

    return MPI_SUCCESS;
}



/***************************************************************************

  MPID_PTLS_Short_delete

  cleanup

  ***************************************************************************/
void MPID_PTLS_Ready_delete( MPID_Protocol *p )
{

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("entering MPID_PTLS_Ready_delete()\n");
    }
#endif

    FREE( p );

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("leaving MPID_PTLS_Ready_delete()\n");
    }
#endif

}



/***************************************************************************

  MPID_PTLS_Ready_setup
  
  set up the ready protocol structure

  ***************************************************************************/
MPID_Protocol *MPID_PTLS_Ready_setup()
{
    MPID_Protocol *p;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("entering MPID_PTLS_Ready_setup()\n");
    }
#endif

    p = (MPID_Protocol *) MALLOC( sizeof(MPID_Protocol) );
    if (!p) return 0;
    p->send	   = MPID_PTLS_Ready_send;
    p->recv	   = 0;
    p->isend	   = MPID_PTLS_Ready_isend;
    p->ssend	   = 0;
    p->issend	   = 0;
    p->wait_send   = 0;
    p->push_send   = 0;
    p->cancel_send = 0;
    p->irecv	   = 0;
    p->wait_recv   = 0;
    p->push_recv   = 0;
    p->cancel_recv = 0;
    p->do_ack      = 0;
    p->unex        = 0;
    p->delete      = 0;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("leaving MPID_PTLS_Short_setup()\n");
    }
#endif

    return p;
}
