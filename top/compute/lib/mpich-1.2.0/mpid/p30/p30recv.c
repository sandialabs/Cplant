#include "mpiddev.h"

/***************************************************************************

  MPID_P30_Test_recv
  
  Check to see if a pre-posted recv request has completed

  ***************************************************************************/
int MPID_P30_Test_recv( MPIR_RHANDLE *rhandle )
{
    int mpi_errno = MPI_SUCCESS;
    ptl_event_t event;

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P30_Printf("entering P30_Test_recv()\n");
	P30_Printf("  calling PtlEQGet(\n");
	P30_Printf("    eq_handle = %d\n",rhandle->eq_handle_list_ptr->eq_handle);
	P30_Printf("    &event    = %p )\n",&event);
    }
#   endif

    MPID_P30_Check_for_drop();

    if ( (_mpi_p30_errno = PtlEQGet( rhandle->eq_handle_list_ptr->eq_handle,
				     &event )) == PTL_OK ) {

#       if defined(MPID_DEBUG_ALL)
	if ( MPID_DebugFlag ) {
	    P30_Printf("  got an event");
	    P30_Dump_event( &event );
	}
#       endif

	if ( event.type != PTL_EVENT_PUT ) {
	    P30_Printf("ERROR: was expecting a PTL_EVENT_PUT\n");
	    P30_Dump_event( &event );
	}

	/* fill in the status info */
	P30_GET_TAG( event.match_bits, rhandle->s.MPI_TAG );
	P30_GET_SOURCE( event.match_bits, rhandle->s.MPI_SOURCE );
	rhandle->s.count      = event.mlength;

#       if defined(MPID_DEBUG_ALL)
	if ( MPID_DebugFlag ) {
	    P30_Printf("  rhandle->s.MPI_TAG    = %d\n",rhandle->s.MPI_TAG);
	    P30_Printf("  rhandle->s.MPI_SOURCE = %d\n",rhandle->s.MPI_SOURCE);
	    P30_Printf("  rhandle->s.count      = %d\n",rhandle->s.count);
	}
#       endif

	/* check for truncation */
	if ( event.rlength > event.mlength ) {
#           if defined(MPID_DEBUG_ALL)
	    if ( MPID_DebugFlag ) {
		P30_Printf("  message was truncated\n");
	    }
#           endif
	    mpi_errno = rhandle->s.MPI_ERROR = MPI_ERR_TRUNCATE;
	    fprintf(stderr,"%d: MPI ERROR: truncated message: wanted %d, got %d\n",
		MPID_MyWorldRank, event.rlength, event.mlength );
	}

#       if defined(MPID_DEBUG_ALL)
	if ( MPID_DebugFlag ) {
	    P30_Printf("calling MPID_P30_Free_eq_handle( ptr = %p )\n",
		       rhandle->eq_handle_list_ptr);
	    P30_Printf("recv with handle %p is done\n",rhandle);
	}
#       endif

	/* add the eq handle back into the list */
	MPID_P30_Free_eq_handle( rhandle->eq_handle_list_ptr );

	rhandle->is_complete = 1;
    } else {
	if ( _mpi_p30_errno != PTL_EQ_EMPTY ) {
	    fprintf(stderr,"%d: (%s:%d) PtlEQGet() failed : %s\n",
		    MPID_MyWorldRank, __FILE__, __LINE__, ptl_err_str[_mpi_p30_errno]);
	}
    }
    
#ifdef MPID_DEBUG_ALL
	if ( MPID_DebugFlag ) {
	    P30_Printf("leaving P30_Test_recv()\n");
	}
#endif

    return mpi_errno;
    
}


/***************************************************************************

  MPID_P30_Wait_recv

  Wait for a pre-posted recv request to be completed

  ***************************************************************************/
int MPID_P30_Wait_recv( MPIR_RHANDLE *rhandle )
{
    int mpi_errno = MPI_SUCCESS;
    ptl_event_t event;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
	P30_Printf("entering P30_Wait_recv()\n");
	P30_Printf("  calling PtlEQWait(\n");
	P30_Printf("    eq_handle = %d\n",rhandle->eq_handle_list_ptr->eq_handle);
	P30_Printf("    &event    = %p )\n",&event);
    }
#endif

    MPID_P30_Check_for_drop();

    if ( (_mpi_p30_errno = PtlEQWait( rhandle->eq_handle_list_ptr->eq_handle, &event )) != PTL_OK ) {
	fprintf(stderr,"%d: (%s:%d) PtlEQWait() failed : %s\n",
		MPID_MyWorldRank, __FILE__, __LINE__, ptl_err_str[_mpi_p30_errno]);
    }

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P30_Printf("  got an event\n");
	P30_Dump_event( &event );
    }
#   endif

    if ( event.type != PTL_EVENT_PUT ) {
	P30_Printf("ERROR: was expecting a PTL_EVENT_PUT\n");
	P30_Dump_event( &event );
    }

    /* fill in the status info */
    P30_GET_TAG( event.match_bits, rhandle->s.MPI_TAG );
    P30_GET_SOURCE( event.match_bits, rhandle->s.MPI_SOURCE );
    rhandle->s.count      = event.mlength;

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P30_Printf("  rhandle->s.MPI_TAG    = %d\n",rhandle->s.MPI_TAG);
	P30_Printf("  rhandle->s.MPI_SOURCE = %d\n",rhandle->s.MPI_SOURCE);
	P30_Printf("  rhandle->s.count      = %d\n",rhandle->s.count);
    }
