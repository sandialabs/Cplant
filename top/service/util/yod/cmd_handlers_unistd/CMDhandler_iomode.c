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
/* $Id: CMDhandler_iomode.c,v 1.3 2001/02/16 05:44:25 lafisk Exp $ */



#include "puma.h"
#include <stdio.h>
#include <errno.h>
#include "fileHandle.h"
#include "util.h"
#include "CMDhandlerTable.h"
#include "host_msg.h"


/******************************************************************************/

VOID
CMDhandler_iomode(control_msg_handle *mh)
{

hostReply_t 	ack;
hostCmd_t     *cmd;

    if (DBG_FLAGS(DBG_IO_1)) {
	fprintf(stderr,"CMDhandler_iomode() nid %i, pid %i\n", 
	          SRVR_HANDLE_NID(*mh),
	          SRVR_HANDLE_PID(*mh));
    }
    cmd = (hostCmd_t *)SRVR_HANDLE_USERDEF(*mh);

    ack.retVal = ((fileHandle_t *) cmd->info.iomodeCmd.hostFileIndex)->iomode;
    ack.hostErrno = 0;
    ack.your_seq_no = cmd->my_seq_no;

    send_ack_to_app(mh, &ack);
}
