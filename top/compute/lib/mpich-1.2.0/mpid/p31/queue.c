
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
    mpi_unex_list_t  *ptr;
    ptl_match_bits_t  match_bits;
    ptl_match_bits_t  ignore_bits;
    ptl_event_t       event;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
	P31_Printf("entering P31_Search_unexpected_queue()\n");
    }
#endif
    
    *flag = 0;

    status->MPI_ERROR = MPI_SUCCESS;

    P31_SET_RECV_BITS( match_bits, ignore_bits, context_id, src, tag );

    /* check the list of unexpected message */
    if ( (ptr = _mpi_p31_unex_list_head) != NULL ) {
	do {
	    if ( (ptr->event.match_bits & ~ignore_bits) == (match_bits & ~ignore_bits) ) {
		*flag              = 1;
		status->count      = ptr->event.rlength;
		P31_GET_SOURCE( ptr->event.match_bits, status->MPI_SOURCE );
		P31_GET_TAG( ptr->event.match_bits, status->MPI_TAG );
		return MPI_SUCCESS;
	    }
	    ptr = ptr->next;
	} while ( ptr );

    }

    /* check to see if anything else has come in */
    if ( (_mpi_p31_errno = PtlEQGet( _mpi_p31_unex_eq_handle, &event )) == PTL_OK ) {

	if ( event.type != PTL_EVENT_PUT ) {
	    P31_Printf("ERROR: was expecting a PTL_EVENT_PUT\n");
	    P31_Dump_event( &event );
	}

	/* get a free unexpected message entry */
	if ( (ptr = MPID_P31_Get_unex()) == NULL ) {
	    return MPI_ERR_INTERN;
	}
	ptr->event = event;

	if ( (ptr->event.match_bits & ~ignore_bits) == (match_bits & ~ignore_bits) ) {
	    *flag              = 1;
	    status->count      = ptr->event.rlength;
	    P31_GET_SOURCE( ptr->event.match_bits, status->MPI_SOURCE );
	    P31_GET_TAG( ptr->event.match_bits, status->MPI_TAG );
	}
    } else {
	if ( _mpi_p31_errno == PTL_EQ_DROPPED ) {
	    fprintf(stderr,"%d: (%s:%d) PtlEQGet() failed : PTL_EQ_DROPPED\n",
		    MPID_MyWorldRank, __FILE__, __LINE__ );
	    MPID_P31_Abort( (MPI_Comm)0, MPI_ERR_INTERN, "Dropped events - Too many unexpected messages!" );
	} else if ( _mpi_p31_errno != PTL_EQ_EMPTY ) {
	    fprintf(stderr,"%d: (%s:%d) PtlEQGet() failed : %s\n",
		    MPID_MyWorldRank, __FILE__, __LINE__, ptl_err_str[_mpi_p31_errno]);
	    MPID_P31_Abort( (MPI_Comm)0, MPI_ERR_INTERN, NULL );
	}
    }

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
	P31_Printf("leaving P31_Search_unexpected_queue()\n");
    }
#endif

    return MPI_SUCCESS;
}

/***************************************************************************

  MPID_P31_Process_unex

  Process an unexpected message

  ***************************************************************************/
void MPID_P31_Process_unex( int src_lrank, int tag, int context_id,
			   MPIR_RHANDLE *rhandle, ptl_event_t *event )
{
    ptl_process_id_t src,process_id;
    ptl_match_bits_t match_bits,ignore_bits;
    ptl_md_t         md;
    long             unex_block;
#ifndef __linux__
    unsigned long phell;
#endif

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P31_Printf("entering MPID_P31_Process_unex(\n");
	P31_Printf("  src_lrank  = %d\n",src_lrank);
	P31_Printf("  tag        = %d\n",tag);
	P31_Printf("  context_id = %d\n",context_id);
	P31_Printf("  rhandle    = %p\n",rhandle);
	P31_Printf("  event      = %p\n",event);
    }
