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
/* $Id: CMDhandler_getdirentries.c,v 1.7 2001/02/16 05:44:25 lafisk Exp $ */


#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include "CMDhandlerTable.h"
#include "fileHandle.h"
#include "util.h"
#include "host_msg.h"
#include "fillWorkBuf.h"
#include "un_id.h"
#include "config.h"
#include <memory.h>
#include <malloc.h>
#include "rpc_msgs.h"
#include "srvr_comm.h"

#ifdef LINUX_PORTALS
#include <dirent.h>
#endif



/******************************************************************************/

VOID
CMDhandler_getdirentries( control_msg_handle *mh)
{
#undef CMD
#undef ACK
#define CMD     ( cmd->info.getdirentriesCmd )
#define ACK     ( ack.info.getdirentriesAck )

hostReply_t     ack;
hostCmd_t     *cmd;

#ifdef LINUX_PORTALS
fileHandle_t    *fh;
off_t      basep;
size_t     nbytes, retVal;
#else
int        fh;
long       basep;
int        nbytes, retVal;
#endif

int     bufnum=-1, xfer, remaining, retValTot, rc;


    if (DBG_FLAGS(DBG_IO_1)) {
       fprintf(stderr, "CMDhandler_getdirentries() nid %i pid %i\n",
           SRVR_HANDLE_NID(*mh), SRVR_HANDLE_PID(*mh));
    }
    cmd = (hostCmd_t *)SRVR_HANDLE_USERDEF(*mh);

    fh = (fileHandle_t *)CMD.hostFileIndex;
    nbytes = CMD.nbytes;
    basep =  CMD.basep;

    if (nbytes == 0){
	ack.retVal = 0;
	ACK.basep = basep;

	send_ack_to_app(mh, &ack);
	goto done;
    }

    /*
    ** Fill up and send work buffers with the directory entries
    */

    remaining = nbytes;
    retValTot = 0;

    bufnum = get_work_buf();

    if (bufnum < 0){
        ioErrno = ERESOURCE;
        goto done;
    }

    while (remaining > 0){

	xfer = ((remaining > IOBUFSIZE) ? IOBUFSIZE : remaining);

	retVal = getdirentries(fh->fd, workBufData(bufnum), xfer, &basep);

	if (retVal >= 0){
            retValTot += retVal;
	    remaining -= retVal;
	}

	if ((retVal <= 0) || (remaining == 0)){
	    if (retVal < 0){
	        ack.retVal = -1;
	        ack.hostErrno = errno;        

		send_ack_to_app(mh, &ack);
	    }
	    else{
	        ack.retVal = retValTot;
	        ACK.basep = basep;

	        send_workbuf_and_ack(mh, bufnum, retVal, &ack);
	    }
	    break;
	}
	else{

	    rc = send_workbuf_to_app(mh, bufnum, retVal);

            if (rc < 0){
	        goto done;
	    }
	}
    }

done:

    if (bufnum > 0) free_work_buf(bufnum);

    return;

}  /* end of CMDhandler_getdirentries() */
