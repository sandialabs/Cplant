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
/* $Id: CMDhandler_lseek.c,v 1.5 2001/02/16 05:44:25 lafisk Exp $ */


#include "puma.h"
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>	
#include <errno.h>
#include "fileHandle.h"	
#include "positionFilePtr.h"
#include "util.h"
#include "CMDhandlerTable.h"
#include "host_msg.h"
#include "srvr_comm.h"

/******************************************************************************/

VOID CMDhandler_lseek(control_msg_handle *mh)
{
fileHandle_t    *fh;
hostReply_t	    ack;
INT32	    retVal;
int          snid, spid;
hostCmd_t    *cmd;

    snid = SRVR_HANDLE_NID(*mh);
    spid = SRVR_HANDLE_PID(*mh);
    cmd = (hostCmd_t *)SRVR_HANDLE_USERDEF(*mh);

    if ( DBG_FLAGS( DBG_IO_1 ) ) {
	fprintf(stderr,"CMDhandler_lseek( snid %i, pid %i )\n", snid, spid );
    }

    fh = ( fileHandle_t * ) cmd->info.lseekCmd.hostFileIndex; 

    if ( cacheFileHandle( fh ) ) {
	if ( DBG_FLAGS( DBG_IO_1 ) ) {
	    fprintf(stderr,"CMDhandler_lseek(): lseek( %i, %i, %i)\n", 
						fh->fd, 
						cmd->info.lseekCmd.offset, 
						cmd->info.lseekCmd.whence );
	}

	if ( ( retVal = positionFilePtr( fh, cmd->info.lseekCmd.curPos, 
					snid, spid ) ) != -1 ) {
	    fh->curPos = retVal;	
	}

	if ( ( retVal = lseek( fh->fd,  ( off_t ) cmd->info.lseekCmd.offset, 
					cmd->info.lseekCmd.whence ) ) != -1 ) {
	    fh->curPos = retVal;	
	}

    } else {
	retVal = -1;
    	/* errno = WHAT */ 
    }

    ack.hostErrno = errno;
    ack.retVal = retVal;
    ack.your_seq_no = cmd->my_seq_no;

    if ( DBG_FLAGS( DBG_IO_1 ) ) {

	fprintf(stderr,"CMDhandler_lseek(): lseek retval %i\n", retVal );
	if ( ack.retVal == -1 ) {
		perror("CMDhandler_lseek(): error");
	}
    }
    send_ack_to_app(mh, &ack);
}  
