#include "ptlsdev.h"


/***************************************************************************

  Synchronous short protocol send  

  ***************************************************************************/

/***************************************************************************

  MPID_PTLS_Sshort_send

  short protocol blocking synchronous send

  ***************************************************************************/
int MPID_PTLS_Sshort_send( buf, len, src_lrank, tag, context_id, dest,
				    msgrep, comm )
void *buf;
int  len, tag, context_id, src_lrank, dest, msgrep;
struct MPIR_COMMUNICATOR *comm;
{
    MPIR_SHANDLE shandle;
    int          mpi_errno = MPI_SUCCESS;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("entering MPID_PTLS_Sshort_send()\n");
	PTLS_Printf("calling MPID_PTLS_Sshort_isend()\n");
    }
#endif

    if ( (mpi_errno = MPID_PTLS_Sshort_isend( buf, len, src_lrank, tag, 
					      context_id, dest, msgrep,
					      &shandle, comm ) ) != MPI_SUCCESS ) {
	return mpi_errno;
    }

    mpi_errno = shandle.wait( &shandle );

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("leaving MPID_PTLS_Sshort_send()\n");
    }
#endif

    return mpi_errno;
}


/***************************************************************************
  
  MPID_PTLS_Sshort_isend

  short protocol nonblocking synchronous send

  ***************************************************************************/
int MPID_PTLS_Sshort_isend( buf, len, src_lrank, tag, context_id, dest,
			   msgrep, shandle, comm )
void *buf;
int  len, tag, context_id, src_lrank, dest, msgrep;
MPIR_SHANDLE *shandle;
struct MPIR_COMMUNICATOR *comm;
{
    SEND_INFO_TYPE *send_info;
    IND_MD_TYPE    *iblock;
    int             ack_match_index;

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_SEND)
    if (MPID_DebugFlag) {
        PTLS_Printf("MPID_PTLS_Sshort_isend\n");
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
    send_info->user_data[0]  = PTLS_MSG_SHORT_SYNC;
    send_info->acl_index     = 0;
    send_info->return_portal = ptls_ack_portal;
    send_info->send_flag     = 0;

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_SEND)
    if (MPID_DebugFlag) {
	PTLS_Printf("    getting an mle for an ack\n");
    }
#   endif

    /* get an mle for an ack */
    if ( ack_first_free >= 0 ) {
	ack_match_index = ack_free_list[ack_first_free];
	ack_first_free--;
	shandle->ack_match_desc = &ack_desc_list[ack_match_index];
    }
    else {
	MPIR_ERROR( comm, MPI_ERR_INTERN, "Ack match list entries exhausted" );
    }

    /* set the ack mle's matchbits */
    shandle->ack_match_desc->ign_mbits.ints.i0  = 
    shandle->ack_match_desc->ign_mbits.ints.i1  = 0;
    shandle->ack_match_desc->must_mbits         = send_info->return_matchbits;

    /* attach an ind block to the mle */
    shandle->ack_match_desc->mem_op       = IND_LIN_SV_HDR;
    iblock                                = &shandle->ack_match_desc->mem_desc.ind;
    iblock->num_buf_desc                  = 1;
    iblock->buf_desc_table[0].first_read  = 0;
    iblock->buf_desc_table[0].last_probe  = -1;
    iblock->buf_desc_table[0].next_free   = 0;
    iblock->buf_desc_table[0].ref_count   = 1;
    iblock->buf_desc_table[0].hdr.msg_len = -1;

    /* activate the mle */
    shandle->ack_match_desc->ctl_bits = MCH_OK;

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_SEND)
    if (MPID_DebugFlag) {
	PTLS_Printf("PTLS_Post_send_sync_short-calling portal send-\n");
	PTLS_Printf("   bytes          = %d\n",len);
	PTLS_Printf("   dest           = %d\n",dest);
	PTLS_Printf("   portal         = %d\n",ptls_recv_portal);
	PTLS_Printf("   context id     = %d\n",context_id);
	PTLS_Printf("   local src rank = %d\n",src_lrank);
	PTLS_Printf("   message tag    = %d\n",tag);
	PTLS_Printf("   match_bits     =\n");
	PTLS_Printf("       ints.i0    = 0x%x\n",send_info->dst_matchbits.ints.i0);
	PTLS_Printf("       ints.i1    = 0x%x\n",send_info->dst_matchbits.ints.i1);
	PTLS_Printf("   ret match_bits =\n");
	PTLS_Printf("       ints.i0    = 0x%x\n",send_info->return_matchbits.ints.i0);
	PTLS_Printf("       ints.i1    = 0x%x\n",send_info->return_matchbits.ints.i1);
	PTLS_Printf("   ret portal     = %d\n",send_info->return_portal);
    }
