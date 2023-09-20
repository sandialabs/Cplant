#include "ptlsdev.h"

/****************************************************************************
  Global variables
 ***************************************************************************/
struct _MPIR_COLLOPS   ptls_collops;
struct _MPIR_COLLOPS  *default_collops;
CHAR                 **ptls_data_ptrs;
VOID                  *ptls_coll_work;
int                    ptls_coll_work_size;

/***************************************************************************
  _barrier

  This really should be in ptls/src/lib/coll/collcore.c but Shuler felt that
  a barrier wasn't a 'core' collective routine.

  ***************************************************************************/
int _barrier( INT32 tag, INT32 list[], INT32 listlen, INT32 mylistpos,
    COLL_STRUCT *c_st)
{
    int rc;
    int root=0;

    /* Perform a fan-in reduction */
    if ( (rc = _reduce_short( PUMA_NULLOP, (CHAR *) NULL, 0, 0,
			      tag, list, 1, listlen, 
			      mylistpos, root, 
			      (CHAR *) NULL, c_st)) == 0 ) {
	
	/* Followed by a fan-out broadcast */
	rc = _bcast_short( (CHAR *) NULL, 0, tag, list, 1,
			   listlen, mylistpos, root, c_st );
    }
    
    return rc;

}

/***************************************************************************

  MPID_Comm_init  

 ***************************************************************************/
int MPID_CommInit( comm, newcomm )
MPI_Comm comm;
struct MPIR_COMMUNICATOR *newcomm;
{

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_COLL)
    if (MPID_DebugFlag) {
	PTLS_Printf("PTLS_Comm_init\n");
    }
#   endif

	    
    if ( newcomm->comm_type == MPIR_INTRA ) {

	if (_coll_struct_init( &newcomm->coll_struct ) ) {
	    perror( "_coll_struct_init FAIILED" );
	    return MPI_ERR_INTERN;
	}
	newcomm->coll_cntr = 0;
    }

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_COLL)
    if (MPID_DebugFlag) {
	PTLS_Printf("Leaving PTLS_Comm_init\n");
    }
#   endif

    return (MPI_SUCCESS);
}    


/***************************************************************************

   PTLS_Comm_free

 ***************************************************************************/
int MPID_CommFree( comm )
struct MPIR_COMMUNICATOR *comm;
{

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
	PTLS_Printf("entering PTLS_CommFree()\n");
    }
#endif

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
	PTLS_Printf("leaving PTLS_CommFree()\n");
    }
#endif

    return MPI_SUCCESS;
}

/***************************************************************************
  PTLS_Barrier
  ***************************************************************************/
int PTLS_Barrier( struct MPIR_COMMUNICATOR *comm ) 
{
    int	      size;
    int	      rank;

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_COLL)
    if (MPID_DebugFlag) {
	PTLS_Printf("PTLS_Barrier\n");
    }
#   endif

    size = comm->np;

    if ( size > 1 ) {
    
	if ( _barrier( COLL_TAG( comm->comm_coll->send_context,
                                 comm->comm_coll->coll_cntr ),
			comm->comm_coll->lrank_to_grank,
			comm->comm_coll->np,
			comm->comm_coll->local_rank, 
                        &comm->comm_coll->coll_struct ) ) {
	    return MPI_ERR_INTERN;
	}
    }

    return MPI_SUCCESS;
  
}

/***************************************************************************
  PTLS_Bcast
  ***************************************************************************/
int PTLS_Bcast( void* buffer, int count, struct MPIR_DATATYPE *datatype, int root, 
	       struct MPIR_COMMUNICATOR *comm )
{
    int       totalbytes;
    int       size;
    int       nsize;
    int       tag;

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_COLL)
    if (MPID_DebugFlag) {
	PTLS_Printf("PTLS_Bcast\n");
    }
#   endif

    /* Perform ADI broadcast operation */

    size = comm->np;

    if ( size == 1 ) {
	return MPI_SUCCESS;
    }

    if ( (root >= size) || (root < 0) ) {
	return MPIR_ERROR( comm, MPI_ERR_ROOT, "Invalid root in MPI_BCAST" );
    }

    MPI_Type_size( datatype->self, &nsize );

    totalbytes = nsize * count;

    tag = COLL_TAG( comm->comm_coll->send_context,
		    comm->comm_coll->coll_cntr );

    /* estimate, might call _bcast_time() */
    if ( totalbytes < 50000 ) {
    /* if ( 1 ) { */

#       if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_COLL)
	if (MPID_DebugFlag) {
	    PTLS_Printf("PTLS_Bcast-calling _bcast_short()\n");
	}
#       endif

	_bcast_short( (CHAR *)buffer,
		      (INT32)totalbytes,
		      (INT32)tag,
		      (INT32 *)comm->comm_coll->lrank_to_grank,
		      (INT32)1,
		      (INT32)comm->comm_coll->np,
		      (INT32)comm->comm_coll->local_rank,
		      (INT32)root,
		      (COLL_STRUCT *)&comm->comm_coll->coll_struct );
    }
    else {


#       if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_COLL)
	if (MPID_DebugFlag) {
	    PTLS_Printf("PTLS_Bcast-calling _bcast_long()\n");
	}
#       endif

	FORM_DATA_PTRS( ptls_data_ptrs, 
		        buffer, 
		        totalbytes, 
		        nsize, 
		        comm->comm_coll->np );

	_bcast_long( ptls_data_ptrs, 
		     1, 
		     tag, 
		     (INT32 *)comm->comm_coll->lrank_to_grank,
		     1,
		     (INT32)comm->comm_coll->np,
		     (INT32)comm->comm_coll->local_rank,
		     (INT32)root,
		     (COLL_STRUCT *)&comm->comm_coll->coll_struct,
		     (BOOLEAN)0 );

    }

    return MPI_SUCCESS;

}


