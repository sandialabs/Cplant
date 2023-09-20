
#include<signal.h>
#include "mpiddev.h"

/***************************************************************************

  Global variables

  ***************************************************************************/

unsigned int      _mpi_p31_default_me_size = P31_DEFAULT_IRECV_MAX;

ptl_process_id_t  _mpi_p31_my_id;
ptl_id_t          _mpi_p31_my_size;

ptl_handle_ni_t   _mpi_p31_ni_handle;

ptl_pt_index_t    _mpi_p31_recv_portal = 5;
ptl_pt_index_t    _mpi_p31_read_portal = 6;
ptl_pt_index_t    _mpi_p31_ack_portal  = 7;

ptl_handle_me_t   _mpi_p31_no_match_me_handle;
mpi_unex_block    _mpi_p31_short_unex_block[P31_DEFAULT_NUM_UNEX_BLOCKS];
ptl_handle_me_t   _mpi_p31_long_me_handle;
ptl_handle_me_t   _mpi_p31_ack_me_handle;
ptl_handle_me_t   _mpi_p31_read_me_handle;

ptl_handle_eq_t   _mpi_p31_unex_eq_handle;

unsigned int      _mpi_p31_short_size = P31_DEFAULT_LONG_MSG;

eq_handle_list_t *_mpi_p31_eq_list_head = NULL;

mpi_unex_list_t  *_mpi_p31_unex_list_head = NULL;
mpi_unex_list_t  *_mpi_p31_unex_list_tail = NULL;
mpi_unex_list_t  *_mpi_p31_unex_list_free_head = NULL;

ptl_handle_md_t  _mpi_p31_ack_request_md_handle;

ptl_handle_me_t  _mpi_p31_dummy_me_handle;
ptl_handle_md_t  _mpi_p31_dummy_md_handle;

int              _mpi_p31_errno;

int              _mpi_p31_drop_count;

int              _mpi_unex_block_size;

/***************************************************************************

  MPID_Comm_init

 ***************************************************************************/
int MPID_CommInit( comm, newcomm )
MPI_Comm comm;
struct MPIR_COMMUNICATOR *newcomm;
{
    return MPI_SUCCESS;
}

/***************************************************************************

   MPID_Comm_free

 ***************************************************************************/
int MPID_CommFree( comm )
struct MPIR_COMMUNICATOR *comm;
{

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
	P31_Printf("entering MPID_CommFree()\n");
    }
#endif

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
	P31_Printf("leaving MPID_CommFree()\n");
    }
#endif

    return MPI_SUCCESS;
}

/***************************************************************************

  MPID_P31_Init

  Initialize all of the global structures the portals device uses

  ***************************************************************************/