#   endif

    /* fill in the source rank and tag */
    P31_GET_TAG( event->match_bits, rhandle->s.MPI_TAG );
    P31_GET_SOURCE( event->match_bits, rhandle->s.MPI_SOURCE );
    rhandle->from         = event->initiator.rid;

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P31_Printf("  rhandle->s.MPI_TAG    = %d\n",rhandle->s.MPI_TAG);
	P31_Printf("  rhandle->s.MPI_SOURCE = %d\n",rhandle->s.MPI_SOURCE);
	P31_Printf("  rhandle->from         = %d\n",rhandle->from);
	P31_Printf("  event->mlength        = %d\n",event->mlength);
	P31_Printf("  event->rlength        = %d\n",event->rlength);
	P31_Printf("  event->match_bits     = 0x%lx\n",event->match_bits);
    }
#   endif


    if ( P31_IS_LONG( event->match_bits ) ) {

#       if defined(MPID_DEBUG_ALL)
	if ( MPID_DebugFlag ) {
	    P31_Printf("  long protocol message\n");
	}
#       endif

	/* check for truncation */
	if ( event->rlength > rhandle->len ) {
	    rhandle->s.MPI_ERROR = MPI_ERR_TRUNCATE;
	    rhandle->s.count     = rhandle->len;

#           if defined(MPID_DEBUG_ALL)
	    if ( MPID_DebugFlag ) {
		P31_Printf("  MESSAGE TRUNCATED\n");
		P31_Printf("  rhandle->len = %d\n",rhandle->len);
	    }
#           endif

	} else {
	    rhandle->s.count             =
		rhandle->bytes_as_contig = event->rlength;
	}

	/* long protocol */
	if ( (rhandle->eq_handle_list_ptr = MPID_P31_Get_eq_handle()) == NULL ) {
	    rhandle->s.MPI_ERROR = MPI_ERR_INTERN;
	    return;
	}

	/* set the src address */
	src.addr_kind = PTL_ADDR_GID;
	src.gid       = _mpi_p31_my_id.gid;
	src.rid       = rhandle->from;

	/* create an md for the pull */
	md.start        = rhandle->buf;
	md.length       = event->rlength;
	md.threshold    = 2;
	md.max_offset   = md.length;
	md.options      = 0;
	md.user_ptr     = NULL;
	md.eventq       = rhandle->eq_handle_list_ptr->eq_handle;

#       if defined(MPID_DEBUG_ALL)
	if ( MPID_DebugFlag ) {
	    P31_Printf("  calling PtlMDBind(\n");
	    P31_Printf("    _mpi_p31_ni_handle  = %d\n",_mpi_p31_ni_handle);
	    P31_Dump_md( &md );
	    P31_Printf("    &rhandle->md_handle = %p\n",&rhandle->md_handle);
	}
#       endif

	/* create a free-floating md */
	P31_CALL( PtlMDBind( _mpi_p31_ni_handle,
			     md,
			     &rhandle->md_handle ), PtlMDBind );

#       if defined(MPID_DEBUG_ALL)
	if ( MPID_DebugFlag ) {
	    P31_Printf("  calling PtlGet(\n");
	    P31_Printf("    rhandle->md_handle   = %d\n",rhandle->md_handle);
	    P31_Printf("    src                  =>\n");
	    P31_Printf("      addr_kind          = %d\n",src.addr_kind);
	    P31_Printf("      gid                = %d\n",src.gid);
	    P31_Printf("      rid                = %d\n",src.rid);
	    P31_Printf("    _mpi_p31_read_portal = %d\n",_mpi_p31_read_portal);
	    P31_Printf("    acl                  = 0\n");
	    P31_Printf("    match bits           = 0x%lx\n",event->hdr_data);
	    P31_Printf("    offset               = 0\n");
	}
#       endif

	/* get the message */
	P31_CALL( PtlGet( rhandle->md_handle,
			  src,
			  _mpi_p31_read_portal,
			  0,
			  event->hdr_data,
			  0 ), PtlGet );

	rhandle->got_sent    = 0;
	rhandle->got_reply   = 0;

	rhandle->is_complete = 0;
	rhandle->test        = MPID_P31_Test_pulled;
	rhandle->push        = 0;
	rhandle->wait        = MPID_P31_Wait_pulled;
	rhandle->cancel      = 0;

#       if defined(MPID_DEBUG_ALL)
	if ( MPID_DebugFlag ) {
	    P31_Printf("leaving MPID_P31_Process_unex()\n");
	}
#       endif

	return;
	
    }

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P31_Printf("  short protocol message\n");
	P31_Printf("  event->mlength = %d\n",event->mlength);
	P31_Printf("  event->rlength = %d\n",event->rlength);
    }