/***************************************************************************
  PTLS_Gather
  ***************************************************************************/
int PTLS_Gather( void* sendbuf, int sendcount, struct MPIR_DATATYPE *sendtype, void* recvbuf, 
		int recvcount, struct MPIR_DATATYPE *recvtype, int root, struct MPIR_COMMUNICATOR *comm)
{
    void     *recv_buffer;
    int       totalbytes,i;
    int       send_size;
    unsigned int newtag;
    int       size;
 
#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_COLL)
    if (MPID_DebugFlag) {
        PTLS_Printf("PTLS_Gather\n");
    }
#   endif
 
    size = comm->np;
 
    if ( (root>=size) || (root<0) ) {
        return MPIR_ERROR( comm, MPI_ERR_ROOT, "Invalid root in MPI_Gather" );
    }
 
    if ( size == 1 ) {
	MPI_Type_size( recvtype->self, &size );
	memcpy( recvbuf,
		sendbuf,
		size*recvcount );
	return MPI_SUCCESS;
    }

    MPI_Type_size( sendtype->self, &send_size );
    totalbytes = send_size * (sendcount) * comm->comm_coll->np;
 
    if ( comm->comm_coll->local_rank != root ) {
        if ( ((recv_buffer = (void *)PTLS_COLL_WORK_MALLOC(totalbytes)) == 
	    (void *)NULL) && (totalbytes) ) {
            perror("malloc failed");
            return MPI_ERR_INTERN;
        }
    } else {
	recv_buffer = recvbuf;
    }

    for (i=0; i<= comm->comm_coll->np; i++) {
	ptls_data_ptrs[i] = &(((char *)recv_buffer)[i*send_size*sendcount]);
    }
 
    if ( (send_size * sendcount) > 0 ) {
        memcpy( ptls_data_ptrs[comm->comm_coll->local_rank],
                sendbuf,
                send_size * sendcount );
    }
 
#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_COLL)
    if (MPID_DebugFlag) {
        PTLS_Printf("PTLS_Gather-calling _gather()\n");
    }
#   endif
 
    _gather( ptls_data_ptrs,
             1,
	     COLL_TAG( comm->comm_coll->send_context, 
                       comm->comm_coll->coll_cntr ),
             (INT32 *)comm->comm_coll->lrank_to_grank,
             1,
             (INT32)comm->comm_coll->np,
             (INT32)comm->comm_coll->local_rank,
             (INT32)root,
             (COLL_STRUCT *)&comm->comm_coll->coll_struct );

    return MPI_SUCCESS;
 
}


/***************************************************************************
  PTLS_Gatherv
  ***************************************************************************/
int PTLS_Gatherv(void* sendbuf, int sendcount, struct MPIR_DATATYPE *sendtype, void* recvbuf, 
		 int *recvcounts, int *displs, struct MPIR_DATATYPE *recvtype, int root, 
		 struct MPIR_COMMUNICATOR *comm)
{

    void     *recv_buf;
    int       totalbytes,i;
    int       np        = comm->comm_coll->np;
    int       my_rank   = comm->comm_coll->local_rank;
    int       send_size;
    int       recv_count;
    int       size;
    int      *recv_size;
    int      *recv_counts;
    int      *recv_info=NULL;
    int       recv_info_size;

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_COLL)
    if (MPID_DebugFlag) {
	PTLS_Printf("PTLS_Gatherv\n");
    }
#   endif

    size = comm->np;

    if ( (root>=size) || (root<0) ) {
	return MPIR_ERROR( comm, MPI_ERR_ROOT, "Invalid root in MPI_GATHERV" );
    }

    if ( size == 1 ) {
	MPI_Type_size( recvtype->self, &size );
	memcpy( &( ((char *)recvbuf)[size*displs[0]] ),
		sendbuf,
		size*recvcounts[0] );
	return MPI_SUCCESS;
    }

    recv_info_size = (np+1)*sizeof(int);
    if ( ((recv_info = (int *)PTLS_MALLOC( recv_info_size ) ) == 
	(void *)NULL) && (recv_info_size) ) {
	perror( "malloc() failed" );
	return MPI_ERR_INTERN;
    }
    recv_size   = &recv_info[0];
    recv_counts = &recv_info[1];
 
    if ( my_rank == root ) {
        MPI_Type_size( recvtype->self, recv_size );
	memcpy( recv_counts,
	        recvcounts,
		np*sizeof(int) );
    }
 
    /* broadcast the info significant only at root to everybody else */
    _bcast_short( (CHAR *)recv_info,
                  (INT32)recv_info_size,
		  COLL_TAG( comm->comm_coll->send_context,
                            comm->comm_coll->coll_cntr ),
                  (INT32 *)comm->comm_coll->lrank_to_grank,
                  (INT32)1,
                  (INT32)np,
                  (INT32)my_rank,
                  (INT32)root,
                  (COLL_STRUCT *)&comm->comm_coll->coll_struct );

    recv_count = 0;
    for (i=0;i<np;i++) {
	recv_count += recv_counts[i];
    }
    totalbytes = (recv_count) * (*recv_size);

    if ( ((recv_buf = (void *)PTLS_COLL_WORK_MALLOC( totalbytes) ) == 
	(void *)NULL) && (totalbytes) ) {
	perror( "malloc() failed" );
	return MPI_ERR_INTERN;
    }

    ptls_data_ptrs[0] = (char *)recv_buf;
    for ( i=0; i<np; i++ ) {
	ptls_data_ptrs[i+1] = (char *)( (PTRINT)(ptls_data_ptrs[i]) + 
			   (PTRINT)(recv_counts[i]*(*recv_size)) );
    }

    MPI_Type_size( sendtype->self, &send_size );
    if ( (send_size * sendcount) > 0 ) {
	memcpy( ptls_data_ptrs[my_rank],
	        sendbuf,
	        send_size * sendcount );
    }

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_COLL)
    if (MPID_DebugFlag) {
	PTLS_Printf("PTLS_Gatherv-calling _gather()\n");
    }
