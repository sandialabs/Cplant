
#include "mpiddev.h"
#include "queue.h"

/* gettimeofday for timestamping */
#include <sys/time.h>
#include <unistd.h>

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
	P30_Printf("entering P30_Search_unexpected_queue()\n");
    }
#endif
    
    *flag = 0;

    status->MPI_ERROR = MPI_SUCCESS;

    P30_SET_RECV_BITS( match_bits, ignore_bits, context_id, src, tag );

    /* check the list of unexpected message */
    if ( (ptr = _mpi_p30_unex_list_head) != NULL ) {
	do {
	    if ( (ptr->event.match_bits & ~ignore_bits) == (match_bits & ~ignore_bits) ) {
		*flag              = 1;
		status->count      = ptr->event.rlength;
		P30_GET_SOURCE( ptr->event.match_bits, status->MPI_SOURCE );
		P30_GET_TAG( ptr->event.match_bits, status->MPI_TAG );
		return MPI_SUCCESS;
	    }
	    ptr = ptr->next;
	} while ( ptr );

    }

    /* check to see if anything else has come in */
    if ( (_mpi_p30_errno = PtlEQGet( _mpi_p30_unex_eq_handle, &event )) == PTL_OK ) {

	if ( event.type != PTL_EVENT_PUT ) {
	    P30_Printf("ERROR: was expecting a PTL_EVENT_PUT\n");
	    P30_Dump_event( &event );
	}

	/* get a free unexpected message entry */
	if ( (ptr = MPID_P30_Get_unex()) == NULL ) {
	    return MPI_ERR_INTERN;
	}
	ptr->event = event;

	if ( (ptr->event.match_bits & ~ignore_bits) == (match_bits & ~ignore_bits) ) {
	    *flag              = 1;
	    status->count      = ptr->event.rlength;
	    P30_GET_SOURCE( ptr->event.match_bits, status->MPI_SOURCE );
	    P30_GET_TAG( ptr->event.match_bits, status->MPI_TAG );
	}
    } else {
	if ( _mpi_p30_errno == PTL_EQ_DROPPED ) {
	    fprintf(stderr,"%d: (%s:%d) PtlEQGet() failed : PTL_EQ_DROPPED\n",
		    MPID_MyWorldRank, __FILE__, __LINE__ );
	    MPID_P30_Abort( (MPI_Comm)0, MPI_ERR_INTERN, "Too many unexpected messages!" );
	} else if ( _mpi_p30_errno != PTL_EQ_EMPTY ) {
	    fprintf(stderr,"%d: (%s:%d) PtlEQGet() failed : %s\n",
		    MPID_MyWorldRank, __FILE__, __LINE__, ptl_err_str[_mpi_p30_errno]);
	    MPID_P30_Abort( (MPI_Comm)0, MPI_ERR_INTERN, NULL );
	}
    }

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
	P30_Printf("leaving P30_Search_unexpected_queue()\n");
    }
#endif

    return MPI_SUCCESS;
}

/***************************************************************************

  MPID_P30_Process_unex

  Process an unexpected message

  ***************************************************************************/
void MPID_P30_Process_unex( int src_lrank, int tag, int context_id,
			   MPIR_RHANDLE *rhandle, ptl_event_t *event )
{
    ptl_process_id_t src;
    ptl_md_t         md;

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P30_Printf("entering MPID_P30_Process_unex(\n");
	P30_Printf("  src_lrank  = %d\n",src_lrank);
	P30_Printf("  tag        = %d\n",tag);
	P30_Printf("  context_id = %d\n",context_id);
	P30_Printf("  rhandle    = %p\n",rhandle);
	P30_Printf("  event      = %p\n",event);
    }