#   endif

    /* check for truncation */
    if ( event->mlength > rhandle->len ) {
	rhandle->s.MPI_ERROR = MPI_ERR_TRUNCATE;
	rhandle->s.count     = 
	    rhandle->bytes_as_contig = rhandle->len;

#       if defined(MPID_DEBUG_ALL)
	if ( MPID_DebugFlag ) {
	    P31_Printf("  MESSAGE TRUNCATED\n");
	    P31_Printf("  rhandle->len = %d\n",rhandle->len);
	}
#       endif

    } else {
	rhandle->s.count             =
	    rhandle->bytes_as_contig = event->mlength;
    }


    /* short or short synchronous protocol */
    /* copy the message (if any) */
    if ( rhandle->bytes_as_contig > 0 ) {
#       if defined(MPID_DEBUG_ALL)
	if ( MPID_DebugFlag ) {
	    P31_Printf("  calling memcpy(\n");
	    P31_Printf("    target = %p\n",rhandle->buf);
#ifdef __linux__
	    P31_Printf("    source = %p\n",event->md.start + event->offset);
#else
            phell = (unsigned long) event->md.start; 
            phell += event->offset;
	    P31_Printf("    source = %p\n",(void*)phell);
#endif
	    P31_Printf("    bytes  = %d\n",rhandle->bytes_as_contig);
	}
#       endif

#ifdef __linux__
        memcpy( rhandle->buf, 
                event->md.start + event->offset,
                rhandle->bytes_as_contig );
#else
        phell = (unsigned long) event->md.start;
        phell += event->offset;
	memcpy( rhandle->buf, (void*) phell,
		rhandle->bytes_as_contig );
#endif
    }

    /* send an ack back on sync protocol */
    if ( P31_IS_SYNC(event) ) {
	/* synchronous protocol */

	src.addr_kind = PTL_ADDR_GID;
	src.gid       = _mpi_p31_my_id.gid;
	src.rid       = rhandle->from;

#       if defined(MPID_DEBUG_ALL)
	if ( MPID_DebugFlag ) {
	    P31_Printf("  synchronous protocol\n");
	    P31_Printf("  calling PtlPut(\n");
	    P31_Printf("    md_handle             = %d\n",_mpi_p31_ack_request_md_handle);
	    P31_Printf("    ack                   = PTL_NOACK_REQ\n");
	    P31_Printf("    src                   =>\n");
	    P31_Printf("      addr_kind           = %d\n",src.addr_kind);
	    P31_Printf("      gid                 = %d\n",src.gid);
	    P31_Printf("      rid                 = %d\n",src.rid);
	    P31_Printf("    _mpi_p31_ack_portal   = %d\n",_mpi_p31_ack_portal);
	    P31_Printf("    acl                   = 0\n");
	    P31_Printf("    event->match_bits     = 0x%lx\n",event->hdr_data);
	    P31_Printf("    offset                = 0\n");
	    P31_Printf("    hdr_data              = 0 )\n");
	}
#       endif

	/* send an ack back */
	P31_CALL( PtlPut( _mpi_p31_ack_request_md_handle,
			  PTL_NOACK_REQ,
			  src,
			  _mpi_p31_ack_portal,
			  0,
			  (ptl_match_bits_t)event->hdr_data,
			  0,
			  0 ), PtlPut );
    }

    /* short protocol is done */
    rhandle->is_complete = 1;

    /* which unexpected block did this come from? */
    unex_block = (long)event->md.user_ptr;

    /* update the number of bytes copied out of this unex block */
    _mpi_p31_short_unex_block[unex_block].bytes_copied += event->mlength;

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P31_Printf("  Unexpected message is from block %ld\n",unex_block);
	P31_Printf("  bytes copied so far = %d\n",_mpi_p31_short_unex_block[unex_block].bytes_copied);
	P31_Printf("  max_offset          = %d\n",event->md.max_offset);
    }