#   endif
 
    _gather( ptls_data_ptrs,
	     1,
	     COLL_TAG( comm->comm_coll->send_context, 
                       comm->comm_coll->coll_cntr ),
	     (INT32 *)comm->comm_coll->lrank_to_grank,
	     1,
	     (INT32)np,
	     (INT32)my_rank,
	     (INT32)root,
	     (COLL_STRUCT *)&comm->comm_coll->coll_struct );

    if ( my_rank == root ) {
	for ( i=0; i<np; i++ ) {
	    memcpy( &(((char *)recvbuf)[ (*recv_size)*displs[i] ]),
		    ptls_data_ptrs[i],
		    (*recv_size) * recvcounts[i] );
	}
    }

    if (recv_info) {
	PTLS_FREE(recv_info);
    }

    return MPI_SUCCESS;

  
}

/***************************************************************************
  PTLS_Scatter
  ***************************************************************************/
int PTLS_Scatter(void* sendbuf, int sendcount, struct MPIR_DATATYPE *sendtype, void* recvbuf, 
		 int recvcount, struct MPIR_DATATYPE *recvtype, int root, struct MPIR_COMMUNICATOR *comm)
{

    void     *send_buf;
    int       i;
    int       totalbytes;
    int       recv_size;
    int       size;
    
#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_COLL)
    if (MPID_DebugFlag) {
	PTLS_Printf("PTLS_Scatter\n");
    }
#   endif

    size = comm->np;

    if ( (root>=size) || (root<0) ) {
	return MPIR_ERROR( comm, MPI_ERR_ROOT, "Invalid root in MPI_Scatter" );
    }

    if ( size == 1 ) {
	MPI_Type_size( sendtype->self, &size );
	memcpy( recvbuf,
		sendbuf,
		size*sendcount );
	return MPI_SUCCESS;
    }

    MPI_Type_size( recvtype->self, &recv_size   );
    totalbytes = recv_size * recvcount * comm->comm_coll->np;

    if ( comm->comm_coll->local_rank != root ) {
	if ( ((send_buf = (void *)PTLS_COLL_WORK_MALLOC( totalbytes) ) == 
	    (void *)NULL) && (totalbytes) ) {
	    perror( "malloc() failed" );
	    return MPI_ERR_INTERN;
	}
    } else {
	send_buf = (void *)sendbuf;
    }

    for (i=0; i<= comm->comm_coll->np; i++) {
	ptls_data_ptrs[i] = (char *)&(((char *)send_buf)[i*recv_size*recvcount]);
    }

    _scatter( ptls_data_ptrs,
	      1,
	      COLL_TAG( comm->comm_coll->send_context, 
                        comm->comm_coll->coll_cntr ),
	      (INT32 *)comm->comm_coll->lrank_to_grank,
	      1,
	      (INT32)comm->comm_coll->np,
	      (INT32)comm->comm_coll->local_rank,
	      (INT32)root,
	      (COLL_STRUCT *)&comm->comm_coll->coll_struct );

    if ( (recv_size * recvcount) > 0 ) {
	memcpy( recvbuf,
	        ptls_data_ptrs[comm->comm_coll->local_rank],
	        recv_size * recvcount );
    }

    return MPI_SUCCESS;

}

/***************************************************************************
  PTLS_Scatterv
  ***************************************************************************/
int PTLS_Scatterv(void* sendbuf, int *sendcounts, int *displs, struct MPIR_DATATYPE *sendtype, 
		  void* recvbuf, int recvcount, struct MPIR_DATATYPE *recvtype, int root, 
		  struct MPIR_COMMUNICATOR *comm)
{

    void     *send_buf;
    int       totalbytes,i;
    int       np = comm->comm_coll->np;
    int       my_rank = comm->comm_coll->local_rank;
    int       send_count,recv_size;
    int       size;
    int      *send_size;
    int      *send_counts;
    int      *send_info=NULL;
    int       send_info_size;

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_COLL)
    if (MPID_DebugFlag) {
	PTLS_Printf("PTLS_Scatterv\n");
    }
