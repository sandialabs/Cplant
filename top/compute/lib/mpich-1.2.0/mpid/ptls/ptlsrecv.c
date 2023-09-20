
#include "ptlsdev.h"

/***************************************************************************

  MPID_PTLS_Test_recv
  
  Check to see if a pre-posted recv request has completed

  ***************************************************************************/
int MPID_PTLS_Test_recv( MPIR_RHANDLE *rhandle )
{
    PTL_MSG_HDR *hdr;
    int          mpi_errno = MPI_SUCCESS;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
	PTLS_Printf("entering PTLS_Test_recv()\n");
    }
#endif

    if ( rhandle->recv_match_desc->ctl_bits & MCH_FAIL_ON_NOFIT ) {

#ifdef MPID_DEBUG_ALL
	if ( MPID_DebugFlag ) {
	    PTLS_Printf("ctl_bits = 0x%2x\n",rhandle->recv_match_desc->ctl_bits);
	    PTLS_Printf("entry failed on no fit\n");
	    PTLS_Printf("unlinking mle and complaining about truncation\n");
	}
#endif

	UNLINK_RECV_MLE( rhandle );

#ifdef MPID_DEBUG_ALL
	if ( MPID_DebugFlag ) {
	    PTLS_Printf("leaving PTLS_Test_recv()\n");
	}
#endif

	mpi_errno = rhandle->s.MPI_ERROR = MPI_ERR_TRUNCATE;

	return mpi_errno;
    }

#ifdef MPID_DEBUG_ALL
	if ( MPID_DebugFlag ) {
	    PTLS_Printf("msg hdr = %p\n",
		&rhandle->recv_match_desc->mem_desc.ind.buf_desc_table[0].hdr);
	}
#endif

    if ( rhandle->recv_match_desc->mem_desc.ind.buf_desc_table[0].hdr.msg_len > 0 ) {

	rhandle->is_complete = 1;

	/* fill in the status info */
	hdr = &rhandle->recv_match_desc->mem_desc.ind.buf_desc_table->hdr;
	PTLS_GET_LRANK( rhandle->s.MPI_SOURCE, hdr->dst_mbits );
	PTLS_GET_TAG(   rhandle->s.MPI_TAG, hdr->dst_mbits );
	rhandle->s.count      = hdr->msg_len;
	    
#ifdef MPID_DEBUG_ALL
	if ( MPID_DebugFlag ) {
	    PTLS_Printf("receive is complete\n");
	    PTLS_Printf("status:\n");
	    PTLS_Printf("  MPI_SOURCE = %d\n",rhandle->s.MPI_SOURCE);
	    PTLS_Printf("  count      = %d\n",rhandle->s.count);
	    PTLS_Printf("  MPI_TAG    = %d\n",rhandle->s.MPI_TAG);
	    PTLS_Printf("calling MPID_PTLS_Unlink_recv_mle()\n");
	    PTLS_Printf("leaving PTLS_Test_recv()\n");
	}
#endif

        if ( portal_unlock_buffer( rhandle->buf, rhandle->len ) < 0 ) {
            fprintf(stderr,"%s:%d portal_unlock_buffer() failed\n",__FILE__,__LINE__);
        }

	/* unlink the mle */
	UNLINK_RECV_MLE( rhandle );
    
#ifdef MPID_DEBUG_ALL
	if ( MPID_DebugFlag ) {
	    PTLS_Printf("leaving PTLS_Test_recv()\n");
	}
#endif
	return mpi_errno;
    }

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
	PTLS_Printf("receive is not complete\n");
	PTLS_Printf("leaving PTLS_Test_recv()\n");
    }
#endif

    return mpi_errno;
    
}


/***************************************************************************

  MPID_PTLS_Wait_recv

  Wait for a pre-posted recv request to be completed

  ***************************************************************************/
int MPID_PTLS_Wait_recv( MPIR_RHANDLE *rhandle )
{

    PTL_MSG_HDR *hdr;
    int          mpi_errno = MPI_SUCCESS;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
	PTLS_Printf("entering PTLS_Wait_recv()\n");
	PTLS_Printf("recv_match_desc->matchbits = 0x%x 0x%x\n",
		    rhandle->recv_match_desc->must_mbits.ints.i0,
		    rhandle->recv_match_desc->must_mbits.ints.i1 );
    }
