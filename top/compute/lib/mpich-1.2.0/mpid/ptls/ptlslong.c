#include "ptlsdev.h"

/***************************************************************************

  Long protocol send  

  ***************************************************************************/


/***************************************************************************

  MPID_PTLS_Long_send

  blocking long protocol send

  ***************************************************************************/
int MPID_PTLS_Long_send( buf, len, src_lrank, tag, context_id, dest,
				    msgrep, comm )
void *buf;
int  len, tag, context_id, src_lrank, dest, msgrep;
struct MPIR_COMMUNICATOR *comm;
{
    MPIR_SHANDLE shandle;
    int          mpi_errno = MPI_SUCCESS;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("entering MPID_PTLS_Long_send()\n");
    }
#endif

    if ( (mpi_errno = MPID_PTLS_Long_isend( buf, len, src_lrank, tag, 
					    context_id, dest, msgrep, 
					    &shandle, comm ) ) != MPI_SUCCESS ) {
	return mpi_errno;
    }

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("calling MPID_PTLS_Long_wait_send()\n");
    }
#endif

    mpi_errno = shandle.wait( &shandle );

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("leaving MPID_PTLS_Long_send()\n");
    }
#endif

    return mpi_errno;
}


/***************************************************************************

  MPID_PTLS_Long_isend

  nonblocking long protocol send

  ***************************************************************************/
int MPID_PTLS_Long_isend( buf, len, src_lrank, tag, context_id, dest,
			   msgrep, shandle, comm )
void *buf;
int  len, tag, context_id, src_lrank, dest, msgrep;
MPIR_SHANDLE *shandle;
struct MPIR_COMMUNICATOR *comm;
{
    SEND_INFO_TYPE *send_info;
    SINGLE_MD_TYPE *sblock;
    IND_MD_TYPE    *iblock;
    UINT32          read_match_index;
    int             ack_match_index;
    int             i;

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_SEND)
    if (MPID_DebugFlag) {
        PTLS_Printf("MPID_PTLS_Long_isend\n");
    }
#   endif

    send_info = &shandle->send_info;

    /* fill in the send info structure */
    PTLS_SEND_SET_MATCHBITS( send_info->dst_matchbits, context_id, src_lrank,
			     PTLS_LONG_MSG_MASK, tag );

    send_info->dst_offset    = 0;
    send_info->reply_offset  = 0;

    memcpy( &send_info->return_matchbits, &shandle, sizeof(CHAMELEON) );

    send_info->reply_len     = 0;
    send_info->user_data[0]  = PTLS_MSG_LONG;
    send_info->acl_index     = 0;
    send_info->return_portal = ptls_ack_portal;
    send_info->send_flag     = 0;



#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_SEND)
    if (MPID_DebugFlag) {
        PTLS_Printf("   getting an mle for reading\n");
    }
#   endif

    /* get an mle for reading */
    if ( read_first_free >= 0 ) {
	read_match_index = read_free_list[read_first_free];
	read_first_free--;
	shandle->read_match_desc = &read_desc_list[read_match_index];
    }
    else {
	MPIR_ERROR( comm, MPI_ERR_INTERN, "Read match list entries exhausted" );
    }

    /* set the read mle's matchbits */
    shandle->read_match_desc->ign_mbits.ints.i0  = 
    shandle->read_match_desc->ign_mbits.ints.i1  = 0;
    shandle->read_match_desc->must_mbits         = send_info->return_matchbits;

    /* attach a single block to the read mle */
    shandle->read_match_desc->mem_op             = SINGLE_SNDR_OFF_RPLY;
    sblock                 = &shandle->read_match_desc->mem_desc.single;
    sblock->buf            = buf;
    sblock->buf_len        = len;
    sblock->rw_bytes       = -1;
    sblock->msg_cnt	   = 0;

    /* lock down the reply buffer */
    if ( portal_lock_buffer( buf, len ) < 0 ) {
        fprintf(stderr,"%s:%d portal_lock_buffer() failed\n",__FILE__,__LINE__ )
;
    }

    /* activate the mle */
    shandle->read_match_desc->ctl_bits = MCH_OK;

    
