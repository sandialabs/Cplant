
#ifndef PTLS_DEBUG_H
#define PTLS_DEBUG_H

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
void PTLS_Printf( char *, ... );

#endif PTLS_DEBUG_H
