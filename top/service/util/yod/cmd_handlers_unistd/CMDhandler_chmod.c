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
/* $Id: CMDhandler_chmod.c,v 1.6 2001/02/16 05:44:25 lafisk Exp $ */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include "CMDhandlerTable.h"
#include "util.h"
#include "host_msg.h"
#include "fillWorkBuf.h"
#include "un_id.h"
#include "config.h"

#include "rpc_msgs.h"
#include "srvr_comm.h"


/******************************************************************************/

VOID 
CMDhandler_chmod( control_msg_handle *mh)
{
hostReply_t ack;
INT32        retVal;
CHAR          fname[MAXPATHLEN+1];
int         snid, spid, bufnum=-1, len, rc;
hostCmd_t   *cmd;
int gid_real = getgid();


    CMD_SET_VALS(snid, spid, cmd, bufnum, len, mh);

    if ( DBG_FLAGS( DBG_IO_1 ) ) {
        fprintf( stderr, "CMDhandler_chmod() nid %i pid %i\n", snid, spid );
    }

    if (len > MAXPATHLEN){
        ioErrno = EPROTOCOL;
        goto done;
    }
    
    if (bufnum < 0){
         ioErrno = ERESOURCE;
         goto done;
    }

    rc = srvr_comm_put_reply(mh, workBufData(bufnum),  len);
    
    if (rc){
        ioErrno = CPerrno;
        goto done;
    }

    strcpy( fname, workBufData( bufnum ) );

    ack.your_seq_no = cmd->my_seq_no;

    if ( iAmFyod() ) {
      if (cmd->fail == 1) {
        ack.retVal = -1;
        ack.hostErrno = EFAULT;

        send_ack_to_app(mh, &ack);
        
        goto done;
      }
      else {
        memmove(fname+6, fname, cmd->info.chmodCmd.fnameLen);
        memmove(fname, "/etc/var/sfyod", 14);
      }
    }

    setgid( cmd->gid );
    setreuid( -1, cmd->euid );

    retVal = chmod( fname, cmd->info.chmodCmd.mode );

    ack.hostErrno = errno;
    ack.retVal = retVal;

    setreuid( -1, getuid() );
    setgid( gid_real );

    send_ack_to_app(mh, &ack);

done:

    if (bufnum >= 0) free_work_buf(bufnum);

    if ( DBG_FLAGS( DBG_IO_1 ) ) {
        fprintf( stderr, "CMDhandler_chmod() returning\n" );
    }
    return;
}