#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_SEND)
    if (MPID_DebugFlag) {
	PTLS_Printf("    got match list entry number %d from portal %d\n",
		    read_match_index,
		    ptls_read_portal );
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

    /* attach an independent block to the ack mle */
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
	PTLS_Printf("    got ack match index %d\n",ack_match_index);
	PTLS_Printf("PTLS_Post_send_long-calling portal send-\n");
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
	shandle->errval = MPI_ERR_INTERN;
	MPIR_RETURN( comm, MPI_ERR_INTERN, "send_user_msg2() failed" );
    }

    shandle->is_complete     = 0;
    shandle->errval          = MPI_SUCCESS;
    shandle->comm            = comm;
    shandle->start           = buf;
    shandle->bytes_as_contig = len;
    shandle->wait	     = MPID_PTLS_Long_wait_send;
    shandle->test	     = MPID_PTLS_Long_test_send;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("leaving MPID_PTLS_Long_isend()\n");
    }
#endif

    return MPI_SUCCESS;
}




/***************************************************************************

  MPID_PTLS_Long_test_send

  test to see if the kernel has processed the send

  ***************************************************************************/
int MPID_PTLS_Long_test_send( shandle )
MPIR_SHANDLE *shandle;
{
    
    int mpi_errno = MPI_SUCCESS;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("entering MPID_PTLS_Long_test_send()\n");
    }
#endif

    if ( shandle->send_info.send_flag ) {

#ifdef MPID_DEBUG_ALL
        if ( MPID_DebugFlag ) {
	    PTLS_Printf("kernel has processed send\n");
	    PTLS_Printf("checking for ack\n");
	    PTLS_Printf("calling MPID_PTLS_Long_test_send_ack()\n");
	}
#endif

	shandle->wait = MPID_PTLS_Long_wait_send_ack;
	shandle->test = MPID_PTLS_Long_test_send_ack;

	mpi_errno = shandle->test( shandle );

    }

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("leaving MPID_PTLS_Long_test_send()\n");
    }
#endif

    return mpi_errno;

}



/***************************************************************************

  MPID_PTLS_Long_test_send_ack

  test for arrival of an acknowledgment

  ***************************************************************************/
int MPID_PTLS_Long_test_send_ack( shandle )
MPIR_SHANDLE *shandle;
{

    int mpi_errno = MPI_SUCCESS;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("entering MPID_PTLS_Long_test_send_ack()\n");
    }
#endif

    /* check for a recv ack */
    if ( shandle->ack_match_desc->mem_desc.ind.buf_desc_table[0].hdr.msg_len == 0 ) {

#ifdef MPID_DEBUG_ALL
        if ( MPID_DebugFlag ) {
	    PTLS_Printf("got an ack\n");
	}
#endif

	/* got a recv ack, check to see what happened on the recv side */
	if ( shandle->ack_match_desc->mem_desc.ind.buf_desc_table[0].hdr.user_data[0] ==
	     ACK_SVD_HDR_BDY ) {

#ifdef MPID_DEBUG_ALL
	    if ( MPID_DebugFlag ) {
	        PTLS_Printf("both header and body were saved\n");
		PTLS_Printf("request is complete\n");
	    }
#endif

	    /* saved both header and body, so we're done */
	    shandle->is_complete = 1;

	    /* free up the ack descriptor */
	    ack_first_free++;
	    ack_free_list[ack_first_free] = 
	      (int)(shandle->ack_match_desc - ack_head_desc);

	    /* free up the read descriptor */
	    read_first_free++;
	    read_free_list[read_first_free] = 
	      (int)(shandle->read_match_desc - read_head_desc);

            if ( portal_unlock_buffer( shandle->start, shandle->bytes_as_contig) < 0 ) {
                fprintf(stderr,"%s:%d portal_unlock_buffer() failed\n",__FILE__,__LINE__ );
            }


	}
	else {

#ifdef MPID_DEBUG_ALL
	    if ( MPID_DebugFlag ) {
	        PTLS_Printf("only header was saved\n");
		PTLS_Printf("checking whether buffer was read\n");
		PTLS_Printf("calling MPID_PTLS_Long_test_send_read()\n");
	    }
#endif

	    /* only saved header, see if data has been read */
	    shandle->wait = MPID_PTLS_Long_wait_send_read;
	    shandle->test = MPID_PTLS_Long_test_send_read;
	    
	    mpi_errno = shandle->test( shandle );
	}
    }

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("leaving MPID_PTLS_Long_test_send_ack()\n");
    }
