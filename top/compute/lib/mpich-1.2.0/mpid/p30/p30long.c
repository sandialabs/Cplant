#include "mpiddev.h"

/***************************************************************************

  Long protocol send  

  ***************************************************************************/


/***************************************************************************

  MPID_P30_Long_send

  blocking long protocol send

  ***************************************************************************/
int MPID_P30_Long_send( buf, len, src_lrank, tag, context_id, dest,
				    msgrep, comm )
void *buf;
int  len, tag, context_id, src_lrank, dest, msgrep;
struct MPIR_COMMUNICATOR *comm;
{
    MPIR_SHANDLE shandle;
    int          mpi_errno = MPI_SUCCESS;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P30_Printf("entering MPID_P30_Long_send()\n");
    }
#endif

    if ( (mpi_errno = MPID_P30_Long_isend( buf, len, src_lrank, tag, 
					    context_id, dest, msgrep, 
					    &shandle, comm ) ) != MPI_SUCCESS ) {
	return mpi_errno;
    }

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P30_Printf("calling MPID_P30_Long_wait_send()\n");
    }
#endif

    mpi_errno = shandle.wait( &shandle );

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P30_Printf("leaving MPID_P30_Long_send()\n");
    }
#endif

    return mpi_errno;
}


/***************************************************************************

  MPID_P30_Long_isend

  nonblocking long protocol send

  ***************************************************************************/
int MPID_P30_Long_isend( buf, len, src_lrank, tag, context_id, dest,
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
        P30_Printf("MPID_P30_Long_isend\n");
    }
#   endif

    shandle->got_sent    = 0;
    shandle->got_ack     = 0;
    shandle->ack_mlength = -1;

    if ( (shandle->eq_handle_list_ptr = MPID_P30_Get_eq_handle()) == NULL ) {
	return MPI_ERR_INTERN;
    }

    /* set the destination process id */
    dest_id.addr_kind = PTL_ADDR_GID;
    dest_id.gid       = _mpi_p30_my_id.gid;
    dest_id.rid       = dest;

    /* set the read match bits */
    match_bits        = (ptl_match_bits_t)shandle;
#ifdef __i386__
    match_bits       &= 0x00000000ffffffff;
#endif
    
#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_SEND)
    if (MPID_DebugFlag) {
	P30_Printf("  calling PtlMEInsert(\n");
	P30_Printf("    _mpi_p30_read_me_handle = %d\n",_mpi_p30_read_me_handle);
	P30_Dump_process_id( &dest_id );
	P30_Printf("    match_bits              = 0x%lx\n",match_bits);
	P30_Printf("    acl                     = 0\n");
	P30_Printf("    link                    = PTL_RETAIN\n");
	P30_Printf("    insert                  = PTL_INS_AFTER\n");
	P30_Printf("    &shandle->me_handle     = %p )\n",&shandle->me_handle);
    }
#   endif

    /* get an me on the read portal */
    P30_CALL( PtlMEInsert( _mpi_p30_read_me_handle,
			   dest_id,
			   match_bits,
			   0,
			   PTL_RETAIN,
			   PTL_INS_AFTER,
			   &shandle->me_handle ), PtlMEInsert );
    
    /* fill in the md */
    md.start      = buf;
    md.length     = len;
    md.threshold  = 3; /* sent, ack, get */
    md.max_offset = len;
    md.options    = PTL_MD_OP_GET;
    md.user_ptr   = shandle;
    md.eventq     = shandle->eq_handle_list_ptr->eq_handle;
    
#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_SEND)
    if (MPID_DebugFlag) {
	P30_Printf("  calling PtlMDAttach(\n");
	P30_Printf("     shandle->me_handle     = %d\n",shandle->me_handle);
	P30_Dump_md( &md );
	P30_Printf("    link                   = PTL_RETAIN\n");
	P30_Printf("    &shandle->md_handle    = %p )\n",&shandle->md_handle);
    }
