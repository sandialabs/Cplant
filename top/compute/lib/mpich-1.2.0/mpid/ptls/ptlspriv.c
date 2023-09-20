
#include "ptlsdev.h"


/***************************************************************************

  Global variables

  ***************************************************************************/
int                ptls_myid;
int                ptls_nnodes;
int                ptls_gid;

int                ptls_match_list_size=0;
int                ptls_heap_size=0;
int                ptls_short_msg_size=0;
int                ptls_read_match_list_size=0;

PORTAL_INDEX       ptls_recv_portal;
PORTAL_INDEX       ptls_ack_portal;
PORTAL_INDEX       ptls_read_portal;

PORTAL_DESCRIPTOR *ack_ptl_desc;

MATCH_DESC_TYPE   *recv_desc_list;
MATCH_DESC_TYPE   *ack_desc_list;
MATCH_DESC_TYPE   *read_desc_list;

MATCH_DESC_TYPE   *recv_head_desc;
MATCH_DESC_TYPE   *ack_head_desc;
MATCH_DESC_TYPE   *read_head_desc;

MATCH_DESC_TYPE   *recv_short_catchall_desc;
MATCH_DESC_TYPE   *recv_long_catchall_desc;

UINT32             recv_short_catchall_match_index;

MATCH_DESC_TYPE   *recv_last_reg_desc;
int                recv_last_reg_index;

int               *recv_free_list,
recv_first_free;
int               *ack_free_list,
ack_first_free;
int               *read_free_list,
read_first_free;

int               *recv_previous_list;

IND_MD_BUF_DESC   *ack_ind_bufs;
IND_MD_BUF_DESC   *recv_ind_bufs;

void              *dynamic_heap;

COLL_STRUCT        g_coll_struct;

/***************************************************************************

  MPID_PTLS_Init

  Initialize all of the global structures the portals device uses

  ***************************************************************************/
int MPID_PTLS_init( int *argc, char **argv ) 
{
    CHAMELEON  ignorebits;
    CHAMELEON  matchbits;
    int        i,j;
    char      *tmpstring;

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_INIT) 
    if (MPID_DebugFlag) {
	PTLS_Printf("PTLS_Init\n");
    }
#   endif
    
    MPID_MyWorldSize = _my_nnodes;
    MPID_MyWorldRank = _my_rank;

    _coll_struct_init( &g_coll_struct );

    default_collops    = MPIR_intra_collops;
    MPIR_intra_collops = &ptls_collops;

    /* Set various default values */

#   if defined(GET_TIMES)
    if ( (tmpstring = getenv(MPI_ENV_PRINT_STATS_STRING)) )
      ptls_print_stats = atoi( tmpstring );
    else
      ptls_print_stats = 0;
#   endif GET_TIMES

    if ( (tmpstring = getenv(MPI_ENV_MATCH_LIST_SIZE_STRING)) )
      ptls_match_list_size = atoi( tmpstring );
    if ( ptls_match_list_size < PTLS_DEFAULT_MATCH_LIST_SIZE )
      ptls_match_list_size = PTLS_DEFAULT_MATCH_LIST_SIZE;

    if ( (tmpstring = getenv(MPI_ENV_HEAP_SIZE_STRING)) )
      ptls_heap_size = atoi( tmpstring );
    if ( ptls_heap_size < PTLS_DEFAULT_HEAP_SIZE )
      ptls_heap_size = PTLS_DEFAULT_HEAP_SIZE;

    if ( (tmpstring = getenv(MPI_ENV_SHORT_MSG_SIZE_STRING)) )
      ptls_short_msg_size = atoi( tmpstring );
    if ( ptls_short_msg_size < PTLS_DEFAULT_SHORT_MSG_SIZE )
      ptls_short_msg_size = PTLS_DEFAULT_SHORT_MSG_SIZE;
    
    if ( (tmpstring = getenv(MPI_ENV_COLL_WORK_SIZE_STRING)) )
      ptls_coll_work_size = atoi( tmpstring );
    if ( ptls_coll_work_size < PTLS_DEFAULT_COLL_WORK_SIZE )
      ptls_coll_work_size = PTLS_DEFAULT_COLL_WORK_SIZE;

    if ( (tmpstring = getenv(MPI_ENV_DEFAULT_COLL_OPS_STRING)) )
      MPIR_intra_collops = default_collops;

