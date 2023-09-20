#include "mpiddev.h"

FILE *MPID_DEBUG_FILE = 0;
FILE *MPID_TRACE_FILE = 0;
int   MPID_DebugFlag  = 0;

void MPID_SetDebugFile( name )
char *name;
{
    char filename[1024];
    
    if (strcmp( name, "-" ) == 0) {
	MPID_DEBUG_FILE = stdout;
	return;
    }
    if (strchr( name, '%' )) {
	sprintf( filename, name, MPID_MyWorldRank );
	MPID_DEBUG_FILE = fopen( filename, "w" );
    }
    else
	MPID_DEBUG_FILE = fopen( name, "w" );

    if (!MPID_DEBUG_FILE) MPID_DEBUG_FILE = stdout;
}

void MPID_SetTraceFile( name )
char *name;
{
    char filename[1024];

    if (strcmp( name, "-" ) == 0) {
	MPID_TRACE_FILE = stdout;
	return;
    }
    if (strchr( name, '%' )) {
	sprintf( filename, name, MPID_MyWorldRank );
	MPID_TRACE_FILE = fopen( filename, "w" );
    }
    else
	MPID_TRACE_FILE = fopen( name, "w" );

    /* Is this the correct thing to do? */
    if (!MPID_TRACE_FILE)
	MPID_TRACE_FILE = stdout;
}

void MPID_SetDebugFlag( f )
int f;
{
    MPID_DebugFlag = f;
}

/* for FORTRAN */
void mpid_setdebugflag__()
{
    MPID_DebugFlag = 1;
}

void mpid_unsetdebugflag()
{
    MPID_DebugFlag = 0;
}

void P30_Printf( char *format, ... )
{
    va_list args;

    if ( ! MPID_DEBUG_FILE )
	MPID_DEBUG_FILE = stdout;

    fprintf(MPID_DEBUG_FILE,"%d: ",MPID_MyWorldRank);

    va_start(args,format);
    vfprintf(MPID_DEBUG_FILE,format,args);
    va_end(args);
}

void MPID_Dump_queues( int dummy )
{
    mpi_unex_list_t *ptr;
    ptl_event_t      event;
    int              context;
    int              source;
    int              tag;
    int              count;

    P30_Printf("Dumping unexpected msg queue\n");
    if ( (ptr = _mpi_p30_unex_list_head) != NULL ) {
	do {
	    P30_GET_CONTEXT( ptr->event.match_bits, context );
	    P30_GET_SOURCE(  ptr->event.match_bits, source  );
	    P30_GET_TAG(     ptr->event.match_bits, tag     );
	    P30_Printf("----\n");
	    P30_Printf("  context   = %d\n",context);
	    P30_Printf("  source    = %d\n",source);
	    P30_Printf("  tag       = %d\n",tag);
	    P30_Printf("  length    = %d\n",ptr->event.rlength);
	    ptr = ptr->next;
	} while ( ptr );
    }

    PtlEQCount( _mpi_p30_unex_eq_handle, &count );

    while( count-- ) {
        if ( (_mpi_p30_errno = PtlEQGet( _mpi_p30_unex_eq_handle, &event )) == PTL_OK ) {
	    P30_GET_CONTEXT( event.match_bits, context );
	    P30_GET_SOURCE(  event.match_bits, source  );
	    P30_GET_TAG(     event.match_bits, tag     );
	    P30_Printf("----\n");
	    P30_Printf("  context   = %d\n",context);
	    P30_Printf("  source    = %d\n",source);
	    P30_Printf("  tag       = %d\n",tag);
	    P30_Printf("  length    = %d\n",event.rlength);
        }
    }
    
}