#   endif

    /* fill in the source rank and tag */
    P30_GET_TAG( event->match_bits, rhandle->s.MPI_TAG );
    P30_GET_SOURCE( event->match_bits, rhandle->s.MPI_SOURCE );
    rhandle->from         = event->initiator.rid;

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P30_Printf("  rhandle->s.MPI_TAG    = %d\n",rhandle->s.MPI_TAG);
	P30_Printf("  rhandle->s.MPI_SOURCE = %d\n",rhandle->s.MPI_SOURCE);
	P30_Printf("  rhandle->from         = %d\n",rhandle->from);
	P30_Printf("  event->mlength        = %d\n",event->mlength);
	P30_Printf("  event->rlength        = %d\n",event->rlength);
	P30_Printf("  event->match_bits     = 0x%lx\n",event->match_bits);
    }
#   endif


    if ( P30_IS_LONG( event->match_bits ) ) {

#       if defined(MPID_DEBUG_ALL)
	if ( MPID_DebugFlag ) {
	    P30_Printf("  long protocol message\n");
	}
#       endif

	/* check for truncation */
	if ( event->rlength > rhandle->len ) {
	    rhandle->s.MPI_ERROR = MPI_ERR_TRUNCATE;
	    rhandle->s.count     = rhandle->len;

#           if defined(MPID_DEBUG_ALL)
	    if ( MPID_DebugFlag ) {
		P30_Printf("  MESSAGE TRUNCATED\n");
		P30_Printf("  rhandle->len = %d\n",rhandle->len);
	    }
#           endif

	} else {
	    rhandle->s.count             =
		rhandle->bytes_as_contig = event->rlength;
	}

	/* long protocol */
	if ( (rhandle->eq_handle_list_ptr = MPID_P30_Get_eq_handle()) == NULL ) {
	    rhandle->s.MPI_ERROR = MPI_ERR_INTERN;
	    return;
	}

	/* set the src address */
	src.addr_kind = PTL_ADDR_GID;
	src.gid       = _mpi_p30_my_id.gid;
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
	    P30_Printf("  calling PtlMDBind(\n");
	    P30_Printf("    _mpi_p30_ni_handle  = %d\n",_mpi_p30_ni_handle);
	    P30_Dump_md( &md );
	    P30_Printf("    &rhandle->md_handle = %p\n",&rhandle->md_handle);
	}
#       endif

	/* create a free-floating md */
	P30_CALL( PtlMDBind( _mpi_p30_ni_handle,
			     md,
			     &rhandle->md_handle ), PtlMDBind );

#       if defined(MPID_DEBUG_ALL)
	if ( MPID_DebugFlag ) {
	    P30_Printf("  calling PtlGet(\n");
	    P30_Printf("    rhandle->md_handle   = %d\n",rhandle->md_handle);
	    P30_Printf("    src                  =>\n");
	    P30_Printf("      addr_kind          = %d\n",src.addr_kind);
	    P30_Printf("      gid                = %d\n",src.gid);
	    P30_Printf("      rid                = %d\n",src.rid);
	    P30_Printf("    _mpi_p30_read_portal = %d\n",_mpi_p30_read_portal);
	    P30_Printf("    acl                  = 0\n");
	    P30_Printf("    match bits           = 0x%lx\n",event->hdr_data);
	    P30_Printf("    offset               = 0\n");
	}
#       endif

	/* get the message */
	P30_CALL( PtlGet( rhandle->md_handle,
			  src,
			  _mpi_p30_read_portal,
			  0,
			  event->hdr_data,
			  0 ), PtlGet );

	rhandle->got_sent    = 0;
	rhandle->got_reply   = 0;

	rhandle->is_complete = 0;
	rhandle->test        = MPID_P30_Test_pulled;
	rhandle->push        = 0;
	rhandle->wait        = MPID_P30_Wait_pulled;
	rhandle->cancel      = 0;

#       if defined(MPID_DEBUG_ALL)
	if ( MPID_DebugFlag ) {
	    P30_Printf("leaving MPID_P30_Process_unex()\n");
	}
#       endif

	return;
	
    }

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P30_Printf("  short protocol message\n");
	P30_Printf("  event->mlength = %d\n",event->mlength);
	P30_Printf("  event->rlength = %d\n",event->rlength);
    }