#   endif

    size = comm->np;

    if ( (root>=size) || (root<0) ) {
	return MPIR_ERROR( comm, MPI_ERR_ROOT, "Invalid root in MPI_SCATTERV" );
    }

    if ( size == 1 ) {
	MPI_Type_size( sendtype->self, &size );
	memcpy( recvbuf,
		&( ((char *)sendbuf)[size*displs[0]] ),
		size*sendcounts[0] );
	return MPI_SUCCESS;
    }

    send_info_size = (np+1)*sizeof(int);
    if ( ((send_info = (int *)PTLS_MALLOC( send_info_size ) ) == 
	(void *)NULL) && (send_info_size) ) {
	perror( "malloc() failed" );
	return MPI_ERR_INTERN;
    }
    send_size   = &send_info[0];
    send_counts = &send_info[1];
 
    if ( my_rank == root ) {
        MPI_Type_size( sendtype->self, send_size );
	memcpy( send_counts,
	        sendcounts,
		np*sizeof(int) );
    }
 
    /* broadcast the info significant only at root to everybody else */
    _bcast_short( (CHAR *)send_info,
                  (INT32)send_info_size,
		  COLL_TAG( comm->comm_coll->send_context, 
                            comm->comm_coll->coll_cntr ),
                  (INT32 *)comm->comm_coll->lrank_to_grank,
                  (INT32)1,
                  (INT32)np,
                  (INT32)my_rank,
                  (INT32)root,
                  (COLL_STRUCT *)&comm->comm_coll->coll_struct );

    send_count = 0;
    for (i=0;i<np;i++) {
	send_count += send_counts[i];
    }
    totalbytes = (send_count) * (*send_size);

    if ( ((send_buf = (void *)PTLS_COLL_WORK_MALLOC( totalbytes) ) == 
	(void *)NULL) && (totalbytes) ) {
	perror( "malloc() failed" );
	return MPI_ERR_INTERN;
    }

    ptls_data_ptrs[0] = (char *)send_buf;
    for ( i=0; i<np; i++ ) {
	ptls_data_ptrs[i+1] = (char *)( (PTRINT)(ptls_data_ptrs[i]) + 
			   (PTRINT)(send_counts[i]*(*send_size)) );
    }


    if ( my_rank == root ) {
	for ( i=0; i<np; i++ ) {
	    memcpy( ptls_data_ptrs[i],
		    &(((char *)sendbuf)[ (*send_size)*displs[i] ]),
		    (*send_size) * sendcounts[i] );
	}
    }


    _scatter( ptls_data_ptrs,
	      1,
	      COLL_TAG( comm->comm_coll->send_context, 
                        comm->comm_coll->coll_cntr ),
	      (INT32 *)comm->comm_coll->lrank_to_grank,
	      1,
	      (INT32)np,
	      (INT32)my_rank,
	      (INT32)root,
	      (COLL_STRUCT *)&comm->comm_coll->coll_struct );

    MPI_Type_size( recvtype->self, &recv_size );
    if ( (recv_size * recvcount) > 0 ) {
	memcpy( recvbuf,
	        ptls_data_ptrs[my_rank],
	        recv_size * recvcount );
    }

    if (send_info) {
	PTLS_FREE(send_info);
    }

    return MPI_SUCCESS;
  
}

/***************************************************************************
  PTLS_Allgather
  ***************************************************************************/
int PTLS_Allgather(void* sendbuf, int sendcount, struct MPIR_DATATYPE *sendtype, void* recvbuf, 
		   int recvcount, struct MPIR_DATATYPE *recvtype, struct MPIR_COMMUNICATOR *comm)
{

    int       totalbytes;
    int       err = MPI_SUCCESS;
    int       i,size;
    int       send_size, recv_size;

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_COLL)
    if (MPID_DebugFlag) {
	PTLS_Printf("PTLS_Allgather\n");
    }
#   endif

    size = comm->np;

    if ( size == 1 ) {
	MPI_Type_size( sendtype->self, &send_size );
	memcpy( recvbuf,
	        sendbuf,
	        send_size * sendcount );
	return MPI_SUCCESS;
    }

    MPI_Type_size( sendtype->self, &send_size );
    MPI_Type_size( recvtype->self, &recv_size );

    totalbytes = recv_size * recvcount * comm->comm_coll->np;

    for (i=0; i<= comm->comm_coll->np; i++) {
	ptls_data_ptrs[i] = &(((char *)recvbuf)[i*recv_size*recvcount]);
    }

    if ( (send_size * sendcount) > 0 ) {
	memcpy( ptls_data_ptrs[comm->comm_coll->local_rank],
	        sendbuf,
	        send_size * sendcount );
    }

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_COLL)
    if (MPID_DebugFlag) {
	PTLS_Printf("PTLS_AllGather-calling _collect_long()\n");
    }
#   endif

    _collect_long( ptls_data_ptrs,
		   1,
		   COLL_TAG( comm->comm_coll->send_context, 
                             comm->comm_coll->coll_cntr ),
		   (INT32 *)comm->comm_coll->lrank_to_grank,
		   1,
		   (INT32)comm->comm_coll->np,
		   (INT32)comm->comm_coll->local_rank,
		   (COLL_STRUCT *)&comm->comm_coll->coll_struct,
		   (BOOLEAN)0 );

    return err;
  
}

/***************************************************************************
  PTLS_Allgatherv
  ***************************************************************************/