#endif

#ifdef MPID_DEBUG_ALL
        if ( MPID_DebugFlag ) {
            PTLS_Printf("msg hdr = %p\n",
                &rhandle->recv_match_desc->mem_desc.ind.buf_desc_table[0].hdr);
        }
#endif

    while ( ((volatile)rhandle->recv_match_desc->mem_desc.ind.buf_desc_table[0].hdr.msg_len<0)
	   &&
	   ( ! (rhandle->recv_match_desc->ctl_bits & MCH_FAIL_ON_NOFIT) ) ) {
#ifdef SCHED_YIELD
       sched_yield();
#endif
    }

    if ( rhandle->recv_match_desc->mem_desc.ind.buf_desc_table[0].hdr.msg_len >= 0 ) {

	rhandle->is_complete = 1;

	/* fill in the status info */
	hdr = &rhandle->recv_match_desc->mem_desc.ind.buf_desc_table->hdr;
	PTLS_GET_LRANK( rhandle->s.MPI_SOURCE, hdr->dst_mbits );
	PTLS_GET_TAG(   rhandle->s.MPI_TAG, hdr->dst_mbits );
	rhandle->s.count      = hdr->msg_len;
	    
#ifdef MPID_DEBUG_ALL
	if ( MPID_DebugFlag ) {
	    PTLS_Printf("receive is complete\n");
	    PTLS_Printf("status:\n");
	    PTLS_Printf("  MPI_SOURCE = %d\n",rhandle->s.MPI_SOURCE);
	    PTLS_Printf("  count      = %d\n",rhandle->s.count);
	    PTLS_Printf("  MPI_TAG    = %d\n",rhandle->s.MPI_TAG);
	    PTLS_Printf("leaving PTLS_Wait_recv()\n");
	    PTLS_Printf("calling MPID_PTLS_Unlink_recv_mle()\n");
	}
#endif

    }
    else {

#ifdef MPID_DEBUG_ALL
	if ( MPID_DebugFlag ) {
	    PTLS_Printf("ctl_bits = 0x%2x\n",rhandle->recv_match_desc->ctl_bits);
	    PTLS_Printf("entry failed on no fit\n");
	    PTLS_Printf("unlinking mle and complaining about truncation\n");
	}
#endif

	mpi_errno = rhandle->s.MPI_ERROR = MPI_ERR_TRUNCATE;
    }

    if ( portal_unlock_buffer( rhandle->buf, rhandle->len ) < 0 ) {
        fprintf(stderr,"%s:%d portal_unlock_buffer() failed\n",__FILE__,__LINE__
);
    }

    UNLINK_RECV_MLE( rhandle );

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
	PTLS_Printf("leaving PTLS_Wait_recv()\n");
    }
#endif

    
    return mpi_errno;

}

/***************************************************************************

  MPID_PTLS_Test_pulled_recv
  
  Check to see if a pulled recv request has completed

  ***************************************************************************/
int MPID_PTLS_Test_pulled_recv( MPIR_RHANDLE *rhandle )
{
    int mpi_errno = MPI_SUCCESS;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
	PTLS_Printf("entering PTLS_Test_pulled_recv()\n");
    }
#endif

    if ( rhandle->recv_match_desc->ctl_bits & MCH_FAIL_ON_NOFIT ) {

#ifdef MPID_DEBUG_ALL
	if ( MPID_DebugFlag ) {
	    PTLS_Printf("ctl_bits = 0x%2x\n",rhandle->recv_match_desc->ctl_bits);
	    PTLS_Printf("entry failed on no fit\n");
	    PTLS_Printf("unlinking mle and complaining about truncation\n");
	}
#endif

	UNLINK_RECV_MLE( rhandle );

#ifdef MPID_DEBUG_ALL
	if ( MPID_DebugFlag ) {
	    PTLS_Printf("leaving PTLS_Test_pulled_recv()\n");
	}
#endif

	return rhandle->s.MPI_ERROR = MPI_ERR_TRUNCATE;

    }

#ifdef MPID_DEBUG_ALL
        if ( MPID_DebugFlag ) {
            PTLS_Printf("msg hdr = %p\n",
                &rhandle->recv_match_desc->mem_desc.ind.buf_desc_table[0].hdr);
        }