#   endif

    /* check for truncation */
    if ( event->mlength > rhandle->len ) {
	rhandle->s.MPI_ERROR = MPI_ERR_TRUNCATE;
	rhandle->s.count     = 
	    rhandle->bytes_as_contig = rhandle->len;

#       if defined(MPID_DEBUG_ALL)
	if ( MPID_DebugFlag ) {
	    P30_Printf("  MESSAGE TRUNCATED\n");
	    P30_Printf("  rhandle->len = %d\n",rhandle->len);
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
	    P30_Printf("  calling memcpy(\n");
	    P30_Printf("    target = %p\n",rhandle->buf);
	    P30_Printf("    source = %p\n",event->md.start);
	    P30_Printf("    bytes  = %d\n",rhandle->bytes_as_contig);
	}
#       endif

	memcpy( rhandle->buf,
		event->md.start,
		rhandle->bytes_as_contig );
    }

    /* send an ack back on sync protocol */
    if ( P30_IS_SYNC(event) ) {
	/* synchronous protocol */

	src.addr_kind = PTL_ADDR_GID;
	src.gid       = _mpi_p30_my_id.gid;
	src.rid       = rhandle->from;

#       if defined(MPID_DEBUG_ALL)
	if ( MPID_DebugFlag ) {
	    P30_Printf("  synchronous protocol\n");
	    P30_Printf("  calling PtlPut(\n");
	    P30_Printf("    md_handle             = %d\n",_mpi_p30_ack_request_md_handle);
	    P30_Printf("    ack                   = PTL_NOACK_REQ\n");
	    P30_Printf("    src                   =>\n");
	    P30_Printf("      addr_kind           = %d\n",src.addr_kind);
	    P30_Printf("      gid                 = %d\n",src.gid);
	    P30_Printf("      rid                 = %d\n",src.rid);
	    P30_Printf("    _mpi_p30_ack_portal   = %d\n",_mpi_p30_ack_portal);
	    P30_Printf("    acl                   = 0\n");
	    P30_Printf("    event->match_bits     = 0x%lx\n",event->hdr_data);
	    P30_Printf("    offset                = 0\n");
	    P30_Printf("    hdr_data              = 0 )\n");
	}
#       endif

	/* send an ack back */
	P30_CALL( PtlPut( _mpi_p30_ack_request_md_handle,
			  PTL_NOACK_REQ,
			  src,
			  _mpi_p30_ack_portal,
			  0,
			  (ptl_match_bits_t)event->hdr_data,
			  0,
			  0 ), PtlPut );
    }

    /* short protocol is done */
    rhandle->is_complete = 1;

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P30_Printf("leaving MPID_P30_Process_unex()\n");
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
    ptl_handle_md_t   dummy;

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_RECV)
    if (MPID_DebugFlag) {
	P30_Printf("entering MPID_Search_unexpected_queue_and_post\n");
    }
#   endif

    MPID_P30_Check_for_drop();

    rhandle->s.MPI_ERROR  = MPI_SUCCESS;
    rhandle->s.MPI_TAG    = tag;
    rhandle->s.MPI_SOURCE = src_lrank;

    P30_SET_RECV_BITS( match_bits, ignore_bits, context_id, src_lrank, tag );

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P30_Printf("  src_lrank   = %d\n",src_lrank);
	P30_Printf("  tag         = %d\n",tag);
	P30_Printf("  context_id  = %d\n",context_id);
	P30_Printf("  rhandle     = %p\n",rhandle);
	P30_Printf("  match_bits  = 0x%lx\n",match_bits);
	P30_Printf("  ignore_bits = 0x%lx\n",ignore_bits);
	P30_Printf("  checking unexpected queue\n");
    }