#   endif

    /* attach the md to the me */
    P30_CALL( PtlMDAttach( shandle->me_handle,
			   md,
			   PTL_RETAIN,
			   &shandle->md_handle ), PtlMDAttach );

    /* fill in the send match bits */
    P30_SET_LONG_SEND_BITS( match_bits, context_id, src_lrank, tag );

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_SEND)
    if (MPID_DebugFlag) {
	P30_Printf("  calling PtlPut(\n");
	P30_Printf("    shandle->md_handle     = %d\n",shandle->md_handle);
	P30_Printf("    ack                    = PTL_ACK_REQ\n");
	P30_Dump_process_id( &dest_id );
	P30_Printf("    _mpi_p30_recv_portal   = %d\n",_mpi_p30_recv_portal);
	P30_Printf("    acl                    = 0\n");
	P30_Printf("    match_bits             = 0x%lx\n",match_bits);
	P30_Printf("    0ffset                 = 0\n");
	P30_Printf("    hdr_data               = %ld (0x%lx))\n",(unsigned long)shandle,(unsigned long)shandle);
    }
#   endif

    /* send the message */
    P30_CALL( PtlPut( shandle->md_handle,
		      PTL_ACK_REQ,
		      dest_id,
		      _mpi_p30_recv_portal,
		      0,
		      match_bits,
		      0,
		      (unsigned long)shandle ), PtlPut );

    shandle->is_complete     = 0;
    shandle->errval          = MPI_SUCCESS;
    shandle->comm            = comm;
    shandle->start           = buf;
    shandle->bytes_as_contig = len;
    shandle->wait	     = MPID_P30_Long_wait_send;
    shandle->test	     = MPID_P30_Long_test_send;

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
        P30_Printf("leaving MPID_P30_Long_isend()\n");
    }
#   endif

    return MPI_SUCCESS;
}

/***************************************************************************

  MPID_P30_Long_send_cleanup

  ***************************************************************************/
void MPID_P30_Long_send_cleanup( shandle )
MPIR_SHANDLE *shandle;
{

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
        P30_Printf("entering MPID_P30_Long_send_cleanup()\n");
    }
#   endif

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P30_Printf("  calling PtlMEUnlink( shandle->me_handle = %d )\n",shandle->me_handle);
    }
#   endif

    /* free the me handle (which frees up the md too) */
    P30_CALL( PtlMEUnlink( shandle->me_handle ), PtlMEUnlink );

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P30_Printf("  calling MPID_P30_Free_eq_handle( shandle->eq_handle_list_ptr = %p )\n",
		   shandle->eq_handle_list_ptr);
    }
#   endif

    /* add the eq handle back into the list */
    MPID_P30_Free_eq_handle( shandle->eq_handle_list_ptr );

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
        P30_Printf("leaving MPID_P30_Long_send_cleanup()\n");
    }
#   endif

}

/***************************************************************************

  MPID_P30_Long_test_send

  test to see if the kernel has processed the send

  ***************************************************************************/
int MPID_P30_Long_test_send( shandle )
MPIR_SHANDLE *shandle;
{
    int         mpi_errno = MPI_SUCCESS;
    ptl_event_t event;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P30_Printf("entering MPID_P30_Long_test_send()\n");
    }