#   if defined(MPID_DEBUG_ALL)

    if ( (tmpstring = getenv(MPI_ENV_DEBUG_STRING) ) ) {
	MPID_SetDebugFlag(1);
    }

#   endif
    

#   if defined(GET_TIMES)
    send_sync_short_count                               =
      send_sync_long_count                              =
	send_ready_count                                =
	  send_short_count                              =
	    send_long_count                             = 
	      recv_sync_ack_count                       =
		recv_pull_msg_count                     =
		  recv_copy_msg_count                   =
		    recv_post_count                     =
		      recv_heap_count                   =
			comp_recv_count                 =
			  test_recv_count               = 
			    comp_recv_probe_count       =
			      test_recv_probe_count     = 0;
    						        
    send_sync_short                                     =
      send_sync_long         			        =   
	send_ready                                      =
	  send_short                                    =
	    send_long                                   =
	      comp_send                                 =
		test_send                               =
		  recv_sync_ack                         =
		    recv_pull_msg                       =
		      recv_copy_msg                     =
			recv_post                       =
			  recv_heap                     =
			    comp_recv                   =
			      test_recv                 =
				comp_recv_probe         =
				  test_recv_probe       = 0.0;

#   endif

    /* These are needed for some collective ops */
    if ( ( ptls_coll_work = (VOID *)PTLS_MALLOC( ptls_coll_work_size ) ) == (VOID *)NULL ) {
	perror( "malloc() failed" );
	return MPI_ERR_INTERN;
    }
	
    if ( (ptls_data_ptrs = (CHAR **)PTLS_MALLOC( (_my_nnodes+1) * sizeof( CHAR * ) ))
	== (CHAR **)NULL ) {
	PTLS_MALLOC_ERROR( (_my_nnodes+1) * sizeof(CHAR *) );
	exit(1);
    }


    /* These are the ones provided by the ADI */
    ptls_collops.Barrier        = PTLS_Barrier;
    ptls_collops.Bcast          = PTLS_Choose_Bcast;
    ptls_collops.Gather         = PTLS_Choose_Gather;
    ptls_collops.Gatherv        = PTLS_Choose_Gatherv;
    ptls_collops.Scatter        = PTLS_Choose_Scatter;
    ptls_collops.Scatterv       = PTLS_Choose_Scatterv;
    ptls_collops.Allgather      = PTLS_Choose_Allgather;
    ptls_collops.Allgatherv     = PTLS_Choose_Allgatherv;
    ptls_collops.Reduce         = PTLS_Choose_Reduce;
    ptls_collops.Allreduce      = PTLS_Choose_Allreduce;
    ptls_collops.Reduce_scatter = PTLS_Choose_Reduce_scatter;
      
    /* These use the default MPICH ops (for now) */
    ptls_collops.Alltoall       = default_collops->Alltoall;
    ptls_collops.Alltoallv      = default_collops->Alltoallv;
    ptls_collops.Scan           = default_collops->Scan;

    /* Set world rank and size */
    ptls_myid            = _my_rank;
    ptls_nnodes          = _my_nnodes;

    /* Set group id */
    ptls_gid             = _my_gid;

    /* Get portals */
    /*
    sptl_c_alloc( &ptls_recv_portal, NULL, _my_nnodes, _my_rank );
    sptl_c_alloc( &ptls_ack_portal,  NULL, _my_nnodes, _my_rank );
    sptl_c_alloc( &ptls_read_portal, NULL, _my_nnodes, _my_rank );
    */
    ptls_recv_portal = 14;
    ptls_ack_portal  = 24;
    ptls_read_portal = 34;

    /* for now */
    ptls_read_match_list_size   = ptls_match_list_size;

    /* Set up the portals */
    INIT_ML( ptls_recv_portal, recv_desc_list, 
	     recv_head_desc, (ptls_match_list_size+3));

    INIT_ML( ptls_ack_portal, ack_desc_list,
	     ack_head_desc, (ptls_match_list_size+1));

    INIT_ML( ptls_read_portal, read_desc_list,
	     read_head_desc, (ptls_read_match_list_size+1));

    /* initialize the recv mle's */
    for ( i=0; i<ptls_match_list_size+3; i++ ) 
      recv_desc_list[i].next               =
	recv_desc_list[i].next_on_nofit    =
	  recv_desc_list[i].next_on_nobuf  = -1;



    /* Initialize all of the ack mle's, order doesn't matter */
    for ( i=0; i<ptls_match_list_size+1; i++ ) {
	ack_desc_list[i].next              = 
	  ack_desc_list[i].next_on_nofit   = 
	    ack_desc_list[i].next_on_nobuf = i + 1;
    }
    i--;
    ack_desc_list[i].next              = 
      ack_desc_list[i].next_on_nofit   = 
	ack_desc_list[i].next_on_nobuf = 0;

    /* Initialize all of the read mle's, order doesn't matter */
    for ( i=0; i<ptls_read_match_list_size+1; i++ ) {
	read_desc_list[i].next              = 
	  read_desc_list[i].next_on_nofit   = 
	    read_desc_list[i].next_on_nobuf = i + 1;
    }
    i--;
    read_desc_list[i].next              = 
      read_desc_list[i].next_on_nofit   = 
	read_desc_list[i].next_on_nobuf = 0;

    /* Array of free ack mle's */
    if ( (ack_free_list = (int *)PTLS_MALLOC( ptls_match_list_size * sizeof(int))) 
	== (int *)NULL ) {
	PTLS_MALLOC_ERROR( ptls_match_list_size * sizeof(int) );
	exit(1);
    }

    /* Initialize free ack mle list */
    for ( i=0,j=ptls_match_list_size; j>0; i++,j--)
      ack_free_list[i] = j;

    /* Initialize the first free ack mle */
    ack_first_free = ptls_match_list_size - 1;

    /* Array of free recv mle's */
    if ( (recv_free_list = (int *)PTLS_MALLOC( ptls_match_list_size * sizeof(int) ) )
	== (int *)NULL ) {
	PTLS_MALLOC_ERROR( ptls_match_list_size * sizeof(int) );
	exit(1);
    }

    /* Initialize free recv mle list */
    for ( i=0,j=ptls_match_list_size; j>0; i++,j-- )
      recv_free_list[i] = j;

    /* Initialize the first free recv mle */
    recv_first_free = ptls_match_list_size - 1;

    /* Array of free read mle's */
    if ( (read_free_list = (int *)PTLS_MALLOC( ptls_read_match_list_size * sizeof(int) ) )
	== (int *)NULL ) {
	PTLS_MALLOC_ERROR( ptls_read_match_list_size * sizeof(int) );
	exit(1);
    }

    /* Initialize free read mle list */
    for ( i=0,j=ptls_read_match_list_size; j>0; i++,j--)
      read_free_list[i] = j;

    /* Initialize the first free read mle */
    read_first_free = ptls_read_match_list_size - 1;

    /* Array of previous recv mle's */
    if ( (recv_previous_list = (int *)PTLS_MALLOC( ptls_match_list_size * sizeof(int) ) )
	== (int *)NULL ) {
	PTLS_MALLOC_ERROR( ptls_match_list_size * sizeof(int) );
	exit(1);
    }

    /* Initialize the previous recv mle list */
    for ( i=0; i<ptls_match_list_size; i++ )
      recv_previous_list[i] = 0;

    /* Set up the recv mle catchalls */
    recv_short_catchall_match_index = ptls_match_list_size + 1;
    recv_short_catchall_desc        = &recv_desc_list[ ptls_match_list_size + 1 ];
    recv_long_catchall_desc         = &recv_desc_list[ ptls_match_list_size + 2 ];

    recv_short_catchall_desc->ctl_bits |= MCH_NOT_ACTIVE;

    PTLS_SHORT_OVERFLOW_MATCHBITS( recv_short_catchall_desc->must_mbits, 
				   recv_short_catchall_desc->ign_mbits   );

    recv_short_catchall_desc->next              = 
      recv_short_catchall_desc->next_on_nobuf   =
	recv_short_catchall_desc->next_on_nofit = ptls_match_list_size + 2;

    recv_long_catchall_desc->ctl_bits |= MCH_NOT_ACTIVE;

    PTLS_LONG_OVERFLOW_MATCHBITS( recv_long_catchall_desc->must_mbits, 
				 recv_long_catchall_desc->ign_mbits     );

    recv_long_catchall_desc->next              = 
      recv_long_catchall_desc->next_on_nobuf   =
	recv_long_catchall_desc->next_on_nofit = -1;

    recv_head_desc->next = ptls_match_list_size + 1;
    recv_last_reg_desc   = recv_head_desc;

    recv_last_reg_index  = 0;


    /* set up a dynamic md for the short overflow */
    recv_short_catchall_desc->mem_op = DYN_SV_HDR_BDY;

    if ( (dynamic_heap = (void *)PTLS_MALLOC( ptls_heap_size )) == (void *)NULL ) {
	PTLS_MALLOC_ERROR( ptls_heap_size );
	exit(1);
    }
    
    if ( sptl_dyn_init( &recv_short_catchall_desc->mem_desc.dyn,
		       dynamic_heap,
		       ptls_heap_size ) != 0 ) {
	fprintf(stderr,"ERROR: sptl_dyn_init() failed\n");
	exit(1);
    }

    /* Activate it */
    recv_short_catchall_desc->ctl_bits = MCH_OK;

    /* overlay the dynamic md on the long catchall */
    recv_long_catchall_desc->mem_op = DYN_SV_HDR_ACK;

    if ( sptl_dyn_ovrlay( &recv_long_catchall_desc->mem_desc.dyn, 
			 &recv_short_catchall_desc->mem_desc.dyn ) != 0 ) {
	fprintf(stderr,"ERROR: sptl_dyn_ovrlay() failed\n");
	exit(1);
    }

    /* Activate it */
    recv_long_catchall_desc->ctl_bits = MCH_OK;

    /*Allocate some ind buffer descriptors for recv mle's */
    if ( ( recv_ind_bufs = (IND_MD_BUF_DESC *)PTLS_MALLOC((ptls_match_list_size+1) *
							  sizeof(IND_MD_BUF_DESC))) ==
	(IND_MD_BUF_DESC *)NULL ) {
	PTLS_MALLOC_ERROR( (ptls_match_list_size+1) * sizeof(IND_MD_BUF_DESC) );
	exit(1);
    }

    /* Initialize them */
    for ( i=0; i<ptls_match_list_size+1; i++ ) {
	recv_ind_bufs[i].buf     = (CHAR *)NULL;
	recv_ind_bufs[i].buf_len = 0;
    }

    /* attach the buffers to the recv mle's */
    for ( i=0; i<ptls_match_list_size+1; i++ )
      recv_desc_list[i].mem_desc.ind.buf_desc_table = &recv_ind_bufs[i];

    /* Allocate some ind buffer descriptors for the ack portal */
    if ( ( ack_ind_bufs = (IND_MD_BUF_DESC *)PTLS_MALLOC((ptls_match_list_size+1) *
							 sizeof(IND_MD_BUF_DESC))) ==
	(IND_MD_BUF_DESC *)NULL ) {
	PTLS_MALLOC_ERROR( ptls_match_list_size * sizeof(IND_MD_BUF_DESC) );
	exit(1);
    }
    
    /* Initialize them */
    for ( i=1; i<ptls_match_list_size+1; i++ ) {
	ack_ind_bufs[i].buf     = (CHAR *)NULL;
	ack_ind_bufs[i].buf_len = 0;
    }

    /* attach the buffers to the ack mle's */
    for ( i=1; i<ptls_match_list_size+1; i++ )
      ack_desc_list[i].mem_desc.ind.buf_desc_table = &ack_ind_bufs[i];

    /* lock down the independent buffer descriptors into which the kernel will w
rite
    ** header information.
    */
    if ( portal_lock_buffer( recv_ind_bufs,
                             ((ptls_match_list_size+1)*sizeof(IND_MD_BUF_DESC)) ) < 0 ) {
        fprintf(stderr,"%s:%d portal_lock_buffer() failed\n",__FILE__,__LINE__);
    }

    if ( portal_lock_buffer( ack_ind_bufs,
                             (ptls_match_list_size*sizeof(IND_MD_BUF_DESC)) ) < 0 ) {
        fprintf(stderr,"%s:%d portal_lock_buffer() failed\n",__FILE__,__LINE__);
    }

    if ( portal_lock_buffer( dynamic_heap, ptls_heap_size ) < 0 ) {
        fprintf(stderr,"%s:%d portal_lock_buffer() failed\n",__FILE__,__LINE__);
    }

    if ( _barrier( -1, NULL, _my_nnodes, _my_rank, &g_coll_struct ) < 0 ) {
	fprintf(stderr,"_barrier() failed\n");
    }

    return MPI_SUCCESS;

}

