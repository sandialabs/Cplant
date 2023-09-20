
#include "mpiddev.h"
#include "queue.h"

/***************************************************************************

  MPID_Search_unexpected_queue

  Look through the dynamic heap for an unexpected message that matches

  ***************************************************************************/
int MPID_Search_unexpected_queue( struct MPIR_COMMUNICATOR *comm, int src, 
				  int tag, int context_id, int *flag, 
				  MPI_Status *status )
{
    CHAMELEON        matchbits,ignorebits;
    DYN_MD_TYPE     *dblock;
    DYN_MSG_LINK    *msglink;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
	PTLS_Printf("entering PTLS_Search_unexpected_queue()\n");
    }
#endif

    status->MPI_ERROR = MPI_SUCCESS;

    PTLS_RECV_SET_MATCHBITS( matchbits, ignorebits, context_id, src, tag );

    dblock = &recv_short_catchall_desc->mem_desc.dyn;

    msglink = sptl_dyn_srch( dblock,
			     ANY_RANK,
			     ANY_GROUP,
			     ignorebits,
			     matchbits,
			     ptls_recv_portal,
			     TRUE,
			     (BOOLEAN *)flag );
			     
    if (*flag) {
        /* Copy relevant data to status */
        status->count      = msglink->hdr.msg_len;
        PTLS_GET_LRANK( status->MPI_SOURCE, msglink->hdr.dst_mbits );
        PTLS_GET_TAG(   status->MPI_TAG,    msglink->hdr.dst_mbits );
    }

    return MPI_SUCCESS;
}


/***************************************************************************

  MPID_Search_unexpected_queue_and_post

  Look through the dynamic heap for an unexpected message that matches
  and post a receive if one is not found

  ***************************************************************************/
void MPID_Search_unexpected_queue_and_post(int src_lrank, int tag, int context_id,
					   MPIR_RHANDLE *rhandle )
{
    CHAMELEON        ignorebits;
    CHAMELEON        matchbits;
    MATCH_DESC_TYPE *prev_match_desc;
    IND_MD_TYPE     *iblock;
    DYN_MD_TYPE     *dtype;
    DYN_MSG_LINK    *msglink;
    UINT32           new_match_index;
    BOOLEAN          found;
    UINT8            msg_type;

    rhandle->s.MPI_ERROR = MPI_SUCCESS;

    PTLS_RECV_SET_MATCHBITS( matchbits, 
			     ignorebits, 
			     context_id,
			     src_lrank,
			     tag );

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_RECV)
    if (MPID_DebugFlag) {
	PTLS_Printf("PTLS_Post_recv -- getting match list entry\n");
	PTLS_Printf("   context_id     = %d\n",context_id);
	PTLS_Printf("   src_lrank      = %d\n",src_lrank);
	PTLS_Printf("   tag            = %d\n",tag);
	PTLS_Printf("   ignorebits     =\n");
	PTLS_Printf("       ints.i0    = 0x%x\n",ignorebits.ints.i0);
	PTLS_Printf("       ints.i1    = 0x%x\n",ignorebits.ints.i1);
	PTLS_Printf("   matchbits      =\n");
	PTLS_Printf("       ints.i0    = 0x%x\n",matchbits.ints.i0);
	PTLS_Printf("       ints.i1    = 0x%x\n",matchbits.ints.i1);
    }
#   endif

    /* Get the next regular mle from the free list */
    if ( recv_first_free >= 0 ) {
	new_match_index = recv_free_list[recv_first_free];
	recv_first_free--;
	rhandle->recv_match_desc = &recv_desc_list[ new_match_index ];
    }
    else {
	rhandle->s.MPI_ERROR = MPI_ERR_INTERN;
	MPIR_ERROR( rhandle->comm, MPI_ERR_INTERN, "Ran out of match list entries" );
	return;
    }

    /* Modify the matchbits */
    rhandle->recv_match_desc->ign_mbits         = ignorebits;
    rhandle->recv_match_desc->must_mbits        = matchbits;

    /* Set the new mle pointers */
    rhandle->recv_match_desc->next              = 
      rhandle->recv_match_desc->next_on_nobuf   = 
	recv_short_catchall_match_index;
    rhandle->recv_match_desc->next_on_nofit     = -1;

    recv_previous_list[new_match_index] = recv_last_reg_index;
    prev_match_desc                     = &recv_desc_list[recv_last_reg_index];
    recv_last_reg_index                 = new_match_index;
    recv_last_reg_desc                  = rhandle->recv_match_desc; 
    
    prev_match_desc->next               = 
      prev_match_desc->next_on_nobuf    = new_match_index;
    prev_match_desc->next_on_nofit      = -1;

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_RECV)
    if (MPID_DebugFlag) {
	PTLS_Printf("got match list entry number %d from portal %d\n",
		    new_match_index,
		    ptls_recv_portal );
	PTLS_Printf("previous index = %d\n",(int)(prev_match_desc-recv_head_desc));
    }
