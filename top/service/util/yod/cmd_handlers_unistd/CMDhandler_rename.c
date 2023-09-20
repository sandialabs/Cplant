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
/* $Id: CMDhandler_rename.c,v 1.6 2001/02/16 05:44:25 lafisk Exp $ */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "CMDhandlerTable.h"
#include "util.h"
#include "host_msg.h"
#include "fillWorkBuf.h"
#include "config.h"
#include "srvr_comm.h"




/******************************************************************************/

VOID 
CMDhandler_rename( control_msg_handle *mh)
{
hostReply_t ack;
CHAR          fname1[MAXPATHLEN+1];
CHAR          fname2[MAXPATHLEN+1];
int gid_real = getgid();
int         bufnum=-1, len, snid, spid, rc;
hostCmd_t   *cmd;

    CMD_SET_VALS(snid, spid, cmd, bufnum, len, mh);

    if (DBG_FLAGS(DBG_IO_1))
    {
        fprintf(stderr,"CMDhandler_rename() nid %i pid %i\n", snid, spid);
    }
    if (len > (MAXPATHLEN*2)){
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
    
    strcpy( fname1, workBufData( bufnum ) );
    strcpy( fname2, workBufData( bufnum ) + cmd->info.renameCmd.fnameLen1 );

    ack.your_seq_no = cmd->my_seq_no;

    if ( iAmFyod() ) {
      if (cmd->fail == 1) {
        ack.retVal = -1;
        ack.hostErrno = EFAULT;

        send_ack_to_app(mh, &ack);
        goto done; 
      }
      else {
        memmove(fname1+6, fname1, cmd->info.linkCmd.fnameLen1);
        memmove(fname1, "/etc/var/sfyod", 14);
        memmove(fname2+6, fname2, cmd->info.linkCmd.fnameLen2);
        memmove(fname2, "/etc/var/sfyod", 14);
      }
    }

    setreuid( -1, cmd->euid );
    setgid( cmd->gid );

    ack.retVal = rename( fname1, fname2 );
    ack.hostErrno = errno;

    setreuid( -1, getuid() );
    setgid( gid_real );

    send_ack_to_app(mh, &ack);

done:

    if (bufnum >= 0) free_work_buf(bufnum);

    return;
}