#endif

    if ( rhandle->recv_match_desc->mem_desc.ind.buf_desc_table[0].hdr.msg_len ==
	 rhandle->len ) {

	rhandle->is_complete = 1;

	/* already filled in the status info */
	    
#ifdef MPID_DEBUG_ALL
	if ( MPID_DebugFlag ) {
	    PTLS_Printf("receive is complete\n");
	    PTLS_Printf("status:\n");
	    PTLS_Printf("  MPI_SOURCE = %d\n",rhandle->s.MPI_SOURCE);
	    PTLS_Printf("  count      = %d\n",rhandle->s.count);
	    PTLS_Printf("  MPI_TAG    = %d\n",rhandle->s.MPI_TAG);
	    PTLS_Printf("calling MPID_PTLS_Unlink_recv_mle()\n");
	}
#endif

        if ( portal_unlock_buffer( rhandle->buf, rhandle->len ) < 0 ) {
            fprintf(stderr,"%s:%d portal_unlock_buffer() failed\n",__FILE__,__LINE__);
        }

	/* unlink the mle */
        UNLINK_RECV_MLE( rhandle );
    
#ifdef MPID_DEBUG_ALL
	if ( MPID_DebugFlag ) {
	    PTLS_Printf("leaving PTLS_Test_pulled_recv()\n");
	}
#endif

	return mpi_errno;

    }

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
	PTLS_Printf("receive is not complete\n");
	PTLS_Printf("leaving PTLS_Test_pulled_recv()\n");
    }
#endif

    return mpi_errno;
    
}


/***************************************************************************

  MPID_PTLS_Wait_pulled_recv

  Wait for a pulled recv request to be completed

  ***************************************************************************/
int MPID_PTLS_Wait_pulled_recv( MPIR_RHANDLE *rhandle )
{
    int mpi_errno = MPI_SUCCESS;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
	PTLS_Printf("entering PTLS_Wait_pulled_recv()\n");
	PTLS_Printf("msg hdr = %p\n",
	    &rhandle->recv_match_desc->mem_desc.ind.buf_desc_table[0].hdr );
    }
#endif

    while ( ((volatile)rhandle->recv_match_desc->mem_desc.ind.buf_desc_table[0].hdr.msg_len<0)
	   &&
	   ( ! (rhandle->recv_match_desc->ctl_bits & MCH_FAIL_ON_NOFIT) ) ) {
#ifdef SCHED_YIELD
        sched_yield();
#endif
    }

    if ( rhandle->recv_match_desc->mem_desc.ind.buf_desc_table[0].hdr.msg_len >= 0 ) {

	rhandle->is_complete = 1;
	
	/* already filled in the status info */

#ifdef MPID_DEBUG_ALL
	if ( MPID_DebugFlag ) {
	    PTLS_Printf("receive is complete\n");
	    PTLS_Printf("status:\n");
	    PTLS_Printf("  MPI_SOURCE = %d\n",rhandle->s.MPI_SOURCE);
	    PTLS_Printf("  count      = %d\n",rhandle->s.count);
	    PTLS_Printf("  MPI_TAG    = %d\n",rhandle->s.MPI_TAG);
	    PTLS_Printf("leaving PTLS_Wait_recv()\n");
	    PTLS_Printf("calling MPID_PTLS_Unlink_recv_mle()\n");
	}
#endif
    }
    else {

#ifdef MPID_DEBUG_ALL
	if ( MPID_DebugFlag ) {
	    PTLS_Printf("ctl_bits = 0x%2x\n",rhandle->recv_match_desc->ctl_bits);
	    PTLS_Printf("entry failed on no fit\n");
	    PTLS_Printf("unlinking mle and complaining about truncation\n");
	}
#endif
	
	mpi_errno = rhandle->s.MPI_ERROR = MPI_ERR_TRUNCATE;
    }

    if ( portal_unlock_buffer( rhandle->buf, rhandle->len ) < 0 ) {
        fprintf(stderr,"%s:%d portal_unlock_buffer() failed\n",__FILE__,__LINE__
);
    }

    UNLINK_RECV_MLE( rhandle );

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
	PTLS_Printf("leaving PTLS_Wait_pulled_recv()\n");
    }
#endif

    return mpi_errno;

}