/***************************************************************************

  MPID_PTLS_Finalize

  cleanup 

  ***************************************************************************/
void MPID_PTLS_finalize()
{


#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
	PTLS_Printf("entering PTLS_Finalize()\n");
    }
#endif

    _barrier( -1, NULL, _my_nnodes, _my_rank, &g_coll_struct );

    PTLS_FREE( recv_free_list ); 
    
    PTLS_FREE( read_free_list );

    PTLS_FREE( recv_previous_list );

    if ( portal_unlock_buffer( recv_ind_bufs,
                             ((ptls_match_list_size+1)*sizeof(IND_MD_BUF_DESC)) ) < 0 ) {
        fprintf(stderr,"%s:%d portal_unlock_buffer() failed\n",__FILE__,__LINE__);
    }

    if ( portal_unlock_buffer( ack_ind_bufs,
                             (ptls_match_list_size*sizeof(IND_MD_BUF_DESC)) ) < 0 ) {
        fprintf(stderr,"%s:%d portal_unlock_buffer() failed\n",__FILE__,__LINE__
);
    }

    if ( portal_unlock_buffer( dynamic_heap, ptls_heap_size ) < 0 ) {
        fprintf(stderr,"%s:%d portal_unlock_buffer() failed\n",__FILE__,__LINE__);
    }

    PTLS_FREE( dynamic_heap );

    PTLS_FREE( recv_ind_bufs );

    PTLS_FREE( ack_ind_bufs );

    PTLS_FREE( ptls_data_ptrs );

    PTLS_FREE( ack_free_list );

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
	PTLS_Printf("leaving PTLS_Finalize()\n");
    }
#endif

}

/***************************************************************************

  MPID_Node_name

  ***************************************************************************/
void MPID_Node_name( name, nlen )
char *name;
int  nlen;
{
    /* the ZOLTAN people wanted MPI to export the physical node id */
    sprintf(name,"%d",_my_pnid);
}


/***************************************************************************

  MPID_PTLS_Wtick

  ***************************************************************************/
/* 
   This returns a value that is correct but not the best value that
   could be returned.
   It makes several separate stabs at computing the tickvalue.
*/
void MPID_PTLS_Wtick( tick )
double *tick;
{
    static double tickval = -1.0;
    double t1, t2;
    int    cnt;
    int    icnt;

    if (tickval < 0.0) {
	tickval = 1.0e6;
	for (icnt=0; icnt<10; icnt++) {
	    cnt = 1000;
	    MPID_Wtime( &t1 );
	    while (cnt--) {
		MPID_Wtime( &t2 );
		if (t2 > t1) break;
		}
	    if (cnt && t2 - t1 < tickval)
		tickval = t2 - t1;
	}
    }
    *tick = tickval;
}

