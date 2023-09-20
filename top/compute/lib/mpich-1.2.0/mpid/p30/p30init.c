#include "mpiddev.h"

/***************************************************************************

  MPID_P30_InitMsgPass

  ***************************************************************************/
MPID_Device *MPID_P30_InitMsgPass( argc, argv, short_len, long_len )
int  *argc;
char ***argv;
int  short_len, long_len;
{
    MPID_Device *dev;
    char        *tmpstring;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P30_Printf("entering MPID_CH_InitMsgPass()\n");
    }
#endif

    /* get the length of a long protocol message */
    if ( tmpstring = getenv(MPI_ENV_LONG_MSG_STRING) ) {
	_mpi_p30_short_size = atoi(tmpstring);
    }
    if ( _mpi_p30_short_size < 0 ) {
	_mpi_p30_short_size = P30_DEFAULT_LONG_MSG;
    }

    short_len = _mpi_p30_short_size;

    dev = (MPID_Device *)MALLOC( sizeof(MPID_Device) );
    if (!dev) return 0;
    /* if (short_len < 0) short_len = 4096; */
    if (long_len < 0)  long_len  = 0;
    dev->long_len     = short_len;
    dev->vlong_len    = long_len;
    dev->short_msg    = MPID_P30_Short_setup();
    dev->long_msg     = MPID_P30_Long_setup();
    dev->vlong_msg    = dev->long_msg;
    dev->ready        = MPID_P30_Ready_setup();
    dev->check_device = 0;
    dev->terminate    = MPID_P30_End;
    dev->abort	      = MPID_P30_Abort;
    dev->next	      = 0;

    /* Set the file for Debugging output.  The actual output is controlled
       by MPIDDebugFlag */
#ifdef MPID_DEBUG_ALL
    if (MPID_DEBUG_FILE == 0) MPID_DEBUG_FILE = stdout;
#endif

    if ( MPID_P30_init( argc, *argv ) ) {

#ifdef MPID_DEBUG_ALL
      if ( MPID_DebugFlag ) {
          P30_Printf("Error initializing portals device\n");
      }
#endif

	FREE( dev );
	return 0;
    }

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P30_Printf("leaving MPID_CH_InitMsgPass()\n");
    }
#endif

    return dev;
}


/***************************************************************************

  MPID_P30_Abort

  ***************************************************************************/
int MPID_P30_Abort( comm, code, msg )
struct MPIR_COMMUNICATOR *comm;
int      code;
char     *msg;
{
    extern unsigned int _my_pid;

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P30_Printf("entering P30_Abort()\n");
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

  MPID_P30_End

  ***************************************************************************/
int MPID_P30_End( dev )
MPID_Device *dev;
{

#ifdef MPID_DEBUG_ALL
    if ( MPID_DebugFlag ) {
        P30_Printf("entering MPID_P30_End()\n");
    }
#endif

    /* Finish off any pending transactions */
    /* MPID_P30_Complete_pending(); */

    (dev->short_msg->delete)( dev->short_msg );
    (dev->long_msg->delete)( dev->long_msg );
    /* same protocol as long message, so don't delete again */
    /* (dev->vlong_msg->delete)( dev->vlong_msg ); */
    FREE( dev );
    /* We should really generate an error or warning message if there 
       are uncompleted operations... */
    MPID_P30_finalize();
    return 0;
}


/***************************************************************************

  MPID_P30_Version_name

  ***************************************************************************/
void MPID_P30_Version_name( name )
char *name;
{
    sprintf( name, "ADI version %4.2f - transport %s", MPIDPATCHLEVEL, 
	     MPIDTRANSPORT );
}