#   endif

    /* check the list of unexpected message */
    if ( (ptr = _mpi_p30_unex_list_head) ) {
	do {
	    
	    if ( (ptr->event.match_bits & ~ignore_bits) == (match_bits & ~ignore_bits) ) {
#               if defined(MPID_DEBUG_ALL)
		if ( MPID_DebugFlag ) {
		    P30_Printf("  found matching msg in unexpected queue\n");
		    P30_Printf("  calling MPID_P30_Process_unex(\n");
		    P30_Printf("    src_lrank  = %d\n",src_lrank);
		    P30_Printf("    tag        = %d\n",tag);
		    P30_Printf("    context_id = %d\n",context_id);
		    P30_Printf("    rhandle    = %p\n",rhandle);
		    P30_Printf("    ptr        = %p\n",ptr);
		}
#               endif

		/* found it */
		MPID_P30_Process_unex( src_lrank, tag, context_id, rhandle, &ptr->event );

		/* free the unexpected ptr */
		MPID_P30_Free_unex( ptr );

		/* add the used unexpected md back into the list */
		if ( P30_IS_SHORT(ptr->event.match_bits) ) {

		    /* add the used unexpected md back into the list */
		    ptr->event.md.length     = _mpi_p30_short_size;
		    ptr->event.md.threshold  = 1;
                    ptr->event.md.max_offset = ptr->event.md.length;
		    ptr->event.md.options    = PTL_MD_OP_PUT | PTL_MD_ACK_DISABLE;
		    ptr->event.md.eventq     = _mpi_p30_unex_eq_handle;

#                   if defined(MPID_DEBUG_ALL)
		    if ( MPID_DebugFlag ) {
			P30_Printf("  calling PtlMDInsert(\n");
			P30_Printf("    _mpi_p30_dummy_md_handle = %d\n",_mpi_p30_dummy_md_handle);
			P30_Dump_md( &ptr->event.md );
			P30_Printf("    link                     = PTL_UNLINK\n");
			P30_Printf("    insert                   = PTL_INS_BEFORE\n");
			P30_Printf("    &dummy                   = %p )\n",&dummy);
		    }
#                   endif

		    P30_CALL( PtlMDInsert( _mpi_p30_dummy_md_handle,
					   ptr->event.md,
					   PTL_UNLINK,
					   PTL_INS_BEFORE,
					   &dummy ), PtlMDInsert );

		}

		return;
	    }
	    ptr = ptr->next;
	} while ( ptr );
    }

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P30_Printf("  checking event queue\n");
    }