#   endif

    /* if the number of bytes copied out of this block is greater than the max_offset
     * of the memory descriptor, then the block has been emptied and can be put back
     * onto the list
     */
    if ( _mpi_p31_short_unex_block[unex_block].bytes_copied > event->md.max_offset ) {

#       if defined(MPID_DEBUG_ALL)
	if ( MPID_DebugFlag ) {
	    P31_Printf("  Block has been emptied - reinserting it in list\n");
	}
#       endif

	/* insert the me */
	process_id.addr_kind = PTL_ADDR_GID;
	process_id.gid       = _mpi_p31_my_id.gid;
	process_id.rid       = PTL_ID_ANY;

	P31_SET_SHORT_UNEX_BITS( match_bits, ignore_bits );

	P31_CALL( PtlMEInsert( _mpi_p31_long_me_handle,
			       process_id,
			       match_bits,
			       ignore_bits,
			       PTL_UNLINK,
			       PTL_INS_BEFORE,
			       &_mpi_p31_short_unex_block[unex_block].me_handle ), PtlMEInsert );

	/* attach the md */
	md.start      = _mpi_p31_short_unex_block[unex_block].start;
	md.length     = _mpi_unex_block_size;
	md.threshold  = PTL_MD_THRESH_INF;
	md.max_offset = _mpi_unex_block_size - _mpi_p31_short_size;
	md.options    = PTL_MD_OP_PUT | PTL_MD_ACK_DISABLE;
	md.user_ptr   = (void *)unex_block; 
	md.eventq     = _mpi_p31_unex_eq_handle;

	/* attach the new md to the match entry */
	P31_CALL( PtlMDAttach( _mpi_p31_short_unex_block[unex_block].me_handle,
			       md,
			       PTL_UNLINK,
			       &_mpi_p31_short_unex_block[unex_block].md_handle ), PtlMDAttach );

	_mpi_p31_short_unex_block[unex_block].bytes_copied = 0;
    }

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P31_Printf("leaving MPID_P31_Process_unex()\n");
    }
#   endif

}


/***************************************************************************

  MPID_Search_unexpected_queue_and_post

  Look through the dynamic heap for an unexpected message that matches
  and post a receive if one is not found

  ***************************************************************************/
void MPID_Search_unexpected_queue_and_post(int src_lrank, int tag, int context_id,
					   MPIR_RHANDLE *rhandle )
{
    ptl_md_t          md;
    ptl_md_t          new_md;
    ptl_event_t       event;
    mpi_unex_list_t  *ptr;
    ptl_match_bits_t  match_bits;
    ptl_match_bits_t  ignore_bits;
    ptl_process_id_t  src_id;
    ptl_sr_value_t    drop_count;

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_RECV)
    if (MPID_DebugFlag) {
	P31_Printf("entering MPID_Search_unexpected_queue_and_post\n");
    }
#   endif

    P31_CALL( PtlNIStatus( _mpi_p31_ni_handle, PTL_SR_DROP_COUNT, &drop_count ), PtlNIStatus );
    if ( drop_count > _mpi_p31_drop_count ) {
	MPID_P31_Abort( (MPI_Comm)0, MPI_ERR_INTERN, "Drop count - Too many unexpected messages!" );
    }

    rhandle->s.MPI_ERROR  = MPI_SUCCESS;
    rhandle->s.MPI_TAG    = tag;
    rhandle->s.MPI_SOURCE = src_lrank;

    P31_SET_RECV_BITS( match_bits, ignore_bits, context_id, src_lrank, tag );

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P31_Printf("  src_lrank   = %d\n",src_lrank);
	P31_Printf("  tag         = %d\n",tag);
	P31_Printf("  context_id  = %d\n",context_id);
	P31_Printf("  rhandle     = %p\n",rhandle);
	P31_Printf("  match_bits  = 0x%lx\n",match_bits);
	P31_Printf("  ignore_bits = 0x%lx\n",ignore_bits);
	P31_Printf("  checking unexpected queue\n");
    }
