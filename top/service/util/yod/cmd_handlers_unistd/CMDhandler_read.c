/*************************************************************************
Cplant Release Version 2.0.1.10
Release Date: Nov 5, 2002 
#############################################################################
#
#     This Cplant(TM) source code is the property of Sandia National
#     Laboratories.
#
#     This Cplant(TM) source code is copyrighted by Sandia National
#     Laboratories.
#
#     The redistribution of this Cplant(TM) source code is subject to the
#     terms of the GNU Lesser General Public License
#     (see cit/LGPL or http://www.gnu.org/licenses/lgpl.html)
#
#     Cplant(TM) Copyright 1998, 1999, 2000, 2001, 2002 Sandia Corporation. 
#     Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
#     license for use of this work by or on behalf of the US Government.
#     Export of this program may require a license from the United States
#     Government.
#
#############################################################################
**************************************************************************/
/* $Id: CMDhandler_read.c,v 1.3 2001/02/16 05:44:25 lafisk Exp $ */


#include <stdio.h>	
#include <errno.h>
#include "fileHandle.h"	
#include "util.h"	
#include "CMDhandlerTable.h"
#include "host_msg.h"
#include "host_read.h"
#include "srvr_comm.h"

/******************************************************************************/

VOID
CMDhandler_read(control_msg_handle *mh)
{

#undef CMD
#define CMD	( cmd->info.readCmd )

fileHandle_t	*fh; 
int             snid, spid;
hostCmd_t   *cmd;
hostReply_t ack;

    cmd = (hostCmd_t *)SRVR_HANDLE_USERDEF(*mh);
    snid = SRVR_HANDLE_NID(*mh);
    spid = SRVR_HANDLE_PID(*mh);

    if ( DBG_FLAGS( DBG_IO_1 ) ) {
	fprintf( stderr, "\nCMDhandler_read( nid %i, pid %i )\n", snid, spid );
    }

    fh = ( fileHandle_t * ) CMD.hostFileIndex;

    if ( cacheFileHandle( fh ) ) {
        /*
	** Read and send to app in work buf blocks, last block
	**  contains the ack.  Sets ioErrno on error.  Updates *fh.
	*/
	host_read( fh, mh);

    } else {
	ack.retVal = -1;
	ack.your_seq_no = cmd->my_seq_no;

	if (ioErrno == EOHHELL){
	    ack.hostErrno = EIO;
	}
	else{
	    ack.hostErrno = EBADF;
	}
	send_ack_to_app(mh, &ack);

        if ( DBG_FLAGS( DBG_IO_1 ) ) {
	    fprintf( stderr, 
	         "\nCMDhandler_read() cacheFileHandle failure %p\n", fh);
        }
    }

    if ( DBG_FLAGS( DBG_IO_1 ) ) {
	fprintf( stderr, "\nCMDhandler_read() returning \n" );
    }
}  