int MPID_P31_init( int *argc, char **argv ) 
{
    ptl_process_id_t process_id;
    ptl_match_bits_t match_bits, ignore_bits;
    ptl_md_t         md;
    eq_handle_list_t *current;
    eq_handle_list_t *new;
    mpi_unex_list_t  *new_unex_ptr;
    mpi_unex_list_t  *cur_unex_ptr;
    int               i;
    unsigned int      unex_max = P31_DEFAULT_UNEX_MAX;
    unsigned int      long_max = P31_DEFAULT_LONG_MAX;
    char             *tmpstring;

#   if defined(MPID_DEBUG_ALL)
    if ( (tmpstring = getenv(MPI_ENV_DEBUG_STRING) ) ) {
	MPID_SetDebugFlag(1);
    }
#   endif

#   if defined(MPID_DEBUG_ALL) || defined(MPID_DEBUG_INIT) 
    if (MPID_DebugFlag) {
	P31_Printf("P31_Init\n");
    }
#   endif

#   if defined(MPID_DEBUG_ALL)
    if ( (tmpstring = getenv(MPI_ENV_DUMP_QUEUES_STRING)) ) {
	(void)signal(SIGUSR2, MPID_Dump_queues);
    }
#   endif

    /* get the max number of oustanding MPI_Irecv()'s */
    if ( (tmpstring = getenv(MPI_ENV_IRECV_MAX_STRING)) ) {
	_mpi_p31_default_me_size = atoi(tmpstring);
    }
    if ( _mpi_p31_default_me_size < P31_DEFAULT_IRECV_MAX ) {
	_mpi_p31_default_me_size = P31_DEFAULT_IRECV_MAX;
    }

    /* get the max number of oustanding long sends */
    if ( (tmpstring = getenv(MPI_ENV_LONG_MAX_STRING)) ) {
	long_max = atoi(tmpstring);
    }
    if ( long_max < P31_DEFAULT_LONG_MAX ) {
	long_max = P31_DEFAULT_LONG_MAX;
    }

    /* get the max number of unexpected messages */
    if ( (tmpstring = getenv(MPI_ENV_UNEX_MAX_STRING)) ) {
	unex_max = atoi(tmpstring);
    }
    if ( unex_max < P31_DEFAULT_UNEX_MAX ) {
	unex_max = P31_DEFAULT_UNEX_MAX;
    }

    /* get the size of an unexpected block */
    if ( (tmpstring = getenv(MPI_ENV_UNEX_BLOCK_SIZE_STRING)) ) {
	_mpi_unex_block_size = atoi(tmpstring);
    }
    if ( _mpi_unex_block_size < P31_DEFAULT_UNEX_BLOCK_SIZE ) {
	_mpi_unex_block_size = P31_DEFAULT_UNEX_BLOCK_SIZE;
    }

    /* initialize Portals library */
    P31_CALL( PtlInit(), PtlInit );

    /* initialize network interface */
    P31_CALL( PtlNIInit( PTL_IFACE_MYR,
			 8,
			 2,
			 &_mpi_p31_ni_handle ), PtlNIInit );

    /* get my rank and size of the job */
    P31_CALL( PtlGetId( _mpi_p31_ni_handle,
			&_mpi_p31_my_id,
			&_mpi_p31_my_size ), PtlGetId );

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P31_Printf("_mpi_p31_my_id   =>\n");
	P31_Printf("  addr_kind      = %d\n",_mpi_p31_my_id.addr_kind );
	P31_Printf("  nid            = %d\n",_mpi_p31_my_id.nid );
	P31_Printf("  pid            = %d\n",_mpi_p31_my_id.pid );
	P31_Printf("  gid            = %d\n",_mpi_p31_my_id.gid );
	P31_Printf("  rid            = %d\n",_mpi_p31_my_id.rid );
	P31_Printf("_mpi_p31_my_size = %d\n",_mpi_p31_my_size );
    }
#   endif
    
    MPID_MyWorldSize = _mpi_p31_my_size;
    MPID_MyWorldRank = _mpi_p31_my_id.rid;

    /* dump protocol info */
    if ( (tmpstring = getenv(MPI_ENV_INFO_STRING)) ) {
	if ( MPID_MyWorldRank == 0 ) {
	    fprintf(stderr,"IRECV_MAX = %d\n",_mpi_p31_default_me_size);
	    fprintf(stderr,"LONG_MAX  = %d\n",long_max);
	    fprintf(stderr,"UNEX_MAX  = %d\n",unex_max);
	    fprintf(stderr,"LONG_MSG  = %d\n",_mpi_p31_short_size);
	}
    }
	
#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P31_Printf("MPID_MyWorldSize = %d\n",MPID_MyWorldSize);
	P31_Printf("MPID_MyWorldRank = %d\n",MPID_MyWorldRank);
    }
