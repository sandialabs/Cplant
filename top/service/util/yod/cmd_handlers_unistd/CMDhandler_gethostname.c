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
/* $Id: CMDhandler_gethostname.c,v 1.6 2001/02/16 05:44:25 lafisk Exp $ */


#include "puma.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <errno.h>
#include "CMDhandlerTable.h"
#include "util.h"
#include "host_msg.h"
#include "fillWorkBuf.h"
#include "config.h"
#include "unistd.h"
#include "srvr_comm.h"



/******************************************************************************/

VOID 
CMDhandler_gethostname( control_msg_handle *mh)
{
hostReply_t ack;
hostCmd_t   *cmd;
int     bnum,snid,spid;

    cmd = (hostCmd_t *)SRVR_HANDLE_USERDEF(*mh);
    snid = SRVR_HANDLE_NID(*mh);
    spid = SRVR_HANDLE_PID(*mh);

    if (DBG_FLAGS(DBG_IO_1)) {
        fprintf(stderr,"CMDhandler_gethostname() nid %i pid %i\n", snid, spid);
    }
    ack.your_seq_no = cmd->my_seq_no;

    if((bnum = get_work_buf()) < 0 ) {
        ack.hostErrno = ENOMEM;
        ack.retVal = -1; 

        send_ack_to_app(mh, &ack);
    } else  {
        ack.retVal = gethostname(workBufData(bnum),
                      cmd->info.gethnameCmd.hnameLen);
        ack.hostErrno = errno;

        if (ack.retVal == -1){
            send_ack_to_app(mh, &ack);
        }
        else{
	    send_workbuf_and_ack(mh, bnum, 
                 strlen(workBufData(bnum)) + 1, &ack);
        }

        free_work_buf(bnum);
    }

   return;
}  
