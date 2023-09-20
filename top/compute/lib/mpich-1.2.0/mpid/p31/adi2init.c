/*
 *  $Id: adi2init.c,v 1.1 2001/12/18 00:34:41 rbbrigh Exp $
 *
 *  (C) 1995 by Argonne National Laboratory and Mississipi State University.
 *      All rights reserved.  See COPYRIGHT in top-level directory.
 */

#include "mpiddev.h"

/* Home for these globals */
int MPID_MyWorldSize, MPID_MyWorldRank;
MPID_SBHeader MPIR_rhandles;
MPID_SBHeader MPIR_shandles;

/***************************************************************************/
/* Some operations are completed in several stages.  To ensure that a      */
/* process does not exit from MPID_End while requests are pending, we keep */
/* track of how many are out-standing                                      */
/***************************************************************************/
int MPID_n_pending = 0;
/*
 * Create the MPID_DevSet device from the requested devices, and
 * initialize the device mapping
 */

/* This COULD be a single piece of permanent storage, but that is awkward
   for shared-memory versions (hot-spot-references).  */
MPID_DevSet *MPID_devset = 0;

extern MPID_Device *MPID_CH_InitMsgPass 
       ANSI_ARGS(( int *, char ***, int, int ));

int MPID_Complete_pending ANSI_ARGS((void));

static int MPID_Short_len = -1;

void MPID_Init( argc, argv, config, error_code )
int  *argc, *error_code;
void *config;
char ***argv;
{
    int i, np;
    MPID_Device *dev;
    MPID_Config *config_info = (MPID_Config *)config;

    /*
     * Create the device set structure.  Currently, only handles one
     * device and maps all operations to that device
     */
    MPID_devset = (MPID_DevSet *)MALLOC( sizeof(MPID_DevSet) );
    if (!MPID_devset) {
	*error_code = MPI_ERR_INTERN;
	return ;
    }
    /* Make devset safe for initializations errors */
    MPID_devset->dev_list = 0;

    dev = MPID_CH_InitMsgPass( argc, argv, MPID_Short_len, -1 );
    if (!dev) {
	*error_code = MPI_ERR_INTERN;
	return;
    }
    np = MPID_MyWorldSize;
    MPID_devset->ndev = 1;
    MPID_devset->dev  = dev;

    MPID_devset->ndev_list   = 1;
    MPID_devset->dev_list    = dev;

    /* 
     * Get the basic options.  Note that this must be AFTER the initialization
     * in case the initialization routine was responsible for sending the
     * arguments to other processors.
     */
    MPID_ProcessArgs( argc, argv );

    /* Initialize the send/receive handle allocation system */
    /* Use the persistent version of send/receive (since we don't have
       separate MPIR_pshandles/MPIR_prhandles) */
    MPIR_shandles   = MPID_SBinit( sizeof( MPIR_PSHANDLE ), _mpi_p31_default_me_size, 100 );
    MPIR_rhandles   = MPID_SBinit( sizeof( MPIR_PRHANDLE ), _mpi_p31_default_me_size, 100 );
    /* Needs to be changed for persistent handles.  A common request form? */

    MPID_devset->req_pending = 0;
    *error_code = MPI_SUCCESS;
}

/* Barry Smith suggests that this indicate who is aborting the program.
   There should probably be a separate argument for whether it is a 
   user requested or internal abort.
 */
void MPID_Abort( comm_ptr, code, user, str )
struct MPIR_COMMUNICATOR *comm_ptr;
int      code;
char     *user, *str;
{
    MPID_Device *dev;

    fprintf( stderr, "[%d] %s Aborting program %s\n", MPID_MyWorldRank,
	     user ? user : "", str ? str : "!" );
    fflush( stderr );
    fflush( stdout );

    /* We may be aborting before defining any devices */
    if (MPID_devset) {
	if ( MPID_devset->dev ) {
	    MPID_devset->dev->abort( comm_ptr, code, str );
	}
    }
    else {
	exit( code );
    }
}

void MPID_End()
{
    MPID_Device *dev, *ndev;

    /* Finish off any pending transactions */
    /* Should this be part of the device terminate routines instead ? 
       Probably not, incase they need to be done in an arbitrary sequence */
    /*    MPID_Complete_pending(); */
    
    /* Eventually make this optional */
    /* MPID_Dump_queues(); */

    if ( MPID_devset ) {
	MPID_devset->dev->terminate( MPID_devset->dev );
    }

    /* Clean up request handles */
    MPID_SBdestroy( MPIR_shandles );
    MPID_SBdestroy( MPIR_rhandles );

    /* Free remaining storage */
    FREE( MPID_devset->dev );
    FREE( MPID_devset );

}

int MPID_Complete_pending()
{
    MPID_Device *dev;
    int         lerr;

    if (MPID_devset->ndev_list == 1) {
	dev = MPID_devset->dev_list;
	while (MPID_n_pending > 0) {
	    lerr = (*dev->check_device)( dev, MPID_BLOCKING );
	    if (lerr > 0) {
		return lerr;
	    }
	}
    }

    return MPI_SUCCESS;
}

void MPID_SetPktSize( len )
int len;
{
    MPID_Short_len = len;
}

/*
  Perhaps this should be a util function
 */
/*
int MPID_WaitForCompleteSend( request )
MPIR_SHANDLE *request;
{
    while (!request->is_complete)
	MPID_DeviceCheck( MPID_BLOCKING );
    return MPI_SUCCESS;
}

int MPID_WaitForCompleteRecv( request )
MPIR_RHANDLE *request;
{
    while (!request->is_complete)
	MPID_DeviceCheck( MPID_BLOCKING );
    return MPI_SUCCESS;
}
*/
void MPID_Version_name( name )
char *name;
{
    sprintf( name, "ADI version %4.2f - transport %s", MPIDPATCHLEVEL, 
	     MPIDTRANSPORT );
}

