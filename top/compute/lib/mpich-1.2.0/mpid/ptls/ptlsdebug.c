
#include "ptlsdev.h"

FILE *MPID_DEBUG_FILE = stdout;
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
void mpid_setdebugflag_()
{
    MPID_DebugFlag = 1;
}

void mpid_unsetdebugflag()
{
    MPID_DebugFlag = 0;
}

void PTLS_Printf( char *format, ... )
{
    va_list args;

    fprintf(MPID_DEBUG_FILE,"%d: ",_my_rank);

    va_start(args,format);
    vfprintf(MPID_DEBUG_FILE,format,args);
    va_end(args);
}

void MPID_Dump_queues()
{
    /* maybe later */

    return;
}

