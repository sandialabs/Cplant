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
/* $Id: CMDhandler_write.c,v 1.3 2001/02/16 05:44:25 lafisk Exp $ */


#include <stdio.h>	
#include <errno.h>
#include "fileHandle.h"	
#include "util.h"	
#include "CMDhandlerTable.h"
#include "host_msg.h"
#include "host_write.h"
#include "srvr_comm.h"

/******************************************************************************/

VOID
CMDhandler_write( control_msg_handle *mh) 

{
#undef CMD
#define CMD	( cmd->info.writeCmd )

hostReply_t 	ack;
fileHandle_t	*fh;
INT32		bytesWritten;
BIGGEST_OFF	curPos;
int             snid, spid, hostErrno;
hostCmd_t   *cmd;

    cmd = (hostCmd_t *)SRVR_HANDLE_USERDEF(*mh);
    snid = SRVR_HANDLE_NID(*mh);
    spid = SRVR_HANDLE_PID(*mh);

    if (DBG_FLAGS(DBG_IO_1)) {
	fprintf(stderr, "\nCMDhandler_write( nid %i, pid %i )\n", snid, spid);
    }

    fh = (fileHandle_t *)CMD.hostFileIndex;

    ack.your_seq_no = cmd->my_seq_no;
    curPos = fh->curPos;

    if (cacheFileHandle(fh)) {
        /*
        ** Read blocks from app process and write.  Set ioErrno on
        ** error.  Update *fh.
        */
	bytesWritten = host_write(fh, mh, &hostErrno);

        if (bytesWritten > 0){
	    curPos = fh->curPos;
	}

    } else {

	ack.retVal = -1;

        if (ioErrno == EOHHELL){
            ack.hostErrno = EIO;  /* yod error, not file system error */
        }
        else{
            ack.hostErrno = EBADF;
        }
        send_ack_to_app(mh, &ack);

        if ( DBG_FLAGS( DBG_IO_1 ) ) {
            fprintf( stderr,
                 "\nCMDhandler_write() cacheFileHandle failure %p\n", fh);
        }
        return;
    }

    ack.retVal = bytesWritten;
    ack.hostErrno = hostErrno;
    ack.info.writeAck.curPos = fh->curPos ;

    send_ack_to_app(mh, &ack);

    if (DBG_FLAGS(DBG_IO_1)) {
	fprintf(stderr, "\nCMDhandler_write() returning\n");
    }

}  /* end of CMDhandler_write() */