#   endif

    if ( send_user_msg2( (CHAR *)buf, len, dest, ptls_recv_portal, 0, send_info ) ) {
	shandle->errval  = MPI_ERR_INTERN;
	MPIR_RETURN( comm, MPI_ERR_INTERN, "send_user_msg2() failed" );
    }

    shandle->is_complete     = 0;
    shandle->errval          = MPI_SUCCESS;
    shandle->comm            = comm;
    shandle->start           = buf;
    shandle->bytes_as_contig = len;
    shandle->wait	     = MPID_PTLS_Sshort_wait_send;
    shandle->test	     = MPID_PTLS_Sshort_test_send;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("leaving MPID_PTLS_Sshort_isend()\n");
    }
#endif

    return MPI_SUCCESS;
}


/***************************************************************************

  MPID_PTLS_Sshort_wait_send

  wait for short protocol synchronous send request to complete

  ***************************************************************************/
int MPID_PTLS_Sshort_wait_send( shandle )
MPIR_SHANDLE *shandle;
{

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("entering MPID_PTLS_Sshort_wait_send()\n");
	PTLS_Printf("waiting for kernel to process send\n");
    }
#endif

    /* wait for the send to go out */
    while ( shandle->send_info.send_flag == 0 );

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("kernel has processed send\n");
	PTLS_Printf("waiting for ack\n");
    }
#endif

    /* wait for the ack to come back */
    while ( (volatile)shandle->ack_match_desc->mem_desc.ind.buf_desc_table[0].hdr.msg_len 
	   < 0 );
    

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("got an ack\n");
    }
#endif

    ack_first_free++;
    ack_free_list[ack_first_free] = (int)(shandle->ack_match_desc - ack_head_desc);

    shandle->is_complete = 1;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("request is complete\n");
	PTLS_Printf("leaving MPID_PTLS_Sshort_wait_send()\n");
    }
#endif

    return MPI_SUCCESS;

}



/***************************************************************************
  
  MPID_PTLS_Sshort_test_send

  test for the completion of a short protocol synchronous send request

  ***************************************************************************/
int MPID_PTLS_Sshort_test_send( shandle )
MPIR_SHANDLE *shandle;
{
 
#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("entering MPID_PTLS_Sshort_test_send()\n");
    }
#endif

   /* see if the send went out */
    if ( shandle->send_info.send_flag ) {

	/* see if the ack came back */
	if ( shandle->ack_match_desc->mem_desc.ind.buf_desc_table[0].hdr.msg_len == 0 ) {
	    shandle->is_complete = 1;

#ifdef MPID_DEBUG_ALL
	    if ( MPID_DebugFlag ) {
	        PTLS_Printf("request is complete\n");
	    }
#endif

	    ack_first_free++;
	    ack_free_list[ack_first_free] = 
	      (int)(shandle->ack_match_desc - ack_head_desc);

	}
    }

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("leaving MPID_PTLS_Sshort_test_send()\n");
    }
#endif

    return MPI_SUCCESS;
}

