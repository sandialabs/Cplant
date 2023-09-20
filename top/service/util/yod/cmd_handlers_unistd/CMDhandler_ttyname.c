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
/* $Id: CMDhandler_ttyname.c,v 1.4 2001/02/16 05:44:25 lafisk Exp $ */



#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "CMDhandlerTable.h"
#include "fileHandle.h"
#include "util.h"
#include "host_msg.h"
#include "srvr_comm.h"

#ifdef LINUX_PORTALS
#include <unistd.h>
#endif

/******************************************************************************/

VOID
CMDhandler_ttyname( control_msg_handle *mh)
{

fileHandle_t 	*fh;
hostReply_t 	ack;
char		*retVal;
int             snid, spid, bufnum;
hostCmd_t   *cmd;

    cmd = (hostCmd_t *)SRVR_HANDLE_USERDEF(*mh);
    snid = SRVR_HANDLE_NID(*mh);
    spid = SRVR_HANDLE_PID(*mh);

    if (DBG_FLAGS(DBG_IO_1))   {
	fprintf(stderr, "CMDhandler_ttyname() nid %i pid %i\n", snid, spid);
    }
    ack.your_seq_no = cmd->my_seq_no; 
    ack.retVal      = 0;

    bufnum = get_work_buf();
   
    if (bufnum < 0){
        ack.hostErrno = EIO;
        ioErrno = ERESOURCE;
        send_ack_to_app(mh, &ack);
        return;
    }

    fh = (fileHandle_t *)cmd->info.ttynameCmd.hostFileIndex;

    if (cacheFileHandle(fh)) {
	if ((retVal = (char *) ttyname(fh->fd))) {

 	    strcpy(workBufData(bufnum), retVal);
            ack.retVal = 1;      /* just make it non-NULL */

	}
        else{
            ack.hostErrno = errno;
        }
    } else {
        ack.hostErrno = EIO;
    }

    if (ack.retVal){
        send_workbuf_and_ack(mh, bufnum, 
                         strlen(workBufData(bufnum)) + 1,
                          &ack);
    }
    else{
        send_ack_to_app(mh, &ack);
    }

    free_work_buf(bufnum);

}  /* end of CMDhandler_ttyname() */
