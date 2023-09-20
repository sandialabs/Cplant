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
/* $Id: CMDhandler_setgid.c,v 1.4 2001/08/27 02:18:38 lafisk Exp $ */


#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include "CMDhandlerTable.h"
#include "util.h"
#include "host_msg.h"
#include "un_id.h"
#include "config.h"
#include "srvr_comm.h"


extern INT32 physnid2rank[MAX_MESH_SIZE];
/******************************************************************************/

VOID
CMDhandler_setgid(control_msg_handle *mh)
{

hostReply_t	ack;
INT32 		retVal;
gid_t 		savgid;
int        snid, spid;
hostCmd_t       *cmd;

    snid = SRVR_HANDLE_NID(*mh);
    spid = SRVR_HANDLE_PID(*mh);
    cmd = (hostCmd_t *)SRVR_HANDLE_USERDEF(*mh);

    if (DBG_FLAGS(DBG_IO_1)) {
	fprintf(stderr,"CMDhandler_setgid() nid %i pid %i\n", snid, spid);
    }

    savgid = getgid();
    retVal = setgid( (gid_t)(cmd->info.setidCmd.id) );
    if (!retVal) {
    	un_ids[physnid2rank[snid]].gid = cmd->info.setidCmd.id;
    	setgid( savgid );
    }

    ack.your_seq_no = cmd->my_seq_no;
    ack.retVal = retVal;
    ack.hostErrno = errno;

    send_ack_to_app(mh, &ack);
}

/******************************************************************************/

VOID
CMDhandler_setegid(control_msg_handle *mh)
{

hostReply_t	ack;
INT32 		retVal;
gid_t 		sav_egid;
int        snid, spid;
hostCmd_t       *cmd;

    snid = SRVR_HANDLE_NID(*mh);
    spid = SRVR_HANDLE_PID(*mh);
    cmd = (hostCmd_t *)SRVR_HANDLE_USERDEF(*mh);

    if (DBG_FLAGS(DBG_IO_1)) {
	fprintf(stderr,"CMDhandler_setegid() nid %i pid %i\n", snid, spid);
    }
    sav_egid = getegid();
    retVal = setegid( (gid_t)(cmd->info.setidCmd.id));
    if (!retVal) {
    	un_ids[physnid2rank[snid]].egid = cmd->info.setidCmd.id;
    	setegid( sav_egid );
    }

    ack.your_seq_no = cmd->my_seq_no;
    ack.retVal = retVal;
    ack.hostErrno = errno;

    send_ack_to_app(mh, &ack);
}