#   endif

    rhandle->recv_match_desc->mem_op = IND_CIRC_SV_HDR_BDY_ACK;

    /* Attach an ind block to the mle */
    iblock                                = 
      &rhandle->recv_match_desc->mem_desc.ind;
    iblock->num_buf_desc                  = 1;
    iblock->buf_desc_table[0].first_read  = 0;
    iblock->buf_desc_table[0].last_probe  = -1;
    iblock->buf_desc_table[0].next_free   = 0;
    iblock->buf_desc_table[0].ref_count   = 1;
    iblock->buf_desc_table[0].hdr.msg_len = -1;
    iblock->buf_desc_table[0].buf         = (CHAR *)rhandle->buf;
    iblock->buf_desc_table[0].buf_len     = (UINT32)rhandle->len;

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_RECV)
    if (MPID_DebugFlag) {
	PTLS_Printf("PTLS_Post_recv-calling ptl_dyn_find_activate-\n");
	PTLS_Printf("   portal         = %d\n",ptls_recv_portal);
	PTLS_Printf("   bytes          = %d\n",rhandle->len);
	PTLS_Printf("   source         = %d\n",-1);
	PTLS_Printf("   group          = %d\n",-1);
	PTLS_Printf("   ignore bits\n");
	PTLS_Printf("       ints.i0    = 0x%x\n",ignorebits.ints.i0);
	PTLS_Printf("       ints.i1    = 0x%x\n",ignorebits.ints.i1);
	PTLS_Printf("   match_bits     =\n");
	PTLS_Printf("       ints.i0    = 0x%x\n",matchbits.ints.i0);
	PTLS_Printf("       ints.i1    = 0x%x\n",matchbits.ints.i1);
    }