#   endif

    process_id.addr_kind = PTL_ADDR_GID;
    process_id.gid       = 0;
    process_id.rid       = 0;

    match_bits           = 0xffffffffffffffff;
    ignore_bits          = 0;

    /* attach the no-match match entry */
    P31_CALL( PtlMEAttach( _mpi_p31_ni_handle,
			   _mpi_p31_recv_portal,
			   process_id,
			   match_bits,
			   ignore_bits,
			   PTL_RETAIN,
			   &_mpi_p31_no_match_me_handle ), PtlMEAttach );

    /* attach the first me to the ack portal */
    P31_CALL( PtlMEAttach( _mpi_p31_ni_handle,
			   _mpi_p31_ack_portal,
			   process_id,
			   match_bits,
			   ignore_bits,
			   PTL_RETAIN,
			   &_mpi_p31_ack_me_handle ), PtlMEAttach );

    /* attach the first me to the read portal */
    P31_CALL( PtlMEAttach( _mpi_p31_ni_handle,
			   _mpi_p31_read_portal,
			   process_id,
			   match_bits,
			   ignore_bits,
			   PTL_RETAIN,
			   &_mpi_p31_read_me_handle ), PtlMEAttach );

    /* insert the short protocol unexpected match entries */
    process_id.addr_kind = PTL_ADDR_GID;
    process_id.gid       = _mpi_p31_my_id.gid;
    process_id.rid       = PTL_ID_ANY;

    P31_SET_SHORT_UNEX_BITS( match_bits, ignore_bits );

    _mpi_p31_dummy_me_handle = _mpi_p31_no_match_me_handle;

    for( i=0; i<P31_DEFAULT_NUM_UNEX_BLOCKS; i++ ) {
	P31_CALL( PtlMEInsert( _mpi_p31_dummy_me_handle,
			       process_id,
			       match_bits,
			       ignore_bits,
			       PTL_UNLINK,
			       PTL_INS_AFTER,
			       &_mpi_p31_short_unex_block[i].me_handle ), PtlMEInsert );
	_mpi_p31_dummy_me_handle = _mpi_p31_short_unex_block[i].me_handle;
    }

    /* insert the long protocol unexpected match entry */
    P31_SET_LONG_UNEX_BITS( match_bits, ignore_bits );
    
    P31_CALL( PtlMEInsert( _mpi_p31_short_unex_block[i-1].me_handle,
			   process_id,
			   match_bits,
			   ignore_bits,
			   PTL_RETAIN,
			   PTL_INS_AFTER,
			   &_mpi_p31_long_me_handle ), PtlMEInsert );

    /* create an event queue for unexpected messages */
    P31_CALL( PtlEQAlloc( _mpi_p31_ni_handle,
			  (unex_max * 2 ),
			  &_mpi_p31_unex_eq_handle     ), PtlEQAlloc );

    /* allocate memory and attach memory descriptors for unexpected messages */
    for( i=0; i<P31_DEFAULT_NUM_UNEX_BLOCKS; i++ ) {

	if ( (_mpi_p31_short_unex_block[i].start = malloc( _mpi_unex_block_size )) == NULL ) {
	    fprintf(stderr,"malloc() failed\n");
	    return -1;
	}

	/* attach the short protocol unexpected message md's */
	
	/* fill in the md structure */
	md.start      = _mpi_p31_short_unex_block[i].start;
	md.length     = _mpi_unex_block_size;
	md.threshold  = PTL_MD_THRESH_INF;
	md.max_offset = _mpi_unex_block_size - _mpi_p31_short_size;
	md.options    = PTL_MD_OP_PUT | PTL_MD_ACK_DISABLE;
	md.user_ptr   = (void *)((long)i); 
	md.eventq     = _mpi_p31_unex_eq_handle;

	/* attach the new md to the match entry */
	P31_CALL( PtlMDAttach( _mpi_p31_short_unex_block[i].me_handle,
			       md,
			       PTL_UNLINK,
			       &_mpi_p31_short_unex_block[i].md_handle ), PtlMDAttach );

	_mpi_p31_short_unex_block[i].bytes_copied = 0;

    }

    /* fill in long protocol unexpected message md */
    md.start        = NULL;
    md.length       = 0;
    md.threshold    = PTL_MD_THRESH_INF;
    md.max_offset   = 0;
    md.options      = PTL_MD_OP_PUT | PTL_MD_TRUNCATE;
    md.user_ptr     = NULL;
    md.eventq       = _mpi_p31_unex_eq_handle;

    /* attach it to the long protocol unexpected me */
    P31_CALL( PtlMDAttach( _mpi_p31_long_me_handle,
			   md,
			   PTL_RETAIN,
			   &_mpi_p31_dummy_md_handle ), PtlMDAttach );

    /* allocate a linked list of event queue handles for posted receives and long sends */
    for ( i=0; i<(_mpi_p31_default_me_size + long_max); i++ ) {

	if ( (new = malloc( sizeof(eq_handle_list_t) )) == NULL ) {
	    fprintf(stderr,"malloc() failed\n");
	    return -1;
	}

	if ( _mpi_p31_eq_list_head == NULL ) {
	    _mpi_p31_eq_list_head = new;
	    new->prev             = NULL;
	} else {
	    current->next = new;
	    new->prev     = current;
	}
	new->next = NULL;
	current   = new;

	P31_CALL( PtlEQAlloc( _mpi_p31_ni_handle,
			      P31_MAX_EVENTS,
			      &current->eq_handle ), PtlEQAlloc );
    }

    /* create list of unexpected messages */
    for ( i=0; i<unex_max*2; i++ ) {
	
	if ( (new_unex_ptr = malloc( sizeof(mpi_unex_list_t) )) == NULL ) {
	    fprintf(stderr,"malloc() failed\n");
	    return -1;
	}

	if ( _mpi_p31_unex_list_free_head == NULL ) {
	    _mpi_p31_unex_list_free_head = new_unex_ptr;
	    cur_unex_ptr                 = new_unex_ptr;
	    cur_unex_ptr->prev           = NULL;
	}else {
	    new_unex_ptr->prev           = cur_unex_ptr;
	    cur_unex_ptr->next           = new_unex_ptr;
	    cur_unex_ptr                 = new_unex_ptr;
	}
	cur_unex_ptr->next = NULL;
    }

    /* get a free-floating md for sending a sync ack */
    md.start        = NULL;
    md.length       = 0;
    md.threshold    = PTL_MD_THRESH_INF;
    md.max_offset   = 0;
    md.options      = 0;
    md.user_ptr     = NULL;
    md.eventq       = PTL_EQ_NONE;

    P31_CALL( PtlMDBind( _mpi_p31_ni_handle,
			 md,
			 &_mpi_p31_ack_request_md_handle ), PtlMDBind );

    /* wait for everybody else */
    P31_CALL( PtlNIBarrier( _mpi_p31_ni_handle ), PtlNIBarrier );

    /* get drop count */
    P31_CALL( PtlNIStatus( _mpi_p31_ni_handle,
			    PTL_SR_DROP_COUNT,
			    &_mpi_p31_drop_count ), PtlNIStatus );

    return MPI_SUCCESS;

}