#   endif

    /* check the list of unexpected message */
    if ( (ptr = _mpi_p31_unex_list_head) ) {
	do {
	    
	    if ( (ptr->event.match_bits & ~ignore_bits) == (match_bits & ~ignore_bits) ) {
#               if defined(MPID_DEBUG_ALL)
		if ( MPID_DebugFlag ) {
		    P31_Printf("  found matching msg in unexpected queue\n");
		    P31_Printf("  calling MPID_P31_Process_unex(\n");
		    P31_Printf("    src_lrank  = %d\n",src_lrank);
		    P31_Printf("    tag        = %d\n",tag);
		    P31_Printf("    context_id = %d\n",context_id);
		    P31_Printf("    rhandle    = %p\n",rhandle);
		    P31_Printf("    ptr        = %p\n",ptr);
		}
#               endif

		/* found it */
		MPID_P31_Process_unex( src_lrank, tag, context_id, rhandle, &ptr->event );

		/* free the unexpected ptr */
		MPID_P31_Free_unex( ptr );

		return;
	    }

	    ptr = ptr->next;

	} while ( ptr );
    }

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P31_Printf("  checking event queue\n");
    }
#   endif

    /* check to see if anything else has come in */
    while ( 1 ) {

	if ( (_mpi_p31_errno = PtlEQGet( _mpi_p31_unex_eq_handle, &event )) == PTL_OK ) {

	    if ( event.type != PTL_EVENT_PUT ) {
		P31_Printf("ERROR: was expecting a PTL_EVENT_PUT\n");
		P31_Dump_event( &event );
	    }

#           if defined(MPID_DEBUG_ALL)
	    if ( MPID_DebugFlag ) {
		P31_Printf("  found an event\n");
		P31_Dump_event( &event );
	    }
#           endif

	    if ( (event.match_bits & ~ignore_bits) == (match_bits & ~ignore_bits) ) {

#               if defined(MPID_DEBUG_ALL)
		if ( MPID_DebugFlag ) {
		    P31_Printf("  found matching msg in event queue\n");
		    P31_Printf("  calling MPID_P31_Process_unex(\n");
		    P31_Printf("    src_lrank  = %d\n",src_lrank);
		    P31_Printf("    tag        = %d\n",tag);
		    P31_Printf("    context_id = %d\n",context_id);
		    P31_Printf("    rhandle    = %p\n",rhandle);
		    P31_Printf("    &event     = %p\n",&event);
		}
#               endif

		/* found it */
		MPID_P31_Process_unex( src_lrank, tag, context_id, rhandle, &event );

		return;

	    } else {
		/* get a free unexpected message entry */
		if ( (ptr = MPID_P31_Get_unex()) == NULL ) {
		    rhandle->s.MPI_ERROR =  MPI_ERR_INTERN;
		    return;
		}
		ptr->event = event;
	    }
	}
	else {
	    if ( _mpi_p31_errno == PTL_EQ_DROPPED ) {
		fprintf(stderr,"%d: (%s:%d) PtlEQGet() failed : PTL_EQ_DROPPED\n",
			MPID_MyWorldRank, __FILE__, __LINE__ );
		MPID_P31_Abort( (MPI_Comm)0, MPI_ERR_INTERN, "Dropped events - Too many unexpected messages!" );
	    } else if ( _mpi_p31_errno != PTL_EQ_EMPTY ) {
		fprintf(stderr,"%d: (%s:%d) PtlEQGet() failed : %s\n",
			MPID_MyWorldRank, __FILE__, __LINE__, ptl_err_str[_mpi_p31_errno]);
		MPID_P31_Abort( (MPI_Comm)0, MPI_ERR_INTERN, NULL );
	    } else {
		break; /* nothing there */
	    }
	}
    }

    /* didn't find it, so prepare to post a receive */
    if ( (rhandle->eq_handle_list_ptr = MPID_P31_Get_eq_handle()) == NULL ) {
	rhandle->s.MPI_ERROR =  MPI_ERR_INTERN;
	return;
    }

    /* set the src process id */
    src_id.addr_kind = PTL_ADDR_GID;
    src_id.gid       = _mpi_p31_my_id.gid;
    src_id.rid       = PTL_ID_ANY; /* source is in the match bits */

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P31_Printf("  didn't find message\n");
	P31_Printf("  calling PtlMEInsert(\n");
	P31_Printf("    _mpi_p31_no_match_me_handle = %d\n",_mpi_p31_no_match_me_handle);
	P31_Printf("    src_id                      =>\n");
	P31_Printf("      addr_kind                 = %d\n",src_id.addr_kind);
	P31_Printf("      gid                       = %d\n",src_id.gid);
	P31_Printf("      rid                       = PTL_ID_ANY\n");
	P31_Printf("    match_bits                  = 0x%lx\n",match_bits);
	P31_Printf("    ignore_bits                 = 0x%lx\n",ignore_bits);
	P31_Printf("    unlink                      = PTL_UNLINK\n");
	P31_Printf("    insert                      = PTL_INS_BEFORE\n");
	P31_Printf("    &rhandle->me_handle         = %p )\n",&rhandle->me_handle);
    }