#   endif

    /* check the dynamic heap to see if anything's been dropped */
    if ( (recv_short_catchall_desc->ctl_bits & MCH_FAIL_ON_NOFIT) ||
	(recv_long_catchall_desc->ctl_bits  & MCH_FAIL_ON_NOFIT)    ) {
	rhandle->s.MPI_ERROR = MPI_ERR_INTERN;
        MPIR_ERROR( rhandle->comm, MPI_ERR_INTERN, "MPI heap overflow" );
	return;
    }
   
    /* clear the control bits */
    rhandle->recv_match_desc->ctl_bits = MCH_NOT_ACTIVE;
 
    /* lock down the buffer in case the message hasn't arrived yet */
    if ( portal_lock_buffer( rhandle->buf, rhandle->len ) < 0 ) {
        fprintf(stderr,"%s:%d portal_lock_buffer() failed\n",__FILE__,__LINE__);
    }

    /* search the dynamic heap to see if it's here */
    dtype   = &recv_short_catchall_desc->mem_desc.dyn;
    msglink = sptl_dyn_srch_act( dtype, ANY_RANK, ANY_GROUP,
				 ignorebits, matchbits, ptls_recv_portal, FALSE, 
				 &found, NULL, 
				 (MATCH_CTRL_TYPE *)
				 &rhandle->recv_match_desc->ctl_bits );

      if (found) {

	  PTLS_GET_LRANK( src_lrank, msglink->hdr.dst_mbits );
	  PTLS_GET_TAG(   tag,       msglink->hdr.dst_mbits );

#         if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_RECV)
	  if (MPID_DebugFlag) {
	      PTLS_Printf("   len     = %d\n",rhandle->len);
	      PTLS_Printf("   msg_len = %d\n",msglink->hdr.msg_len);
	      PTLS_Printf("   source  = %d\n",src_lrank);
              PTLS_Printf("   ints.i0 = 0x%x\n",msglink->hdr.dst_mbits.ints.i0);
              PTLS_Printf("   ints.i1 = 0x%x\n",msglink->hdr.dst_mbits.ints.i1);
              PTLS_Printf("   tag     = %d\n",tag); 
          } 
#         endif


	  if ( rhandle->len < msglink->hdr.msg_len ) {

#             if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_RECV)
	      if (MPID_DebugFlag) {
		  PTLS_Printf("   Receiving truncated message\n"); 
	      }
#             endif
          
	      rhandle->s.MPI_ERROR       = MPI_ERR_TRUNCATE;
	      rhandle->s.count           = rhandle->len;
	  }
	  else {
	      rhandle->s.count           =
		rhandle->bytes_as_contig = msglink->hdr.msg_len;
	  }

	  rhandle->from                  = msglink->hdr.src_rank;

	  msg_type = msglink->hdr.user_data[0];

	  switch ( msg_type ) {

	  case PTLS_MSG_SHORT_SYNC:

#             if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_RECV)
	      if (MPID_DebugFlag) {
		  PTLS_Printf("   Short synchronous protocol\n");
		  PTLS_Printf("   msg_type = %d\n",msg_type);
	      }
#             endif

	      /* short overflow doesn't ack, so this does */
	      rhandle->send_info.dst_matchbits            = msglink->hdr.ret_mbits;
	      rhandle->send_info.dst_offset               = 
	      rhandle->send_info.reply_offset             = 
	      rhandle->send_info.return_matchbits.ints.i0 = 
	      rhandle->send_info.return_matchbits.ints.i1 = 
	      rhandle->send_info.reply_len                = 0;
	      rhandle->send_info.user_data[0]             = PTLS_SYNC_ACK;
	      rhandle->send_info.acl_index                = 0;
	      rhandle->send_info.return_portal            = PTLS_PORTAL_NO_ACK;
	      rhandle->send_info.send_flag                = 0;

	      if ( send_user_msg2( (CHAR *)NULL,
				   0,
				   rhandle->from,
				   ptls_ack_portal,
				   0,
				   &rhandle->send_info ) ) {
		  rhandle->s.MPI_ERROR = MPI_ERR_INTERN;
		  MPIR_ERROR( rhandle->comm, MPI_ERR_INTERN, "send_user_msg2() failed" );
		  return;
	      }

	  case PTLS_MSG_SHORT:

#             if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_RECV)
	      if (MPID_DebugFlag) {
		  PTLS_Printf("   Short protocol\n");
		  PTLS_Printf("   msg_type = %d\n",msg_type);
	      }
#             endif

	      /* Copy the message (if there is any data) */
	      if ( rhandle->bytes_as_contig > 0 ) {
		  memcpy( (void *)rhandle->buf,
			  (void *)DYN_OSET2PTR(dtype, msglink->data),
			  rhandle->bytes_as_contig );
	      }
	      
	      /* Free up the mle */
	      UNLINK_RECV_MLE( rhandle );
    
	      rhandle->is_complete = 1;

	      /* fill in the status info */
	      PTLS_GET_LRANK( rhandle->s.MPI_SOURCE, msglink->hdr.dst_mbits );
	      PTLS_GET_TAG(   rhandle->s.MPI_TAG,    msglink->hdr.dst_mbits );
	      rhandle->s.count = rhandle->bytes_as_contig;

	      break;

	  case PTLS_MSG_LONG:

#             if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_RECV)
	      if (MPID_DebugFlag) {
		  PTLS_Printf("   Long protocol\n");
	      }
#             endif

	      /* set up the mle to accept reply msg */
	      PTLS_PULL_SET_MATCHBITS( matchbits, ignorebits );
	      rhandle->recv_match_desc->ign_mbits  = ignorebits;
	      rhandle->recv_match_desc->must_mbits = matchbits;
	      /* Change the mem_op not to ack */
	      rhandle->recv_match_desc->mem_op     = IND_CIRC_SV_HDR_BDY;
	      /* Activate it */
	      rhandle->recv_match_desc->ctl_bits   = MCH_OK;

	      /* fill in the send info structure */
	      rhandle->send_info.dst_matchbits            = msglink->hdr.ret_mbits;
	      rhandle->send_info.dst_offset               = 0;
	      rhandle->send_info.reply_offset             = 0;
	      rhandle->send_info.return_matchbits.ints.i0 = matchbits.ints.i0;
	      rhandle->send_info.return_matchbits.ints.i1 = matchbits.ints.i1;
	      rhandle->send_info.reply_len                = rhandle->bytes_as_contig;
	      rhandle->send_info.acl_index                = 0;
	      rhandle->send_info.return_portal            = ptls_recv_portal;
	      rhandle->send_info.send_flag                = 0;

#             if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_RECV)
	      if (MPID_DebugFlag) {
		  PTLS_Printf("PTLS_Pull_long_msg-changed matchbits for pulling\n");
		  PTLS_Printf("   portal         = %d\n",ptls_recv_portal);
		  PTLS_Printf("   ignore bits\n");
		  PTLS_Printf("       shorts.s0  = 0x%x\n",
			      rhandle->recv_match_desc->ign_mbits.shorts.s0);
		  PTLS_Printf("       shorts.s1  = 0x%x\n",
			      rhandle->recv_match_desc->ign_mbits.shorts.s1);
		  PTLS_Printf("       ints.i1    = 0x%x\n",
			      rhandle->recv_match_desc->ign_mbits.ints.i1);
		  PTLS_Printf("   match_bits     =\n");
		  PTLS_Printf("       shorts.s0  = 0x%x\n",
			      rhandle->recv_match_desc->must_mbits.shorts.s0);
		  PTLS_Printf("       shorts.s1  = 0x%x\n",
			      rhandle->recv_match_desc->must_mbits.shorts.s1);
		  PTLS_Printf("       ints.i1    = 0x%x\n",
			      rhandle->recv_match_desc->must_mbits.ints.i1);
		  PTLS_Printf("PTLS_Pull_long_msg-activating memory descriptor\n");
		  PTLS_Printf("PTLS_Pull_long_msg-calling portal send-\n");
		  PTLS_Printf("   buf            = 0x%x\n", NULL );
		  PTLS_Printf("   bytes          = %d\n",0);
		  PTLS_Printf("   offset         = %d\n",0);
		  PTLS_Printf("   dst grp        = %d\n",ptls_gid);
		  PTLS_Printf("   dest           = %d\n",rhandle->from);
		  PTLS_Printf("   portal         = %d\n",ptls_read_portal);
		  PTLS_Printf("   match_bits     =\n");
		  PTLS_Printf("       ints.i0    = 0x%x\n",rhandle->send_info.dst_matchbits.ints.i0);
		  PTLS_Printf("       ints.i1    = 0x%x\n",rhandle->send_info.dst_matchbits.ints.i1);
		  PTLS_Printf("   ret match_bits =\n");
		  PTLS_Printf("       ints.i0    = 0x%x\n",
			      rhandle->send_info.return_matchbits.ints.i0);
		  PTLS_Printf("       ints.i1    = 0x%x\n",
			      rhandle->send_info.return_matchbits.ints.i1);
		  PTLS_Printf("   ret portal     = %d\n",rhandle->send_info.return_portal);
		  PTLS_Printf("   ret len        = %d\n",rhandle->send_info.reply_len);
		  PTLS_Printf("   ret offset     = %d\n",rhandle->send_info.reply_offset);
	      }
#             endif


	      if ( send_user_msg2( (CHAR *)NULL,
				   0,
				   rhandle->from,
				   ptls_read_portal,
				   0,
				   &rhandle->send_info ) ) {
		  rhandle->s.MPI_ERROR = MPI_ERR_INTERN;
		  MPIR_ERROR( rhandle->comm, MPI_ERR_INTERN, "send_user_msg2() failed" );
		  return;
	      }

	      /* fill in the status info */
	      PTLS_GET_LRANK( rhandle->s.MPI_SOURCE, msglink->hdr.dst_mbits );
	      PTLS_GET_TAG(   rhandle->s.MPI_TAG,    msglink->hdr.dst_mbits );
	      rhandle->s.count = rhandle->bytes_as_contig;
	      
	      rhandle->is_complete = 0;
	      rhandle->test        = MPID_PTLS_Test_pulled_recv;
	      rhandle->push        = 0;
	      rhandle->wait        = MPID_PTLS_Wait_pulled_recv;
	      rhandle->cancel      = 0;

	      break;

	  default:
	      MPIR_ERROR( rhandle->comm, MPI_ERR_INTERN, "Unknown message protocol" );
	      return;
	  }
	  
	  sptl_dyn_hp_free( dtype, msglink );

          if ( portal_unlock_buffer( rhandle->buf, rhandle->len ) < 0 ) {
              fprintf(stderr,"%s:%d portal_unlock_buffer() failed\n",__FILE__,__LINE__);
          }

      }
    else {
	/* message was not found, so recv is posted */
	rhandle->is_complete = 0;
	rhandle->test        = MPID_PTLS_Test_recv;
        rhandle->push        = 0;
        rhandle->wait        = MPID_PTLS_Wait_recv;
        rhandle->cancel      = 0;
    }
    
}