#endif

    return mpi_errno;

}



/***************************************************************************

  MPID_PTLS_Long_test_send_read

  test for reading of the send buffer

  ***************************************************************************/
int MPID_PTLS_Long_test_send_read( shandle )
MPIR_SHANDLE *shandle;
{

    int mpi_errno = MPI_SUCCESS;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("entering MPID_PTLS_Long_test_send_read()\n");
    }
#endif

    /* see if the data has been read */
    if ( shandle->is_complete = 
	 (shandle->read_match_desc->mem_desc.single.rw_bytes == 
	  shandle->bytes_as_contig) &&
/*	 (shandle->read_match_desc->mem_desc.single.msg_cnt > 0) && */
	 (PTL_DESC(ptls_read_portal).active_cnt == 0 )  ) {

	/* free up the ack descriptor */
	ack_first_free++;
	ack_free_list[ack_first_free] = 
	  (int)(shandle->ack_match_desc - ack_head_desc);

	/* free up the read descriptor */
	read_first_free++;
	read_free_list[read_first_free] = 
	  (int)(shandle->read_match_desc - read_head_desc);

        if ( portal_unlock_buffer( shandle->start, shandle->bytes_as_contig ) < 0 ) {
            fprintf(stderr,"%s:%d portal_unlock_buffer() failed\n",__FILE__,__LINE__ );
        }

    }

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        if ( shandle->is_complete ) {
	    PTLS_Printf("request is complete\n");
	}
	else {
	    PTLS_Printf("request is not complete\n");
	}
        PTLS_Printf("leaving MPID_PTLS_Long_test_send_read()\n");
    }
#endif

    return mpi_errno;

}



/***************************************************************************

  MPID_PTLS_Long_wait_send

  wait for completion of long protocol send

  ***************************************************************************/
int MPID_PTLS_Long_wait_send( shandle )
MPIR_SHANDLE *shandle;
{

    int mpi_errno = MPI_SUCCESS;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("entering MPID_PTLS_Long_wait_send()\n");
    }
#endif

    while ( shandle->send_info.send_flag == 0 ) {
#ifdef SCHED_YIELD
        sched_yield();
#endif
    }

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("kernel has processed send\n");
	PTLS_Printf("checking for ack\n");
	PTLS_Printf("calling MPID_PTLS_Long_wait_send_ack()\n");
    }
#endif

    shandle->wait = MPID_PTLS_Long_wait_send_ack;
    shandle->test = MPID_PTLS_Long_test_send_ack;

    mpi_errno = shandle->wait( shandle );

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("leaving MPID_PTLS_Long_wait_send()\n");
    }
#endif

    return mpi_errno;
}



/***************************************************************************

  MPID_PTLS_Long_wait_send_ack

  wait for an acknowledgment to arrive

  ***************************************************************************/
int MPID_PTLS_Long_wait_send_ack( shandle )
MPIR_SHANDLE *shandle;
{

    int mpi_errno = MPI_SUCCESS;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("entering MPID_PTLS_Long_wait_send_ack()\n");
	PTLS_Printf("waiting for an ack\n");
	PTLS_Printf("shandle->ack_desc = 0x%x\n",shandle->ack_match_desc);
    }
#endif

    /* wait for the recv ack */
    while ( (volatile)shandle->ack_match_desc->mem_desc.ind.buf_desc_table[0].hdr.msg_len 
	    < 0 ) {
#ifdef SCHED_YIELD
        sched_yield();
#endif
    }

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("got an ack\n");
    }
