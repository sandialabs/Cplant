#include "ptlsdev.h"

/***************************************************************************

  MPID_PTLS_InitMsgPass

  ***************************************************************************/
MPID_Device *MPID_PTLS_InitMsgPass( argc, argv, short_len, long_len )
int  *argc;
char ***argv;
int  short_len, long_len;
{
    MPID_Device *dev;
    char        *tmpstring;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("entering MPID_CH_InitMsgPass()\n");
    }
#endif

    if ( (tmpstring = getenv(MPI_ENV_SHORT_MSG_SIZE_STRING)) )
      short_len = atoi( tmpstring );
    if ( short_len < PTLS_DEFAULT_SHORT_MSG_SIZE )
      short_len = PTLS_DEFAULT_SHORT_MSG_SIZE;

    dev = (MPID_Device *)MALLOC( sizeof(MPID_Device) );
    if (!dev) return 0;
    /* if (short_len < 0) short_len = 4096; */
    if (long_len < 0)  long_len  = 0;
    dev->long_len     = short_len;
    dev->vlong_len    = long_len;
    dev->short_msg    = MPID_PTLS_Short_setup();
    dev->long_msg     = MPID_PTLS_Long_setup();
    dev->vlong_msg    = dev->long_msg;
    dev->ready        = MPID_PTLS_Ready_setup();
    dev->check_device = 0;
    dev->terminate    = MPID_PTLS_End;
    dev->abort	      = MPID_PTLS_Abort;
    dev->next	      = 0;

    /* Set the file for Debugging output.  The actual output is controlled
       by MPIDDebugFlag */
#ifdef MPID_DEBUG_ALL
    if (MPID_DEBUG_FILE == 0) MPID_DEBUG_FILE = stdout;
#endif

    if ( MPID_PTLS_init( argc, *argv ) ) {

#ifdef MPID_DEBUG_ALL
      if ( MPID_DebugFlag ) {
          PTLS_Printf("Error initializing portals device\n");
      }
#endif

	FREE( dev );
	return 0;
    }

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("leaving MPID_CH_InitMsgPass()\n");
    }
#endif

    return dev;
}


/***************************************************************************

  MPID_PTLS_Abort

  ***************************************************************************/
int MPID_PTLS_Abort( comm, code, msg )
struct MPIR_COMMUNICATOR *comm;
int      code;
char     *msg;
{

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("entering PTLS_Abort()\n");
    }
#endif

    if (msg) {
	fprintf( stderr, "[%d] %s\n", MPID_MyWorldRank, msg );
    }
    else {
	fprintf( stderr, "[%d] Aborting program!\n", MPID_MyWorldRank );
    }
    fflush( stderr );
    fflush( stdout );
    nodekill( -1, _my_pid, SIGKILL );
    exit(code);
    return 0;
}


/***************************************************************************

  MPID_PTLS_End

  ***************************************************************************/
int MPID_PTLS_End( dev )
MPID_Device *dev;
{

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        PTLS_Printf("entering MPID_PTLS_End()\n");
    }
#endif

    /* Finish off any pending transactions */
    /* MPID_PTLS_Complete_pending(); */

    (dev->short_msg->delete)( dev->short_msg );
    (dev->long_msg->delete)( dev->long_msg );
    /* same protocol as long message, so don't delete again */
    /* (dev->vlong_msg->delete)( dev->vlong_msg ); */
    FREE( dev );
    /* We should really generate an error or warning message if there 
       are uncompleted operations... */
    MPID_PTLS_finalize();
    return 0;
}


/***************************************************************************

  MPID_PTLS_Version_name

  ***************************************************************************/
void MPID_PTLS_Version_name( name )
char *name;
{
    sprintf( name, "ADI version %4.2f - transport %s", MPIDPATCHLEVEL, 
	     MPIDTRANSPORT );
}