int PTLS_Allgatherv(void* sendbuf, int sendcount, struct MPIR_DATATYPE *sendtype, void* recvbuf, 
		    int *recvcounts, int *displs, struct MPIR_DATATYPE *recvtype, struct MPIR_COMMUNICATOR *comm)
{

    char     *recv_buf;
    int       totalcounts,totalbytes;
    int       i,size;
    int       np      = comm->comm_coll->np;
    int       my_rank = comm->comm_coll->local_rank;
    int       err     = MPI_SUCCESS;
    int       recv_size,send_size;

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_COLL)
    if (MPID_DebugFlag) {
	PTLS_Printf("PTLS_Allgatherv\n");
    }
#   endif

    size = comm->np;

    if ( size == 1 ) {
	MPI_Type_size( recvtype->self, &recv_size );
	memcpy( &( ((char *)recvbuf)[ recv_size*displs[0] ]),
	       sendbuf,
	       recv_size * recvcounts[0] );
	return MPI_SUCCESS;
    }

    MPI_Type_size( recvtype->self, &recv_size );
    MPI_Type_size( sendtype->self, &send_size );

    totalcounts = 0;
    for (i=0; i<np; i++) {
	totalcounts += recvcounts[i];
    }
    totalbytes = totalcounts * recv_size;

    if ( ((recv_buf = (void *)PTLS_COLL_WORK_MALLOC( totalbytes) ) == 
	(void *)NULL) && (totalbytes) ) {
	perror( "malloc() failed" );
	return MPI_ERR_INTERN;
    }

    ptls_data_ptrs[0] = (char *)recv_buf;
    for ( i=0; i<np; i++ ) {
	ptls_data_ptrs[i+1] = (char *)(ptls_data_ptrs[i] + (recv_size * recvcounts[i]));
    }

    if ( (send_size * sendcount) > 0 ) {
	memcpy( ptls_data_ptrs[my_rank],
	       sendbuf,
	       send_size * sendcount );
    }

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_COLL)
    if (MPID_DebugFlag) {
	PTLS_Printf("PTLS_AllGather-calling _collect_long()\n");
    }
#   endif

    _collect_long( ptls_data_ptrs,
		   1,
		   COLL_TAG( comm->comm_coll->send_context, 
                             comm->comm_coll->coll_cntr ),
		   (INT32 *)comm->comm_coll->lrank_to_grank,
		   1,
		   (INT32)comm->comm_coll->np,
		   (INT32)comm->comm_coll->local_rank,
		   (COLL_STRUCT *)&comm->comm_coll->coll_struct,
		   (BOOLEAN)0 );

    for ( i=0; i<np; i++ ) {
	memcpy( &( ((char *)recvbuf)[ recv_size*displs[i] ] ),
	       ptls_data_ptrs[i],
	       recv_size * recvcounts[i] );
    }

    return err;
  
}

/***************************************************************************
  PTLS_Alltoall
  ***************************************************************************/
int PTLS_Alltoall(void* sendbuf, int sendcount, struct MPIR_DATATYPE *sendtype, void* recvbuf, 
		  int recvcount, struct MPIR_DATATYPE *recvtype, struct MPIR_COMMUNICATOR *comm)
{

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_COLL)
    if (MPID_DebugFlag) {
	PTLS_Printf("PTLS_Alltoall\n");
    }
#   endif

    /* Currently no ADI Alltoall operation available */
  
    return MPI_ERR_INTERN;

}

/***************************************************************************
  PTLS_Alltoallv
  ***************************************************************************/
int PTLS_Alltoallv(void* sendbuf, int *sendcounts, int *sdispls, struct MPIR_DATATYPE *sendtype, 
		   void* recvbuf, int *recvcounts, int *rdispls, struct MPIR_DATATYPE *recvtype, 
		   struct MPIR_COMMUNICATOR *comm)
{

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_COLL)
    if (MPID_DebugFlag) {
	PTLS_Printf("PTLS_Alltoallv\n");
    }
#   endif

    /* Currently no ADI Alltoall operation available */
  
    return MPI_ERR_INTERN;

}


/***************************************************************************
  PTLS_Reduce
  ***************************************************************************/
int PTLS_Reduce(void* sendbuf, void* recvbuf, int count, struct MPIR_DATATYPE *datatype, 
		MPI_Op op, int root, struct MPIR_COMMUNICATOR *comm)
{
    void     *work_buffer=NULL;
    void     *red_buffer=NULL;
    int       dsize;
    int       totalbytes;
    int       size;
    struct MPIR_OP *op_ptr = MPIR_GET_OP_PTR(op);

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_COLL)
    if (MPID_DebugFlag) {
	PTLS_Printf("PTLS_Reduce\n");
    }
#   endif

    size = comm->np;

    if ( (root>=size) || (root<0) ) {
	return MPIR_ERROR( comm, MPI_ERR_ROOT, "Invalid root in MPI_Reduce" );
    }

    if ( size == 1 ) {
	MPI_Type_size( datatype->self, &dsize );
	memcpy( recvbuf, sendbuf, dsize*count );
	return MPI_SUCCESS;
    }

    comm->comm_coll->coll_struct.user_struct = (VOID *)&datatype->self;
    MPI_Type_size( datatype->self, &dsize );

    totalbytes = dsize * count;

    if ( totalbytes > 0 ) {
	int bytes;
	bytes = 2*(totalbytes<sizeof(double) ? sizeof(double) : totalbytes);
	if ( ((work_buffer = (void *)PTLS_COLL_WORK_MALLOC( bytes )) == 
	    (void *)NULL) && (bytes) ) {
	    perror( "malloc() failed" );
	    return MPI_ERR_INTERN;
	}
	if ( comm->comm_coll->local_rank != root ) {
	    red_buffer = (void *)((PTRINT)(work_buffer) + 
				  (PTRINT)(bytes/2));
	} else {
	    red_buffer = recvbuf;
	}
    }


    if ( totalbytes > 0 ) {
	memcpy( red_buffer, sendbuf, totalbytes );
    }

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_COLL)
    if (MPID_DebugFlag) {
	PTLS_Printf("PTLS_Reduce-calling _reduce_short()\n");
    }
