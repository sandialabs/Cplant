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
/* $Id: CMDhandler_fsync.c,v 1.3 2001/02/16 05:44:25 lafisk Exp $ */


#include <stdio.h>
#include <errno.h>
#include "CMDhandlerTable.h"
#include "fileHandle.h"
#include "util.h"
#include "host_msg.h"

#include "rpc_msgs.h"
#include "srvr_comm.h"

#ifdef LINUX_PORTALS
#include <unistd.h>
#endif


/******************************************************************************/

VOID
CMDhandler_fsync(control_msg_handle *mh)
{

fileHandle_t	*fh;
hostReply_t	ack;
INT32		retVal;
int             snid, spid;
hostCmd_t   *cmd;

    cmd = (hostCmd_t *)SRVR_HANDLE_USERDEF(*mh);
    snid = SRVR_HANDLE_NID(*mh);
    spid = SRVR_HANDLE_PID(*mh);

    if (DBG_FLAGS(DBG_IO_1)) {
	fprintf(stderr,"CMDhandler_fsync() nid %i pid %i\n", snid, spid);
    }

    fh = (fileHandle_t *) cmd->info.fsyncCmd.hostFileIndex;

    if ( cacheFileHandle( fh ) ) {
	retVal = fsync( fh->fd );
    } else {
	retVal = -1;
    }

    ack.hostErrno = errno;
    ack.retVal = retVal;
    ack.your_seq_no = cmd->my_seq_no;

    send_ack_to_app(mh, &ack);

    return;

}
