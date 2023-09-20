
#ifndef P31_DEBUG_H
#define P31_DEBUG_H

/* global variables */
extern FILE *MPID_DEBUG_FILE;
extern FILE *MPID_TRACE_FILE;
extern int   MPID_DebugFlag;

/* functions */
void MPID_SetDebugFile( char * );
void MPID_SetTraceFile( char * );
void MPID_SetDebugFlag( int );
void mpid_setdebugflag_( void );
void mpid_unsetdebugflag_( void );
void P31_Printf( char *, ... );
void MPID_Dump_queues( int );
void P31_Dump_event( ptl_event_t * );
void P31_Dump_md( ptl_md_t * );
void P31_Dump_process_id( ptl_process_id_t *);

#endif /* P31_DEBUG_H */