void P30_Dump_event( ptl_event_t *event )
{
    P30_Printf("  event =>\n");
    P30_Printf("    type = ");
    switch ( event->type ) {
    case PTL_EVENT_GET   : P30_Printf("PTL_EVENT_GET\n"); break;
    case PTL_EVENT_PUT   : P30_Printf("PTL_EVENT_PUT\n"); break;
    case PTL_EVENT_REPLY : P30_Printf("PTL_EVENT_REPLY\n"); break;
    case PTL_EVENT_ACK   : P30_Printf("PTL_EVENT_ACK\n"); break;
    case PTL_EVENT_SENT  : P30_Printf("PTL_EVENT_SENT\n"); break;
    default              : P30_Printf("UNKNOWN EVENT!\n"); break;
    }
    P30_Printf("  initiator =>\n");
    P30_Printf("     gid    = %d\n", event->initiator.gid);
    P30_Printf("     rid    = %d\n", event->initiator.rid);
    P30_Printf("  portal    = %d\n", event->portal);
    P30_Printf("  match_bits = 0x%lx\n",event->match_bits);
    P30_Printf("  rlength    = %d\n", event->rlength);
    P30_Printf("  mlength    = %d\n", event->mlength);
    P30_Printf("  offset     = %d\n", event->offset);
    P30_Printf("  md =>\n");
    P30_Printf("    start      = %p\n", event->md.start);
    P30_Printf("    length     = %d\n", event->md.length);
    P30_Printf("    threshold  = %d\n",event->md.threshold);
    P30_Printf("    max_offset = %d\n",event->md.max_offset);
    P30_Printf("    options    = %d\n",event->md.options);
    P30_Printf("    user_ptr   = %p\n",event->md.user_ptr);
    P30_Printf("    eventq     = %d\n",event->md.eventq);
    P30_Printf("  hdr_data     = %ld\n",event->hdr_data);
}

void P30_Dump_md( ptl_md_t *md )
{

    P30_Printf("  md          = %p\n",md);
    P30_Printf("    start     = %p\n",md->start);
    P30_Printf("    length    = %d\n",md->length);
    if ( md->threshold == PTL_MD_THRESH_INF ) {
	P30_Printf("    threshold = PTL_MD_THRESH_INF\n");
    } else {
	P30_Printf("    threshold = %d\n",md->threshold);
    }
    P30_Printf("    max_offset = %d\n",md->max_offset);
    P30_Printf("    options   = ");
    if ( md->options & PTL_MD_OP_PUT ) {
	printf("PTL_MD_OP_PUT ");
    }
    if ( md->options & PTL_MD_OP_GET ) {
	printf("PTL_MD_OP_GET ");
    }
    if ( md->options & PTL_MD_MANAGE_REMOTE ) {
	printf("PTL_MD_MANAGE_REMOTE ");
    }
    if ( md->options & PTL_MD_TRUNCATE ) {
	printf("PTL_MD_TRUNCATE ");
    }
    if ( md->options & PTL_MD_ACK_DISABLE ) {
	printf("PTL_MD_ACK_DISABLE ");
    }
    printf("\n");
    P30_Printf("    user_ptr   = %p\n",md->user_ptr);
    P30_Printf("    eventq     = ");
    if ( md->eventq == PTL_EQ_NONE ) {
	printf("PTL_EQ_NONE\n");
    } else {
	printf("%d\n",md->eventq);
    }
}

void P30_Dump_process_id( ptl_process_id_t *id )
{
    P30_Printf("  id           = %p\n");
    P30_Printf("    addr_kind  = ");
    if ( id->addr_kind == PTL_ADDR_NID ) {
	printf("PTL_ADDR_NID\n");
    } else if ( id->addr_kind == PTL_ADDR_GID ) {
	printf("PTL_ADDR_GID\n");
    } else if ( id->addr_kind == PTL_ADDR_BOTH ) {
	printf("PTL_ADDR_BOTH\n");
    }
    if ( (id->addr_kind == PTL_ADDR_NID) ||
	 (id->addr_kind == PTL_ADDR_BOTH ) ) {
	P30_Printf("    nid       = %d\n",id->nid);
	P30_Printf("    pid       = %d\n",id->pid);
    }
    if ( (id->addr_kind == PTL_ADDR_GID) ||
	 (id->addr_kind == PTL_ADDR_BOTH ) ) {
	P30_Printf("    gid       = %d\n",id->gid);
	P30_Printf("    rid       = %d\n",id->rid);
    }
}