#endif

    if ( shandle->ack_match_desc->mem_desc.ind.buf_desc_table[0].hdr.user_data[0] ==
	ACK_SVD_HDR_BDY ) {

#ifdef MPID_DEBUG_ALL
        if ( MPID_DebugFlag ) {
	    PTLS_Printf("saved both header and body\n");
	    PTLS_Printf("request is complete\n");
	}
#endif

	/* saved both header and body, so we're done */
	shandle->is_complete = 1;

	/* free up the ack descriptor */
	ack_first_free++;
	ack_free_list[ack_first_free] = 
	  (int)(shandle->ack_match_desc - ack_head_desc);

	/* free up the read descriptor */
	read_first_free++;
	read_free_list[read_first_free] = 
	  (int)(shandle->read_match_desc - read_head_desc);

        if ( portal_unlock_buffer( shandle->start, shandle->bytes_as_contig ) < 0 ) {
            fprintf(stderr,"%s:%d portal_unlock_buffer() failed\n",__FILE__,__LINE__ );
        }

   }
    else {

#ifdef MPID_DEBUG_ALL
        if ( MPID_DebugFlag ) {
	    PTLS_Printf("only saved header\n");
	    PTLS_Printf("checking to see if buffer was read\n");
	    PTLS_Printf("calling MPID_PTLS_Long_wait_send_read()\n");
	}
#endif

	/* only saved header, see if data has been read */
	shandle->wait = MPID_PTLS_Long_wait_send_read;
	shandle->test = MPID_PTLS_Long_test_send_read;

	mpi_errno = shandle->wait( shandle );

    }

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("leaving MPID_PTLS_Long_wait_send_ack()\n");
    }
#endif

    return mpi_errno;

}



/***************************************************************************

  MPID_PTLS_Long_wait_send_read

  wait for the send buffer to be read

  ***************************************************************************/
int MPID_PTLS_Long_wait_send_read( shandle )
MPIR_SHANDLE *shandle;
{

    int mpi_errno = MPI_SUCCESS;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("entering MPID_PTLS_Long_wait_send_read()\n");
	PTLS_Printf("waiting for buffer to be read\n");
	PTLS_Printf("read_desc->matchbits = 0x%8x 0x%8x\n",
		    shandle->read_match_desc->must_mbits.ints.i0,
		    shandle->read_match_desc->must_mbits.ints.i1 );
	PTLS_Printf("recv_first_free = %d\n",recv_first_free);
    }
#endif

    /* wait for the data to be read */
    while ( (volatile)shandle->read_match_desc->mem_desc.single.rw_bytes < shandle->bytes_as_contig ||
	 (PTL_DESC(ptls_read_portal).active_cnt != 0 )  ) {
#ifdef SCHED_YIELD
        sched_yield();
#endif
    }

    shandle->is_complete = 1;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("%d bytes have been read\n",
		    shandle->read_match_desc->mem_desc.single.rw_bytes);
	PTLS_Printf("request is complete\n");
    }
#endif

    /* free up the ack descriptor */
    ack_first_free++;
    ack_free_list[ack_first_free] = 
      (int)(shandle->ack_match_desc - ack_head_desc);

    /* free up the read descriptor */
    read_first_free++;
    read_free_list[read_first_free] = 
      (int)(shandle->read_match_desc - read_head_desc);

    if ( portal_unlock_buffer( shandle->start, shandle->bytes_as_contig ) < 0 )
    {
        fprintf(stderr,"%s:%d portal_unlock_buffer() failed\n",__FILE__,__LINE__ );
    }

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("leaving MPID_PTLS_Long_wait_send_read()\n");
    }
#endif
    
    return mpi_errno;

}


/***************************************************************************

  MPID_PTLS_Long_delete

  cleanup

  ***************************************************************************/
void MPID_PTLS_Long_delete( MPID_Protocol *p )
{

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("entering MPID_PTLS_Long_delete()\n");
    }
#endif

    FREE( p );

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("leaving MPID_PTLS_Long_delete()\n");
    }
#endif

}



/***************************************************************************

  MPID_PTLS_Long_setup

  set up the long protocol structure

  ***************************************************************************/
MPID_Protocol *MPID_PTLS_Long_setup()
{
    MPID_Protocol *p;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("entering MPID_PTLS_Long_setup()\n");
    }
#endif

    p = (MPID_Protocol *) MALLOC( sizeof(MPID_Protocol) );
    if (!p) return 0;
    p->send	   = MPID_PTLS_Long_send;
    p->recv	   = 0;
    p->isend	   = MPID_PTLS_Long_isend;
    p->ssend	   = MPID_PTLS_Long_send;
    p->issend	   = MPID_PTLS_Long_isend;
    p->wait_send   = 0;
    p->push_send   = 0;
    p->cancel_send = 0;
    p->irecv	   = 0;
    p->wait_recv   = 0;
    p->push_recv   = 0;
    p->cancel_recv = 0;
    p->do_ack      = 0;
    p->unex        = 0;
    p->delete      = MPID_PTLS_Long_delete;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("leaving MPID_PTLS_Long_setup()\n");
    }
#endif

    return p;
}