/***************************************************************************

  MPID_P31_Finalize

  cleanup 

  ***************************************************************************/
void MPID_P31_finalize()
{
    eq_handle_list_t *cur_eq_list_ptr, *next_eq_list_ptr;
    mpi_unex_list_t  *cur_unex_list_ptr, *next_unex_list_ptr;
    int               i;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
	P31_Printf("entering P31_Finalize()\n");
    }
#endif

    /* wait for everybody else */
    P31_CALL( PtlNIBarrier( _mpi_p31_ni_handle ), PtlNIBarrier );

    for( i=0; i<P31_DEFAULT_NUM_UNEX_BLOCKS; i++ ) {
	free( _mpi_p31_short_unex_block[i].start );
    }

    cur_eq_list_ptr = _mpi_p31_eq_list_head;
    while ( cur_eq_list_ptr ) {
	PtlEQFree( cur_eq_list_ptr->eq_handle );
	next_eq_list_ptr = cur_eq_list_ptr->next;
	free( cur_eq_list_ptr );
	cur_eq_list_ptr = next_eq_list_ptr;
    }

    cur_unex_list_ptr = _mpi_p31_unex_list_free_head;
    while ( cur_unex_list_ptr ) {
	next_unex_list_ptr = cur_unex_list_ptr->next;
	free( cur_unex_list_ptr );
	cur_unex_list_ptr = next_unex_list_ptr;
    }

    /* need to unlink everything too */
#ifdef MPI_IS_THE_ONLY_USER_OF_PORTALS_3
    P31_CALL( PtlNIFini( _mpi_p31_ni_handle ), PtlNIFini );

    PtlFini();