#   endif

    _reduce_short( (PUMA_OP)op_ptr->op,
		   (CHAR *)red_buffer,
		   (INT32)count,
		   (INT32)dsize,
		   COLL_TAG( comm->comm_coll->send_context, 
                             comm->comm_coll->coll_cntr ),
		   (INT32 *)comm->comm_coll->lrank_to_grank,
		   (INT32)1, 
		   (INT32)comm->comm_coll->np,
		   (INT32)comm->comm_coll->local_rank,
		   (INT32)root,
		   (CHAR *)work_buffer,
		   (COLL_STRUCT *)&comm->comm_coll->coll_struct );

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_COLL)
    if (MPID_DebugFlag) {
	PTLS_Printf("PTLS_Reduce-called _reduce_short()\n");
    }
#   endif

    return MPI_SUCCESS;

}

/***************************************************************************
  PTLS_Allreduce
  ***************************************************************************/
int PTLS_Allreduce(void* sendbuf, void* recvbuf, int count, struct MPIR_DATATYPE *datatype, 
		   MPI_Op op, struct MPIR_COMMUNICATOR *comm)
{
    CHAR           *work=(CHAR *)NULL;
    int             size;
    int             dsize;
    int             totalbytes;
    int             tag;
    struct MPIR_OP *op_ptr = MPIR_GET_OP_PTR(op);

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_COLL)
    if (MPID_DebugFlag) {
	PTLS_Printf("PTLS_Allreduce\n");
    }
#   endif

    comm->comm_coll->coll_struct.user_struct = (VOID *)&datatype->self;

    size = comm->np;

    MPI_Type_size( datatype->self, &dsize );

    totalbytes = dsize * count;
    if ( totalbytes > 0 ) {
	memcpy( recvbuf, sendbuf, totalbytes );
    }
    if (size==1) {
	return MPI_SUCCESS;
    }

    if ( ((work = (CHAR *)PTLS_COLL_WORK_MALLOC( totalbytes ) ) == 
	(CHAR *)NULL) && (totalbytes) ) {
	perror( "malloc() failed\n" );
	return MPI_ERR_INTERN;
    }

    tag = COLL_TAG( comm->comm_coll->send_context, 
		    comm->comm_coll->coll_cntr );

    if ( totalbytes<50000 ) { 
    /* if ( 0 ) { */ 

#       if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_COLL)
	if (MPID_DebugFlag) {
	    PTLS_Printf("PTLS_Allreduce-calling _all_reduce_short()\n");
	    PTLS_Printf("op_ptr->op = 0x%x\n",op_ptr->op);
	    PTLS_Printf("recvbuf    = 0x%x\n",recvbuf);
	    PTLS_Printf("count      = %d\n",count);
	    PTLS_Printf("dsize      = %d\n",dsize);
	    PTLS_Printf("tag        = %d\n",tag);
	    PTLS_Printf("lr2gr      = 0x%x\n",comm->comm_coll->lrank_to_grank);
	    PTLS_Printf("np         = %d\n",comm->comm_coll->np);
	    PTLS_Printf("lrank      = %d\n",comm->comm_coll->local_rank);
	    PTLS_Printf("work       = 0x%x\n",work);
	    PTLS_Printf("c_st       = 0x%x\n",&comm->comm_coll->coll_struct);
	}
#       endif
	
	if ( _all_reduce_short( (PUMA_OP)op_ptr->op,
			        (CHAR *)recvbuf,
			        (INT32)count,
			        (INT32)dsize,
			        (INT32)tag,
			        (INT32 *)comm->comm_coll->lrank_to_grank,
			        (INT32)1, 
			        (INT32)comm->comm_coll->np,
			        (INT32)comm->comm_coll->local_rank,
			        (CHAR *)work,
			        (COLL_STRUCT *)&comm->comm_coll->coll_struct ) ) {
	    return MPI_ERR_INTERN;
	}

#       if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_COLL)
	if (MPID_DebugFlag) {
	    PTLS_Printf("PTLS_Allreduce-called _all_reduce_short()\n");
	}
#       endif

    }
    else {

	/* I couldn't get this to work */
	
	FORM_DATA_PTRS( ptls_data_ptrs,
		        recvbuf,
		        totalbytes,
		        dsize,
		        comm->comm_coll->np );


#       if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_COLL)
	if (MPID_DebugFlag) {
	    PTLS_Printf("PTLS_Allreduce-calling _all_reduce_long()\n");
	}
#       endif
	
	_all_reduce_long( (PUMA_OP)op_ptr->op,
			  (CHAR **)ptls_data_ptrs,
			  (INT32)1,
			  (INT32)dsize,
			  (INT32)tag,
			  (INT32 *)comm->comm_coll->lrank_to_grank,
			  (INT32)1,
			  (INT32)comm->comm_coll->np,
			  (INT32)comm->comm_coll->local_rank,
			  (CHAR *)work,
			  (COLL_STRUCT *)&comm->comm_coll->coll_struct,
			  (BOOLEAN)0 );

    }

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_COLL)
    if (MPID_DebugFlag) {
	PTLS_Printf("leaving PTLS_Allreduce()\n");
    }