#endif

    if ( (_mpi_p30_errno = PtlEQGet( shandle->eq_handle_list_ptr->eq_handle, &event )) == PTL_OK ) {

#       if defined(MPID_DEBUG_ALL)
	if ( MPID_DebugFlag ) {
	    P30_Printf("  got event\n");
	    P30_Dump_event( &event );
	}
#       endif

	/* it's possible that the PTL_EVENT_ACK can come in before the PTL_EVENT_SENT, so we need
           to see which one we got and keep track of things */

	if ( event.type == PTL_EVENT_SENT ) {

	    shandle->got_sent = 1;

	} else if ( event.type == PTL_EVENT_ACK ) {

	    shandle->got_ack     = 1;
	    shandle->ack_mlength = event.mlength;

	}

	if ( shandle->got_sent && shandle->got_ack ) {

	    if ( shandle->ack_mlength > 0 ) {
		/* entire message was received */
		shandle->is_complete = 1;
		/* free up the resources */
		MPID_P30_Long_send_cleanup( shandle );
		
	    } else {
		/* message wasn't received - check for read */
		shandle->wait = MPID_P30_Long_wait_send_read;
		shandle->test = MPID_P30_Long_test_send_read;

		mpi_errno = shandle->test( shandle );
	    }

	}

    } else {
	if ( _mpi_p30_errno != PTL_EQ_EMPTY ) {
	    fprintf(stderr,"%d: (%s:%d) PtlEQGet() failed : %s\n",
		    MPID_MyWorldRank, __FILE__, __LINE__, ptl_err_str[_mpi_p30_errno]);
	}
    }

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P30_Printf("leaving MPID_P30_Long_test_send()\n");
    }
#endif

    return mpi_errno;

}

/***************************************************************************

  MPID_P30_Long_test_send_read

  test for reading of the send buffer

  ***************************************************************************/
int MPID_P30_Long_test_send_read( shandle )
MPIR_SHANDLE *shandle;
{

    int mpi_errno = MPI_SUCCESS;
    ptl_event_t event;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P30_Printf("entering MPID_P30_Long_test_send_read()\n");
    }
#endif

    if ( (_mpi_p30_errno = PtlEQGet( shandle->eq_handle_list_ptr->eq_handle, &event )) == PTL_OK ) {

#       if defined(MPID_DEBUG_ALL)
	if ( MPID_DebugFlag ) {
	    P30_Printf("  got event\n");
	    P30_Dump_event( &event );
	}
#       endif

	if ( event.type != PTL_EVENT_GET ) {
	    P30_Printf("ERROR: was expecting a PTL_EVENT_GET\n");
	    P30_Dump_event( &event );
	}
	
	/* we're done */
	shandle->is_complete = 1;
		
	/* free up the resources */
	MPID_P30_Long_send_cleanup( shandle );

    } else {
	if ( _mpi_p30_errno != PTL_EQ_EMPTY ) {
	    fprintf(stderr,"%d: (%s:%d) PtlEQGet() failed : %s\n",
		    MPID_MyWorldRank, __FILE__, __LINE__, ptl_err_str[_mpi_p30_errno]);
	}
    }

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        if ( shandle->is_complete ) {
	    P30_Printf("request is complete\n");
	}
	else {
	    P30_Printf("request is not complete\n");
	}
        P30_Printf("leaving MPID_P30_Long_test_send_read()\n");
    }
#endif

    return mpi_errno;

}



/***************************************************************************

  MPID_P30_Long_wait_send

  wait for completion of long protocol send

  ***************************************************************************/
int MPID_P30_Long_wait_send( shandle )
MPIR_SHANDLE *shandle;
{

    int mpi_errno = MPI_SUCCESS;
    ptl_event_t event1,event2;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P30_Printf("entering MPID_P30_Long_wait_send()\n");
    }
