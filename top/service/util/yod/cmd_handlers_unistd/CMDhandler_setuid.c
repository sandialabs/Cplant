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
/* $Id: CMDhandler_setuid.c,v 1.5 2001/08/27 02:18:38 lafisk Exp $ */

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include "CMDhandlerTable.h"
#include "util.h"
#include "host_msg.h"
#include "un_id.h"
#include "config.h"
#include "srvr_comm.h"


extern INT32 physnid2rank[MAX_MESH_SIZE];

#define ROOTUID 0

/******************************************************************************/

VOID
CMDhandler_setuid(control_msg_handle *mh)
{

hostReply_t 	ack;
uid_t		cmd_id;
int        snid, spid;
hostCmd_t       *cmd;

    snid = SRVR_HANDLE_NID(*mh);
    spid = SRVR_HANDLE_PID(*mh);
    cmd = (hostCmd_t *)SRVR_HANDLE_USERDEF(*mh);

    cmd_id = (uid_t) cmd->info.setidCmd.id;

    if (DBG_FLAGS(DBG_IO_1)) {
	fprintf(stderr, "CMDhandler_setuid() nid %i pid %i\n", snid, spid);
    }

    ack.your_seq_no = cmd->my_seq_no;

    if ((un_ids[physnid2rank[snid]].euid == cmd_id) &&
	    (un_ids[physnid2rank[snid]].euid != ROOTUID)) {
	ack.retVal = 0;
    } else  if ((un_ids[physnid2rank[snid]].euid == ROOTUID) ||
	    ((un_ids[physnid2rank[snid]].uid  == ROOTUID) &&
	    (un_ids[physnid2rank[snid]].euid == cmd_id)) ||
	    (un_ids[physnid2rank[snid]].uid == cmd_id)) {

	if (un_ids[physnid2rank[snid]].euid == ROOTUID) {
	    un_ids[physnid2rank[snid]].sav_euid = cmd_id;
	}
	un_ids[physnid2rank[snid]].uid = cmd_id;
	un_ids[physnid2rank[snid]].euid = cmd_id;
	ack.retVal = 0;
    } else if ((cmd_id) && (cmd_id == un_ids[physnid2rank[snid]].sav_euid)) {
	un_ids[physnid2rank[snid]].euid = cmd_id;
	ack.retVal = 0;
    } else {
	ack.retVal = -1;
	ack.hostErrno = EPERM;
    }

    send_ack_to_app(mh, &ack);
}

/******************************************************************************/

VOID
CMDhandler_seteuid(control_msg_handle *mh)
{

hostReply_t 	ack;
INT32		retVal;
uid_t           cmd_id;
int        snid, spid;
hostCmd_t       *cmd;

    snid = SRVR_HANDLE_NID(*mh);
    spid = SRVR_HANDLE_PID(*mh);
    cmd = (hostCmd_t *)SRVR_HANDLE_USERDEF(*mh);

    cmd_id = (uid_t) cmd->info.setidCmd.id;

    if (DBG_FLAGS(DBG_IO_1)) {
	fprintf(stderr,"CMDhandler_seteuid() nid %i pid %i\n", snid, spid);
    }

    if ((un_ids[physnid2rank[snid]].euid == ROOTUID) ||
	    ((un_ids[physnid2rank[snid]].uid == ROOTUID) &&
	    (cmd_id == ROOTUID)) ||
	    ((cmd_id == un_ids[physnid2rank[snid]].uid) ||
	    (cmd_id == un_ids[physnid2rank[snid]].euid) ||
	    (cmd_id == un_ids[physnid2rank[snid]].sav_euid))) {
	un_ids[physnid2rank[snid]].euid = cmd_id;
	retVal = 0;
    } else {
	retVal = -1;
	errno = EPERM;
    }
/*
    sav_euid = geteuid();
    retVal = seteuid( (uid_t)(cmd_id) );
    if (!retVal) {
	un_ids[snid].euid = cmd_id;
	seteuid( sav_euid );
    }
*/

    ack.your_seq_no = cmd->my_seq_no;
    ack.retVal = retVal;
    ack.hostErrno = errno;

    send_ack_to_app(mh, &ack);
}