#   endif

    return MPI_SUCCESS;

}

/***************************************************************************
  PTLS_Reduce_scatter
  ***************************************************************************/
int PTLS_Reduce_scatter(void* sendbuf, void* recvbuf, int *recvcounts, 
			struct MPIR_DATATYPE *datatype, MPI_Op op, struct MPIR_COMMUNICATOR *comm)
{
    char     *work;
    void     *red_buffer;
    int       totalcount=0;
    int       bytes,totalbytes;
    int       i,np;
    int       size;
    int       dsize;
    struct MPIR_OP *op_ptr = MPIR_GET_OP_PTR(op);

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_COLL)
    if (MPID_DebugFlag) {
	PTLS_Printf("PTLS_Reduce_scatter\n");
    }
#   endif

    size = comm->np;

    if (size==1) {
	MPI_Type_size( datatype->self, &dsize   );
	memcpy( recvbuf,
	        sendbuf,
	        dsize*recvcounts[0] );
	return MPI_SUCCESS;
    }

    comm->comm_coll->coll_struct.user_struct = (VOID *)&datatype->self;
    MPI_Type_size( datatype->self, &dsize   );

    np = comm->comm_coll->np;
    for ( i=0; i<np; i++ ) {
	totalcount += recvcounts[i];
    }
    totalbytes = totalcount * dsize;

    bytes = 2*(totalbytes<sizeof(double) ? sizeof(double) : totalbytes);
    if ( ((work = (char *)PTLS_COLL_WORK_MALLOC( bytes ) )
	== (char *)NULL) && (bytes) ) {
	perror( "malloc() failed" );
	return MPI_ERR_INTERN;
    }
    red_buffer = (void *)((PTRINT)(work) + (PTRINT)(bytes/2));

    ptls_data_ptrs[i] = (char *)red_buffer;
    for ( i=0; i<np; i++ ) {
	ptls_data_ptrs[i+1] = (char *)ptls_data_ptrs[i] + (recvcounts[i]*dsize);
    }

    if ( totalbytes > 0 ) {
	memcpy( red_buffer,
	        sendbuf,
	        totalbytes );
    }


#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_COLL)
    if (MPID_DebugFlag) {
	PTLS_Printf("PTLS_Reduce_scatter-calling _dist_reduce_long()\n");
    }
#   endif

    _dist_reduce_long( (PUMA_OP)op_ptr->op,
		       (CHAR **)ptls_data_ptrs,
		       (INT32)1,
		       (INT32)dsize,
		       COLL_TAG( comm->comm_coll->send_context, 
                                 comm->comm_coll->coll_cntr ),
		       (INT32 *)comm->comm_coll->lrank_to_grank,
		       (INT32)1,
		       (INT32)np,
		       (INT32)comm->comm_coll->local_rank,
		       (CHAR *)work,
		       (COLL_STRUCT *)&comm->comm_coll->coll_struct,
		       (BOOLEAN)0 );

    if ( (recvcounts[comm->comm_coll->local_rank] * dsize) > 0 ) {
	memcpy( recvbuf,
		ptls_data_ptrs[comm->comm_coll->local_rank],
	        recvcounts[comm->comm_coll->local_rank]*dsize );

    }


    return MPI_SUCCESS;

}

/***************************************************************************
  PTLS_Scan
  ***************************************************************************/
int PTLS_Scan(void* sendbuf, void* recvbuf, int count, struct MPIR_DATATYPE *datatype, 
	      MPI_Op op, struct MPIR_COMMUNICATOR *comm )
{

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_COLL)
    if (MPID_DebugFlag) {
	PTLS_Printf("PTLS_Scan\n");
    }
#   endif

    /* Currently no ADI scan operation available */

    return MPI_ERR_INTERN;

}

/***************************************************************************
  When the Ptls native collective operations are active, the following 
  routines choose the MPICH default collective ops if any of the datatype
  arguments are non-contiguous.
  ***************************************************************************/

/***************************************************************************
  PTLS_Choose_Bcast
  ***************************************************************************/
int PTLS_Choose_Bcast( void* buffer, int count, struct MPIR_DATATYPE *datatype, int root, 
		      struct MPIR_COMMUNICATOR *comm )
{

    if (datatype->is_contig)
      return(PTLS_Bcast(buffer, count, datatype, root, comm));
    else
      return(default_collops->Bcast(buffer, count, datatype, root, comm));

}


/***************************************************************************
  PTLS_Choose_Gather
  ***************************************************************************/
int PTLS_Choose_Gather( void* sendbuf, int sendcount, struct MPIR_DATATYPE *sendtype, 
		       void* recvbuf, int recvcount, struct MPIR_DATATYPE *recvtype, int root, 
		       struct MPIR_COMMUNICATOR *comm) 
{

    if ( sendtype->is_contig && recvtype->is_contig ) 
      return(PTLS_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, 
			 recvtype, root, comm) );
    else
      return(default_collops->Gather( sendbuf, sendcount, sendtype, 
					recvbuf, recvcount, recvtype, root, comm) );
}


/***************************************************************************
  PTLS_Choose_Gatherv
  ***************************************************************************/