#   endif

    /* check to see if anything else has come in */
    while ( 1 ) {

	if ( (_mpi_p30_errno = PtlEQGet( _mpi_p30_unex_eq_handle, &event )) == PTL_OK ) {

	    if ( event.type != PTL_EVENT_PUT ) {
		P30_Printf("ERROR: was expecting a PTL_EVENT_PUT\n");
		P30_Dump_event( &event );
	    }

#           if defined(MPID_DEBUG_ALL)
	    if ( MPID_DebugFlag ) {
		P30_Printf("  found an event\n");
		P30_Dump_event( &event );
	    }
#           endif

	    if ( (event.match_bits & ~ignore_bits) == (match_bits & ~ignore_bits) ) {

#               if defined(MPID_DEBUG_ALL)
		if ( MPID_DebugFlag ) {
		    P30_Printf("  found matching msg in event queue\n");
		    P30_Printf("  calling MPID_P30_Process_unex(\n");
		    P30_Printf("    src_lrank  = %d\n",src_lrank);
		    P30_Printf("    tag        = %d\n",tag);
		    P30_Printf("    context_id = %d\n",context_id);
		    P30_Printf("    rhandle    = %p\n",rhandle);
		    P30_Printf("    &event     = %p\n",&event);
		}
#               endif

		/* found it */
		MPID_P30_Process_unex( src_lrank, tag, context_id, rhandle, &event );

		if ( P30_IS_SHORT( event.match_bits) ) {

		/* add the used unexpected md back into the list */
		/* fill in the md structure */
		event.md.length     = _mpi_p30_short_size;
		event.md.threshold  = 1;
		event.md.max_offset = event.md.length;
		event.md.options    = PTL_MD_OP_PUT | PTL_MD_ACK_DISABLE;
		event.md.eventq     = _mpi_p30_unex_eq_handle;

#               if defined(MPID_DEBUG_ALL)
		if ( MPID_DebugFlag ) {
		    P30_Printf("  calling PtlMDInsert(\n");
		    P30_Printf("    _mpi_p30_dummy_md_handle = %d\n",_mpi_p30_dummy_md_handle);
		    P30_Dump_md( &event.md );
		    P30_Printf("    link                     = PTL_UNLINK\n");
		    P30_Printf("    insert                   = PTL_INS_BEFORE\n");
		    P30_Printf("    &dummy                   = %p )\n",&dummy);
		}
#               endif

		P30_CALL( PtlMDInsert( _mpi_p30_dummy_md_handle,
				       event.md,
				       PTL_UNLINK,
				       PTL_INS_BEFORE,
				       &dummy ), PtlMDInsert );

		}

		return;

	    } else {
		/* get a free unexpected message entry */
		if ( (ptr = MPID_P30_Get_unex()) == NULL ) {
		    rhandle->s.MPI_ERROR =  MPI_ERR_INTERN;
		    return;
		}
		ptr->event = event;
	    }
	}
	else {
	    if ( _mpi_p30_errno == PTL_EQ_DROPPED ) {
		fprintf(stderr,"%d: (%s:%d) PtlEQGet() failed : PTL_EQ_DROPPED\n",
			MPID_MyWorldRank, __FILE__, __LINE__ );
		MPID_P30_Abort( (MPI_Comm)0, MPI_ERR_INTERN, "Too many unexpected messages!" );
	    } else if ( _mpi_p30_errno != PTL_EQ_EMPTY ) {
		fprintf(stderr,"%d: (%s:%d) PtlEQGet() failed : %s\n",
			MPID_MyWorldRank, __FILE__, __LINE__, ptl_err_str[_mpi_p30_errno]);
		MPID_P30_Abort( (MPI_Comm)0, MPI_ERR_INTERN, NULL );
	    } else {
		break; /* nothing there */
	    }
	}
    }

    /* didn't find it, so prepare to post a receive */
    if ( (rhandle->eq_handle_list_ptr = MPID_P30_Get_eq_handle()) == NULL ) {
	rhandle->s.MPI_ERROR =  MPI_ERR_INTERN;
	return;
    }

    /* set the src process id */
    src_id.addr_kind = PTL_ADDR_GID;
    src_id.gid       = _mpi_p30_my_id.gid;
    src_id.rid       = PTL_ID_ANY; /* source is in the match bits */

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P30_Printf("  didn't find message\n");
	P30_Printf("  calling PtlMEInsert(\n");
	P30_Printf("    _mpi_p30_no_match_me_handle = %d\n",_mpi_p30_no_match_me_handle);
	P30_Printf("    src_id                      =>\n");
	P30_Printf("      addr_kind                 = %d\n",src_id.addr_kind);
	P30_Printf("      gid                       = %d\n",src_id.gid);
	P30_Printf("      rid                       = PTL_ID_ANY\n");
	P30_Printf("    match_bits                  = 0x%lx\n",match_bits);
	P30_Printf("    ignore_bits                 = 0x%lx\n",ignore_bits);
	P30_Printf("    unlink                      = PTL_UNLINK\n");
	P30_Printf("    insert                      = PTL_INS_BEFORE\n");
	P30_Printf("    &rhandle->me_handle         = %p )\n",&rhandle->me_handle);
    }