#   endif

    /* get an me on the recv portal */
    P31_CALL( PtlMEInsert( _mpi_p31_no_match_me_handle,
			   src_id,
			   match_bits,
			   ignore_bits,
			   PTL_UNLINK,
			   PTL_INS_BEFORE,
			   &rhandle->me_handle ), PtlMEInsert );
    
    md.start      = rhandle->buf;
    md.length     = rhandle->len;
    md.threshold  = 0; /* turns it off */
    md.max_offset = md.length;
    md.options    = PTL_MD_OP_PUT | PTL_MD_TRUNCATE;
    md.user_ptr   = rhandle;
    md.eventq     = rhandle->eq_handle_list_ptr->eq_handle;

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P31_Printf("  calling PtlMDAttach(\n");
	P31_Printf("    rhandle->me_handle   = %d\n",rhandle->me_handle);
	P31_Printf("    md                   =>\n");
	P31_Printf("      start              = %p\n",md.start);
	P31_Printf("      length             = %d\n",md.length);
	P31_Printf("      threshold          = %d\n",md.threshold);
	P31_Printf("      max_offset         = %d\n",md.max_offset);
	P31_Printf("      options            = %d\n",md.options);
	P31_Printf("      user_ptr           = %p\n",md.user_ptr);
	P31_Printf("      eventq             = %d\n",md.eventq);
	P31_Printf("    link                 = PTL_UNLINK\n");
	P31_Printf("    &rhandle->md_handle  = %p )\n",&rhandle->md_handle);
    }