int PTLS_Choose_Gatherv(void* sendbuf, int sendcount, struct MPIR_DATATYPE *sendtype, 
			void* recvbuf, int *recvcounts, int *displs, 
			struct MPIR_DATATYPE *recvtype, int root, struct MPIR_COMMUNICATOR *comm)
{
    int rank;
    (void) MPIR_Comm_rank ( comm, &rank );
    if ( sendtype->is_contig && (rank != root || recvtype->is_contig) ) 
      return(PTLS_Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, 
			  displs, recvtype, root, comm));
    else
      return(default_collops->Gatherv(sendbuf, sendcount, sendtype, 
					 recvbuf, recvcounts, displs, recvtype, root, comm));

}


/***************************************************************************
  PTLS_Choose_Scatter
  ***************************************************************************/
int PTLS_Choose_Scatter(void* sendbuf, int sendcount, struct MPIR_DATATYPE *sendtype, 
			void* recvbuf, int recvcount, struct MPIR_DATATYPE *recvtype, 
			int root, struct MPIR_COMMUNICATOR *comm)
{
    if ( sendtype->is_contig && recvtype->is_contig ) 
      return(PTLS_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, 
			  recvtype, root, comm));
    else
      return(default_collops->Scatter(sendbuf, sendcount, sendtype, recvbuf, 
					 recvcount, recvtype, root, comm));

}


/***************************************************************************
  PTLS_Choose_Scatterv 
  ***************************************************************************/
int PTLS_Choose_Scatterv(void* sendbuf, int *sendcounts, int *displs, 
			 struct MPIR_DATATYPE *sendtype, void* recvbuf, int recvcount, 
			 struct MPIR_DATATYPE *recvtype, int root, struct MPIR_COMMUNICATOR *comm)
{
    int rank;
    (void) MPIR_Comm_rank ( comm, &rank );
    if ( (rank != root || sendtype->is_contig) && recvtype->is_contig ) 
      return(PTLS_Scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, 
			   recvcount, recvtype, root, comm));
    else
      return(default_collops->Scatterv(sendbuf, sendcounts, displs, sendtype, 
					  recvbuf, recvcount, recvtype, root, comm));

}


/***************************************************************************
  PTLS_Choose_Allgather
  ***************************************************************************/
int PTLS_Choose_Allgather(void* sendbuf, int sendcount, struct MPIR_DATATYPE *sendtype, 
			  void* recvbuf, int recvcount, struct MPIR_DATATYPE *recvtype, 
			  struct MPIR_COMMUNICATOR *comm)
{
    if ( sendtype->is_contig && recvtype->is_contig ) 
      return(PTLS_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, 
			    recvtype, comm));
    else
      return(default_collops->Allgather(sendbuf, sendcount, sendtype, recvbuf, 
					   recvcount, recvtype, comm));
}


/***************************************************************************
  PTLS_Choose_Allgatherv
  ***************************************************************************/
int PTLS_Choose_Allgatherv(void* sendbuf, int sendcount, struct MPIR_DATATYPE *sendtype, 
			   void* recvbuf, int *recvcounts, int *displs, 
			   struct MPIR_DATATYPE *recvtype, struct MPIR_COMMUNICATOR *comm)
{
    if ( sendtype->is_contig && recvtype->is_contig ) 
      return(PTLS_Allgatherv(sendbuf, sendcount, sendtype, recvbuf, 
			     recvcounts, displs, recvtype, comm));
    else
      return(default_collops->Allgatherv(sendbuf, sendcount, sendtype, recvbuf, 
					    recvcounts, displs, recvtype, comm));
}


/***************************************************************************
  PTLS_Choose_Reduce
  ***************************************************************************/
int PTLS_Choose_Reduce(void* sendbuf, void* recvbuf, int count, struct MPIR_DATATYPE *datatype, 
		       MPI_Op op, int root, struct MPIR_COMMUNICATOR *comm)
{
    struct MPIR_OP *op_ptr = MPIR_GET_OP_PTR(op);

    if ( datatype->is_contig && op_ptr->commute )
      return(PTLS_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm));
    else
      return(default_collops->Reduce(sendbuf, recvbuf, count, datatype, op, root, comm));
}


/***************************************************************************
  PTLS_Choose_Allreduce
  ***************************************************************************/
int PTLS_Choose_Allreduce(void* sendbuf, void* recvbuf, int count, 
			  struct MPIR_DATATYPE *datatype, MPI_Op op, 
			  struct MPIR_COMMUNICATOR *comm)
{
    struct MPIR_OP *op_ptr = MPIR_GET_OP_PTR(op);

    if ( datatype->is_contig && op_ptr->commute )
      return(PTLS_Allreduce(sendbuf, recvbuf, count, datatype, op, comm));
    else
      return(default_collops->Allreduce(sendbuf, recvbuf, count, datatype, op, comm));
}


/***************************************************************************
  PTLS_Choose_Reduce_scatter
  ***************************************************************************/
int PTLS_Choose_Reduce_scatter(void* sendbuf, void* recvbuf, int *recvcounts, 
			       struct MPIR_DATATYPE *datatype, MPI_Op op, 
			       struct MPIR_COMMUNICATOR *comm)
{
    struct MPIR_OP *op_ptr = MPIR_GET_OP_PTR(op);

/*
    if ( datatype->is_contig && op_ptr->commute )
      return(PTLS_Reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, 
				 comm));
    else
*/
      return(default_collops->Reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, 
						op, comm));

}