#endif

    if ( shandle->got_sent == 0 ) {

	if ( (_mpi_p30_errno = PtlEQWait( shandle->eq_handle_list_ptr->eq_handle, &event1 )) != PTL_OK ) {
	    fprintf(stderr,"%d: (%s:%d) PtlEQWait() failed : %s\n",
		    MPID_MyWorldRank, __FILE__, __LINE__, ptl_err_str[_mpi_p30_errno]);
	}

#       if defined(MPID_DEBUG_ALL)
	if ( MPID_DebugFlag ) {
	    P30_Printf("  got event1\n");
	    P30_Dump_event( &event1 );
	}
#       endif

	shandle->got_sent = 1;

	if ( event1.type == PTL_EVENT_ACK ) {
	    shandle->ack_mlength = event1.mlength;
	}

    }

    if ( shandle->got_ack == 0 ) {

	if ( (_mpi_p30_errno = PtlEQWait( shandle->eq_handle_list_ptr->eq_handle, &event2 )) != PTL_OK ) {
	    fprintf(stderr,"%d: (%s:%d) PtlEQWait() failed : %s\n",
		    MPID_MyWorldRank, __FILE__, __LINE__, ptl_err_str[_mpi_p30_errno]);
	}

#       if defined(MPID_DEBUG_ALL)
	if ( MPID_DebugFlag ) {
	    P30_Printf("  got event2\n");
	    P30_Dump_event( &event2 );
	}
#       endif

	shandle->got_ack = 1;

	if ( event2.type == PTL_EVENT_ACK ) {
	    shandle->ack_mlength = event2.mlength;
	}
    }

    if ( shandle->ack_mlength > 0 ) {
	/* entire message was received */
	shandle->is_complete = 1;
	/* free up the resources */
	MPID_P30_Long_send_cleanup( shandle );

    } else {
	/* message wasn't received - check for read */
	shandle->wait = MPID_P30_Long_wait_send_read;
	shandle->test = MPID_P30_Long_test_send_read;
	
	mpi_errno = shandle->wait( shandle );
    }

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P30_Printf("leaving MPID_P30_Long_wait_send()\n");
    }
#endif

    return mpi_errno;
}


/***************************************************************************

  MPID_P30_Long_wait_send_read

  wait for the send buffer to be read

  ***************************************************************************/
int MPID_P30_Long_wait_send_read( shandle )
MPIR_SHANDLE *shandle;
{

    int mpi_errno = MPI_SUCCESS;
    ptl_event_t event;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P30_Printf("entering MPID_P30_Long_wait_send_read()\n");
    }
#endif

    if ( (_mpi_p30_errno = PtlEQWait( shandle->eq_handle_list_ptr->eq_handle, &event )) != PTL_OK ) {
	fprintf(stderr,"%d: (%s:%d) PtlEQWait() failed : %s\n",
		MPID_MyWorldRank, __FILE__, __LINE__, ptl_err_str[_mpi_p30_errno]);
    }

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P30_Printf("  got event\n");
	P30_Dump_event( &event );
    }
#   endif

    /* got the ack event */
    if ( event.type != PTL_EVENT_GET ) {
	P30_Printf("ERROR: was expecting a PTL_EVENT_GET\n");
	P30_Dump_event( &event );
    }
	
    /* we're done */
    shandle->is_complete = 1;

    /* free up the resources */
    MPID_P30_Long_send_cleanup( shandle );

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P30_Printf("leaving MPID_P30_Long_wait_send_read()\n");
    }
#endif
    
    return mpi_errno;

}


/***************************************************************************

  MPID_P30_Long_delete

  cleanup

  ***************************************************************************/
void MPID_P30_Long_delete( MPID_Protocol *p )
{

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P30_Printf("entering MPID_P30_Long_delete()\n");
    }
#endif

    FREE( p );

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P30_Printf("leaving MPID_P30_Long_delete()\n");
    }
#endif

}



/***************************************************************************

  MPID_P30_Long_setup

  set up the long protocol structure

  ***************************************************************************/
MPID_Protocol *MPID_P30_Long_setup()
{
    MPID_Protocol *p;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P30_Printf("entering MPID_P30_Long_setup()\n");
    }
#endif

    p = (MPID_Protocol *) MALLOC( sizeof(MPID_Protocol) );
    if (!p) return 0;
    p->send	   = MPID_P30_Long_send;
    p->recv	   = 0;
    p->isend	   = MPID_P30_Long_isend;
    p->ssend	   = MPID_P30_Long_send;
    p->issend	   = MPID_P30_Long_isend;
    p->wait_send   = 0;
    p->push_send   = 0;
    p->cancel_send = 0;
    p->irecv	   = 0;
    p->wait_recv   = 0;
    p->push_recv   = 0;
    p->cancel_recv = 0;
    p->do_ack      = 0;
    p->unex        = 0;
    p->delete      = MPID_P30_Long_delete;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P30_Printf("leaving MPID_P30_Long_setup()\n");
    }
#endif

    return p;
}
