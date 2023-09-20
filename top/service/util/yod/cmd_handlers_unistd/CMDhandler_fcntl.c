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
/* $Id: CMDhandler_fcntl.c,v 1.3 2001/02/16 05:44:25 lafisk Exp $ */

#include <stdio.h>	
#include <errno.h>
#include <sys/fcntl.h>
#include "CMDhandlerTable.h"
#include "fileHandle.h"	
#include "util.h"
#include "host_msg.h"
#include <sys/types.h>
#include <fcntl.h>
#include "rpc_msgs.h"
#include "srvr_comm.h"




/******************************************************************************/

VOID 
CMDhandler_fcntl(control_msg_handle *mh)
{

fileHandle_t	*fh;
hostReply_t	ack;
INT32		retVal;
hostCmd_t   *cmd;

    if (DBG_FLAGS(DBG_IO_1)) {	
	fprintf(stderr,"CMDhandler_fcntl() nid %i pid %i\n",
	                SRVR_HANDLE_NID(*mh),
	                SRVR_HANDLE_PID(*mh));
    }

    cmd = (hostCmd_t *)SRVR_HANDLE_USERDEF(*mh);

    fh = (fileHandle_t *) cmd->info.fcntlCmd.hostFileIndex;

    if ( cacheFileHandle( fh ) ) {
	retVal = fcntl( fh->fd,cmd->info.fcntlCmd.request,
			cmd->info.fcntlCmd.arg );
    } else {
	retVal = -1;
	/*errno = WHAT*/;
    }

    ack.hostErrno = errno;
    ack.retVal = retVal; 
    ack.your_seq_no = cmd->my_seq_no;

    send_ack_to_app(mh, &ack);

    return;

}