#   endif
    
    P31_CALL( PtlMDAttach( rhandle->me_handle,
			   md,
			   PTL_UNLINK,
			   &rhandle->md_handle ), PtlMDAttach );


    /* call the magic update function */
    while ( 1 ) {

	/* fill in new_md */
	new_md.start      = md.start;
	new_md.length     = md.length;
	new_md.threshold  = 1; /* turns it on */
	new_md.max_offset = new_md.length;
	new_md.options    = md.options;
	new_md.user_ptr   = md.user_ptr;
	new_md.eventq     = md.eventq;

#       if defined(MPID_DEBUG_ALL)
	if ( MPID_DebugFlag ) {
	    P31_Printf("  calling PtlMDUpdate(\n");
	    P31_Printf("    rhandle->md_handle   = %d\n",rhandle->md_handle);
	    P31_Printf("    old md               =>\n");
	    P31_Printf("      start              = %p\n",md.start);
	    P31_Printf("      length             = %d\n",md.length);
	    P31_Printf("      threshold          = %d\n",md.threshold);
	    P31_Printf("      max_offset         = %d\n",md.max_offset);
	    P31_Printf("      options            = %d\n",md.options);
	    P31_Printf("      user_ptr           = %p\n",md.user_ptr);
	    P31_Printf("      eventq             = %d\n",md.eventq);
	    P31_Printf("    new md               =>\n");
	    P31_Printf("      start              = %p\n",new_md.start);
	    P31_Printf("      length             = %d\n",new_md.length);
	    P31_Printf("      threshold          = %d\n",new_md.threshold);
	    P31_Printf("      max_offset         = %d\n",new_md.max_offset);
	    P31_Printf("      options            = %d\n",new_md.options);
	    P31_Printf("      user_ptr           = %p\n",new_md.user_ptr);
	    P31_Printf("      eventq             = %d\n",new_md.eventq);
	    P31_Printf("    test eventq          = %d )\n",_mpi_p31_unex_eq_handle);
	}
#       endif

	_mpi_p31_errno = PtlMDUpdate( rhandle->md_handle,
				      &md,
				      &new_md,
				      _mpi_p31_unex_eq_handle );

	if ( _mpi_p31_errno == PTL_OK ) {
	    /* receive was posted */

#           if defined(MPID_DEBUG_ALL)
	    if ( MPID_DebugFlag ) {
		P31_Printf("  receive was posted\n");
	    }
#           endif

	    rhandle->is_complete = 0;
	    rhandle->test        = MPID_P31_Test_recv;
	    rhandle->push        = 0;
	    rhandle->wait        = MPID_P31_Wait_recv;
	    rhandle->cancel      = 0;

	    break;
	    
	} 
	else if ( _mpi_p31_errno == PTL_NOUPDATE ) {

#           if defined(MPID_DEBUG_ALL)
	    if ( MPID_DebugFlag ) {
		P31_Printf("  PtlMDUpdate() failed because event queue wasn't empty\n");
	    }
#           endif

	    /* wait for the event to come in */
	    if ( (_mpi_p31_errno = PtlEQWait( _mpi_p31_unex_eq_handle, &event )) == PTL_OK ) {

		if ( event.type != PTL_EVENT_PUT ) {
		    P31_Printf("ERROR: was expecting a PTL_EVENT_PUT\n");
		    P31_Dump_event( &event );
		}
#               if defined(MPID_DEBUG_ALL)
		if ( MPID_DebugFlag ) {
		    P31_Printf("  found an event\n");
		    P31_Dump_event( &event );
		}
#               endif

		if ( (event.match_bits & ~ignore_bits) == (match_bits & ~ignore_bits) ) {

#                   if defined(MPID_DEBUG_ALL)
		    if ( MPID_DebugFlag ) {
			P31_Printf("  found matching msg in event queue\n");
			P31_Printf("  calling PtlMEUnlink( rhandle->me_handle = %d )\n",
				   rhandle->me_handle );
		    }
#                   endif

		    /* free the me and md and eq_handle */
		    P31_CALL( PtlMEUnlink( rhandle->me_handle ), PtlMEUnlink );

		    /* add the eq handle back into the list */
#                   if defined(MPID_DEBUG_ALL)
		    if ( MPID_DebugFlag ) {
			P31_Printf("  calling MPID_P31_Free_eq_handle( rhandle->eq_handle_list_ptr = %p )\n",
				   rhandle->eq_handle_list_ptr );
		    }
#                   endif

		    MPID_P31_Free_eq_handle( rhandle->eq_handle_list_ptr );

#                   if defined(MPID_DEBUG_ALL)
		    if ( MPID_DebugFlag ) {
			P31_Printf("  calling MPID_P31_Process_unex(\n");
			P31_Printf("    src_lrank  = %d\n",src_lrank);
			P31_Printf("    tag        = %d\n",tag);
			P31_Printf("    context_id = %d\n",context_id);
			P31_Printf("    rhandle    = %p\n",rhandle);
			P31_Printf("    &event     = %p\n",&event);
		    }
#                   endif

		    MPID_P31_Process_unex( src_lrank, tag, context_id, rhandle, &event );

		    return;

		} else {
		    /* add it to the unexpected queue */
		    if ( (ptr = MPID_P31_Get_unex()) == NULL ) {
			rhandle->s.MPI_ERROR =  MPI_ERR_INTERN;
			return;
		    }
		    ptr->event = event;

		}

	    } else if ( _mpi_p31_errno == PTL_EQ_DROPPED ) {
		fprintf(stderr,"%d: (%s:%d) PtlEQGet() failed : PTL_EQ_DROPPED\n",
			MPID_MyWorldRank, __FILE__, __LINE__ );
		MPID_P31_Abort( (MPI_Comm)0, MPI_ERR_INTERN, "Dropped events - Too many unexpected messages!" );
	    } else {

		fprintf(stderr,"%d: (%s:%d) PtlEQWait() failed : %s\n",
			MPID_MyWorldRank, __FILE__, __LINE__, ptl_err_str[_mpi_p31_errno]);
		rhandle->s.MPI_ERROR = MPI_ERR_INTERN;
		return;
	    }
	} else {
	    fprintf(stderr,"%d: (%s:%d) PtlMDUpdate() failed : %s\n",
		    MPID_MyWorldRank, __FILE__, __LINE__, ptl_err_str[_mpi_p31_errno]);
	    rhandle->s.MPI_ERROR = MPI_ERR_INTERN;
	    return;
	}
    }

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_RECV)
    if (MPID_DebugFlag) {
	P31_Printf("leaving MPID_Search_unexpected_queue_and_post\n");
    }
#   endif

}