#   endif

    /* get an me on the recv portal */
    P30_CALL( PtlMEInsert( _mpi_p30_no_match_me_handle,
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
	P30_Printf("  calling PtlMDAttach(\n");
	P30_Printf("    rhandle->me_handle   = %d\n",rhandle->me_handle);
	P30_Printf("    md                   =>\n");
	P30_Printf("      start              = %p\n",md.start);
	P30_Printf("      length             = %d\n",md.length);
	P30_Printf("      threshold          = %d\n",md.threshold);
	P30_Printf("      max_offset         = %d\n",md.max_offset);
	P30_Printf("      options            = %d\n",md.options);
	P30_Printf("      user_ptr           = %p\n",md.user_ptr);
	P30_Printf("      eventq             = %d\n",md.eventq);
	P30_Printf("    link                 = PTL_UNLINK\n");
	P30_Printf("    &rhandle->md_handle  = %p )\n",&rhandle->md_handle);
    }
#   endif
    
    P30_CALL( PtlMDAttach( rhandle->me_handle,
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
	    P30_Printf("  calling PtlMDUpdate(\n");
	    P30_Printf("    rhandle->md_handle   = %d\n",rhandle->md_handle);
	    P30_Printf("    old md               =>\n");
	    P30_Printf("      start              = %p\n",md.start);
	    P30_Printf("      length             = %d\n",md.length);
	    P30_Printf("      threshold          = %d\n",md.threshold);
            P30_Printf("      max_offset         = %d\n",md.max_offset);
	    P30_Printf("      options            = %d\n",md.options);
	    P30_Printf("      user_ptr           = %p\n",md.user_ptr);
	    P30_Printf("      eventq             = %d\n",md.eventq);
	    P30_Printf("    new md               =>\n");
	    P30_Printf("      start              = %p\n",new_md.start);
	    P30_Printf("      length             = %d\n",new_md.length);
	    P30_Printf("      threshold          = %d\n",new_md.threshold);
	    P30_Printf("      max_offset         = %d\n",new_md.max_offset);
	    P30_Printf("      options            = %d\n",new_md.options);
	    P30_Printf("      user_ptr           = %p\n",new_md.user_ptr);
	    P30_Printf("      eventq             = %d\n",new_md.eventq);
	    P30_Printf("    test eventq          = %d )\n",_mpi_p30_unex_eq_handle);
	}
#       endif

	_mpi_p30_errno = PtlMDUpdate( rhandle->md_handle,
				      &md,
				      &new_md,
				      _mpi_p30_unex_eq_handle );

	if ( _mpi_p30_errno == PTL_OK ) {
	    /* receive was posted */

#           if defined(MPID_DEBUG_ALL)
	    if ( MPID_DebugFlag ) {
		P30_Printf("  receive was posted\n");
	    }
#           endif

	    rhandle->is_complete = 0;
	    rhandle->test        = MPID_P30_Test_recv;
	    rhandle->push        = 0;
	    rhandle->wait        = MPID_P30_Wait_recv;
	    rhandle->cancel      = 0;

	    break;
	    
	} 
	else if ( _mpi_p30_errno == PTL_NOUPDATE ) {

#           if defined(MPID_DEBUG_ALL)
	    if ( MPID_DebugFlag ) {
		P30_Printf("  PtlMDUpdate() failed because event queue wasn't empty\n");
	    }
#           endif

	    /* wait for the event to come in */
	    if ( (_mpi_p30_errno = PtlEQWait( _mpi_p30_unex_eq_handle, &event )) == PTL_OK ) {

		if ( event.type != PTL_EVENT_PUT ) {
		    P30_Printf("ERROR: was expecting a PTL_EVENT_PUT\n");
		    P30_Dump_event( &event );
		}
#               if defined(MPID_DEBUG_ALL)
		if ( MPID_DebugFlag ) {
		    P30_Printf("  found an event\n");
		    P30_Dump_event( &event );
		}
#               endif

		if ( (event.match_bits & ~ignore_bits) == (match_bits & ~ignore_bits) ) {

#                   if defined(MPID_DEBUG_ALL)
		    if ( MPID_DebugFlag ) {
			P30_Printf("  found matching msg in event queue\n");
			P30_Printf("  calling PtlMEUnlink( rhandle->me_handle = %d )\n",
				   rhandle->me_handle );
		    }
#                   endif

		    /* free the me and md and eq_handle */
		    P30_CALL( PtlMEUnlink( rhandle->me_handle ), PtlMEUnlink );

		    /* add the eq handle back into the list */
#                   if defined(MPID_DEBUG_ALL)
		    if ( MPID_DebugFlag ) {
			P30_Printf("  calling MPID_P30_Free_eq_handle( rhandle->eq_handle_list_ptr = %p )\n",
				   rhandle->eq_handle_list_ptr );
		    }
#                   endif

		    MPID_P30_Free_eq_handle( rhandle->eq_handle_list_ptr );

#                   if defined(MPID_DEBUG_ALL)
		    if ( MPID_DebugFlag ) {
			P30_Printf("  calling MPID_P30_Process_unex(\n");
			P30_Printf("    src_lrank  = %d\n",src_lrank);
			P30_Printf("    tag        = %d\n",tag);
			P30_Printf("    context_id = %d\n",context_id);
			P30_Printf("    rhandle    = %p\n",rhandle);
			P30_Printf("    &event     = %p\n",&event);
		    }
#                   endif

		    MPID_P30_Process_unex( src_lrank, tag, context_id, rhandle, &event );

		    if ( P30_IS_SHORT( event.match_bits) ) {

		    /* add the used unexpected md back into the list */
		    /* fill in the md structure */
		    event.md.length     = _mpi_p30_short_size;
		    event.md.threshold  = 1;
		    event.md.max_offset = event.md.length;
		    event.md.options    = PTL_MD_OP_PUT | PTL_MD_ACK_DISABLE;
		    event.md.eventq     = _mpi_p30_unex_eq_handle;

#                   if defined(MPID_DEBUG_ALL)
		    if ( MPID_DebugFlag ) {
			P30_Printf("  calling PtlMDInsert(\n");
			P30_Printf("    _mpi_p30_dummy_md_handle = %d\n",_mpi_p30_dummy_md_handle);
			P30_Dump_md( &event.md );
			P30_Printf("    link                     = PTL_UNLINK\n");
			P30_Printf("    insert                   = PTL_INS_BEFORE\n");
			P30_Printf("    &dummy                   = %p )\n",&dummy);
		    }
#                   endif

		    P30_CALL( PtlMDInsert( _mpi_p30_dummy_md_handle,
					   event.md,
					   PTL_UNLINK,
					   PTL_INS_BEFORE,
					   &dummy ), PtlMDInsert );

		    }
		    
		    return;

		} else {
		    /* add it to the unexpected queue */
		    if ( (ptr = MPID_P30_Get_unex()) == NULL ) {
			rhandle->s.MPI_ERROR =  MPI_ERR_INTERN;
			return;
		    }
		    ptr->event = event;

		}

	    } else if ( _mpi_p30_errno == PTL_EQ_DROPPED ) {
		fprintf(stderr,"%d: (%s:%d) PtlEQGet() failed : PTL_EQ_DROPPED\n",
			MPID_MyWorldRank, __FILE__, __LINE__ );
		MPID_P30_Abort( (MPI_Comm)0, MPI_ERR_INTERN, "Too many unexpected messages!" );
	    } else {

		fprintf(stderr,"%d: (%s:%d) PtlEQWait() failed : %s\n",
			MPID_MyWorldRank, __FILE__, __LINE__, ptl_err_str[_mpi_p30_errno]);
		rhandle->s.MPI_ERROR = MPI_ERR_INTERN;
		return;
	    }
	} else {
	    fprintf(stderr,"%d: (%s:%d) PtlMDUpdate() failed : %s\n",
		    MPID_MyWorldRank, __FILE__, __LINE__, ptl_err_str[_mpi_p30_errno]);
	    rhandle->s.MPI_ERROR = MPI_ERR_INTERN;
	    return;
	}
    }

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_RECV)
    if (MPID_DebugFlag) {
	P30_Printf("leaving MPID_Search_unexpected_queue_and_post\n");
    }
#   endif

}


