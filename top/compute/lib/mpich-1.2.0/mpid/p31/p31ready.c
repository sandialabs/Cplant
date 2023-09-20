#include "mpiddev.h"


/***************************************************************************

  Ready send
  

  ***************************************************************************/


/***************************************************************************

  MPID_P31_Ready_send

  ***************************************************************************/
int MPID_P31_Ready_send( buf, len, src_lrank, tag, context_id, dest,
			  msgrep, comm )
void *buf;
int  len, tag, context_id, src_lrank, dest, msgrep;
struct MPIR_COMMUNICATOR *comm;
{
    MPIR_SHANDLE shandle;
    int          mpi_errno = MPI_SUCCESS;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P31_Printf("entering MPID_P31_Ready_send()\n");
	P31_Printf("calling MPID_P31_Ready_isend()\n");
    }
#endif

    if ( (mpi_errno = MPID_P31_Ready_isend( buf, len, src_lrank, tag,
					     context_id, dest, msgrep,
					     &shandle, comm ) ) != MPI_SUCCESS ) {
	return mpi_errno;
    }

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P31_Printf("calling MPID_P31_Ready_wait_send()\n");
    }
#endif

    mpi_errno = shandle.wait( &shandle );

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P31_Printf("leaving MPID_P31_Ready_send()\n");
    }
#endif

    return mpi_errno;
}


/***************************************************************************

  MPID_P31_Ready_isend
  
  nonblocking ready send

  ***************************************************************************/
int MPID_P31_Ready_isend( buf, len, src_lrank, tag, context_id, dest,
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
	P31_Printf("entering MPID_P31_Ready_isend_ready-calling portal send-\n");
	P31_Printf("  buf        = %p\n",buf);
	P31_Printf("  len        = %d\n",len);
	P31_Printf("  src_lrank  = %d\n",src_lrank);
	P31_Printf("  tag        = %d\n",tag);
	P31_Printf("  context_id = %d\n",context_id);
	P31_Printf("  dest       = %d\n",dest);
	P31_Printf("  shandle    = %p\n",shandle);
    }
#   endif

    if ( (shandle->eq_handle_list_ptr = MPID_P31_Get_eq_handle()) == NULL ) {
	return MPI_ERR_INTERN;
    }

    md.start      = buf;
    md.length     = len;
    md.threshold  = 1;
    md.max_offset = md.length;
    md.options    = 0;
    md.user_ptr   = shandle;
    md.eventq     = shandle->eq_handle_list_ptr->eq_handle;

    /* bind the memory descriptor to the interface */
    P31_CALL( PtlMDBind( _mpi_p31_ni_handle,
			 md,
			 &shandle->md_handle ), PtlMDBind );

    dest_id.addr_kind = PTL_ADDR_GID;
    dest_id.gid       = _mpi_p31_my_id.gid;
    dest_id.rid       = dest;
    
    /* fill these in */
    P31_SET_READY_SEND_BITS( match_bits, context_id, src_lrank, tag );

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P31_Printf("  calling PtlPut(\n");
	P31_Printf("    shandle->md_handle   = %d\n",shandle->md_handle);
	P31_Printf("    ack                  = PTL_NOACK_REQ\n");
	P31_Printf("    dest_id              =>\n");
	P31_Printf("      addr_kind          = %d\n",dest_id.addr_kind);
	P31_Printf("      gid                = %d\n",dest_id.gid);
	P31_Printf("      rid                = %d\n",dest_id.rid);
	P31_Printf("    _mpi_p31_recv_portal = %d\n",_mpi_p31_recv_portal);
	P31_Printf("    acl                  = 0\n");
	P31_Printf("    match_bits           = 0x%lx\n",match_bits );
	P31_Printf("    offset               = 0\n");
	P31_Printf("    hdr_data             = 0 )\n");
    }
#   endif

    P31_CALL( PtlPut( shandle->md_handle,
		      PTL_NOACK_REQ,
		      dest_id,
		      _mpi_p31_recv_portal,
		      0,
		      match_bits,
		      0,
		      0 ), PtlPut );

    shandle->is_complete     = 0;
    shandle->errval          = MPI_SUCCESS;
    shandle->comm            = comm;
    shandle->start           = buf;
    shandle->bytes_as_contig = len;
    shandle->test	     = MPID_P31_Short_test_send;
    shandle->wait	     = MPID_P31_Short_wait_send;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P31_Printf("leaving MPID_P31_Ready_isend()\n");
    }
#endif

    return MPI_SUCCESS;
}


/***************************************************************************

  MPID_P31_Short_delete

  cleanup

  ***************************************************************************/
void MPID_P31_Ready_delete( MPID_Protocol *p )
{

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P31_Printf("entering MPID_P31_Ready_delete()\n");
    }
#endif

    FREE( p );

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P31_Printf("leaving MPID_P31_Ready_delete()\n");
    }
#endif

}



/***************************************************************************

  MPID_P31_Ready_setup
  
  set up the ready protocol structure

  ***************************************************************************/
MPID_Protocol *MPID_P31_Ready_setup()
{
    MPID_Protocol *p;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P31_Printf("entering MPID_P31_Ready_setup()\n");
    }
#endif

    p = (MPID_Protocol *) MALLOC( sizeof(MPID_Protocol) );
    if (!p) return 0;
    p->send	   = MPID_P31_Ready_send;
    p->recv	   = 0;
    p->isend	   = MPID_P31_Ready_isend;
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
        P31_Printf("leaving MPID_P31_Short_setup()\n");
    }
#endif

    return p;
}