#   endif

	/* check for truncation */
    if ( event.rlength > event.mlength ) {
#      if defined(MPID_DEBUG_ALL)
	if ( MPID_DebugFlag ) {
	    P30_Printf("  message was truncated\n");
	}
#       endif
	fprintf(stderr,"%d: MPI ERROR: truncated message: wanted %d, got %d\n",
	    MPID_MyWorldRank, event.rlength, event.mlength );
	mpi_errno = rhandle->s.MPI_ERROR = MPI_ERR_TRUNCATE;
    }

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P30_Printf("calling MPID_P30_Free_eq_handle( ptr = %p )\n",
		   rhandle->eq_handle_list_ptr);
	P30_Printf("recv with handle %p is done\n",rhandle);
	}
#   endif

    /* add the eq handle back into the list */
    MPID_P30_Free_eq_handle( rhandle->eq_handle_list_ptr );

    rhandle->is_complete = 1;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
	P30_Printf("leaving P30_Wait_recv()\n");
    }
#endif
    
    return mpi_errno;

}

/***************************************************************************

  MPID_P30_Test_pulled
  
  Check to see if a pulled recv request has completed

  ***************************************************************************/
int MPID_P30_Test_pulled( MPIR_RHANDLE *rhandle )
{
    int mpi_errno = MPI_SUCCESS;
    ptl_event_t event;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
	P30_Printf("entering P30_Test_pulled_sent()\n");
    }
#endif

    if ( (_mpi_p30_errno = PtlEQGet( rhandle->eq_handle_list_ptr->eq_handle,
				     &event )) == PTL_OK ) {

#       if defined(MPID_DEBUG_ALL)
	if ( MPID_DebugFlag ) {
	    P30_Printf("  got an event\n");
	    P30_Dump_event( &event );
	}
#       endif

	if ( event.type == PTL_EVENT_SENT ) {
	    rhandle->got_sent = 1;
	}

	if ( event.type == PTL_EVENT_REPLY ) {
	    rhandle->got_reply = 1;
	}

	if ( rhandle->got_sent && rhandle->got_reply ) {

	    /* add the eq handle back into the list */
	    MPID_P30_Free_eq_handle( rhandle->eq_handle_list_ptr );

	    rhandle->is_complete = 1;

	}

    } else {
	if ( _mpi_p30_errno != PTL_EQ_EMPTY ) {
	    fprintf(stderr,"%d: (%s:%d) PtlEQGet() failed : %s\n",
		    MPID_MyWorldRank, __FILE__, __LINE__, ptl_err_str[_mpi_p30_errno]);
	}
    }

#ifdef MPID_DEBUG_ALL
	if ( MPID_DebugFlag ) {
	    P30_Printf("leaving P30_Test_pulled_sent()\n");
	}
#endif

    return mpi_errno;
    
}


/***************************************************************************

  MPID_P30_Wait_pulled

  ***************************************************************************/
int MPID_P30_Wait_pulled( MPIR_RHANDLE *rhandle )
{
    int mpi_errno = MPI_SUCCESS;
    ptl_event_t event1,event2;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
	P30_Printf("entering P30_Wait_pulled_sent()\n");
	P30_Printf("  calling PtlEQWait(\n");
	P30_Printf("    eq_handle     = %d\n",rhandle->eq_handle_list_ptr->eq_handle);
	P30_Printf("    &event1        = %p\n",&event1);
    }
#endif
    
    if ( rhandle->got_sent == 0 ) {

	if ( (_mpi_p30_errno = PtlEQWait( rhandle->eq_handle_list_ptr->eq_handle, &event1 )) != PTL_OK ) {
	    fprintf(stderr,"%d: (%s:%d) PtlEQWait() failed : %s\n",
		    MPID_MyWorldRank, __FILE__, __LINE__, ptl_err_str[_mpi_p30_errno]);
	}

#       ifdef MPID_DEBUG_ALL
	if ( MPID_DebugFlag ) {
	    P30_Printf("  got an event\n");
	    P30_Dump_event( &event1 );
	}
#       endif

	rhandle->got_sent = 1;

    }


    if ( rhandle->got_reply == 0 ) {

	if ( (_mpi_p30_errno = PtlEQWait( rhandle->eq_handle_list_ptr->eq_handle, &event2 )) != PTL_OK ) {
	    fprintf(stderr,"%d: (%s:%d) PtlEQWait() failed : %s\n",
		    MPID_MyWorldRank, __FILE__, __LINE__, ptl_err_str[_mpi_p30_errno]);
	}

#       ifdef MPID_DEBUG_ALL
	if ( MPID_DebugFlag ) {
	    P30_Printf("  got an event\n");
	    P30_Dump_event( &event2 );
	}
#       endif

	rhandle->got_reply = 1;

    }

    /* add the eq handle back into the list */
    MPID_P30_Free_eq_handle( rhandle->eq_handle_list_ptr );

    rhandle->is_complete = 1;

    /* all of the status information should already be filled in */
	
    return mpi_errno;

}