#endif

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
	P31_Printf("leaving P31_Finalize()\n");
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
    sprintf(name,"%d",_mpi_p31_my_id.nid);
}


/***************************************************************************

  MPID_P31_Wtick

  ***************************************************************************/
/* 
   This returns a value that is correct but not the best value that
   could be returned.
   It makes several separate stabs at computing the tickvalue.
*/
void MPID_P31_Wtick( tick )
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


/***************************************************************************

  MPID_P31_Get_eq_handle

  ***************************************************************************/
eq_handle_list_t *MPID_P31_Get_eq_handle()
{
    ptl_event_t       event;
    eq_handle_list_t *ptr = NULL;

    if ( _mpi_p31_eq_list_head != NULL ) {
	/* get a free event queue handle */
	ptr                   = _mpi_p31_eq_list_head;
	_mpi_p31_eq_list_head = _mpi_p31_eq_list_head->next;
    } else {
	/* all gone -- allocate one */
	if ( (ptr = malloc( sizeof(eq_handle_list_t) )) == NULL ) {
	    return NULL;
	}
	P31_CALL( PtlEQAlloc( _mpi_p31_ni_handle,
			      P31_MAX_EVENTS,
			      &ptr->eq_handle ), PtlEQAlloc );
    }
    ptr->next = NULL;
    ptr->prev = NULL;

    /* clear the event queue */
    while ( PtlEQGet( ptr->eq_handle, &event ) != PTL_EQ_EMPTY );

    return ptr;

}


/***************************************************************************

  MPID_P31_Free_eq_handle

  ***************************************************************************/
void MPID_P31_Free_eq_handle( eq_handle_list_t *ptr )
{

    /* add the eq handle back into the list */
    ptr->next                   = _mpi_p31_eq_list_head;
    _mpi_p31_eq_list_head       = ptr;
    _mpi_p31_eq_list_head->prev = NULL;

}

/***************************************************************************

  MPID_P31_Get_unex

  ***************************************************************************/
mpi_unex_list_t *MPID_P31_Get_unex( void )
{
    mpi_unex_list_t *ptr;
    
    /* get a free one */
    if ( (ptr = _mpi_p31_unex_list_free_head ) == NULL ) {

	if ( (ptr = malloc( sizeof(mpi_unex_list_t) )) == NULL ) {
	    return NULL;
	}
	
    } else {
	_mpi_p31_unex_list_free_head = _mpi_p31_unex_list_free_head->next;
    }
    ptr->next                    = NULL;

    /* add it to unex list */
    if ( _mpi_p31_unex_list_head == NULL ) {
	_mpi_p31_unex_list_head = ptr;
	_mpi_p31_unex_list_tail = ptr;
	ptr->prev               = NULL;
    } else {
	_mpi_p31_unex_list_tail->next = ptr;
	ptr->prev                     = _mpi_p31_unex_list_tail;
	_mpi_p31_unex_list_tail       = ptr;
    }

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P31_Printf("  allocating unexpected entry %p\n",ptr);
    }
#   endif
    
    return ptr;

}


/***************************************************************************

  MPID_P31_Free_unex

  ***************************************************************************/
void MPID_P31_Free_unex( mpi_unex_list_t *ptr )
{

    /* unlink it from the unexepected list */
    if ( ptr->prev ) {
	ptr->prev->next = ptr->next;
    }
    
    if ( ptr->next ) {
	ptr->next->prev = ptr->prev;
    }

    if ( ptr == _mpi_p31_unex_list_head ) {
	_mpi_p31_unex_list_head = ptr->next;
    }

    if ( ptr == _mpi_p31_unex_list_tail ) {
	_mpi_p31_unex_list_tail = ptr->prev;
    }

    /* add it to the free list */
    ptr->prev                    = NULL;
    ptr->next                    = _mpi_p31_unex_list_free_head;
    _mpi_p31_unex_list_free_head = ptr;

#   if defined(MPID_DEBUG_ALL)
    if ( MPID_DebugFlag ) {
	P31_Printf("  freeing unexpected entry %p\n",ptr);
    }
#   endif

}
