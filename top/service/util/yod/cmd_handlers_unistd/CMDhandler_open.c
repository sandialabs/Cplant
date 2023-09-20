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
/* $Id: CMDhandler_open.c,v 1.18 2001/02/16 05:44:25 lafisk Exp $ */


#include <sys/types.h>
#include <stdio.h>	
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "fileHandle.h"	
#include "util.h"	
#include "CMDhandlerTable.h"
#include "host_msg.h"
#include "host_open.h"
#include "fillWorkBuf.h"
#include "config.h"
#include "rpc_msgs.h"
 
#ifdef LINUX_PORTALS
#include <unistd.h>
#include "host_msg.h"
#include "puma.h"
#else
#include "fyodYod.h"
#endif

#include "srvr_comm.h"

/******************************************************************************/

VOID
CMDhandler_open( control_msg_handle *mh)
{

#undef CMD
#undef ACK
#define CMD	( cmd->info.openCmd )
#define ACK	( ack.info.openAck )

    fileHandle_t 	*fh = 0;
    hostReply_t 	ack;
    hostCmd_t           *cmd;
    CHAR 		*fname;
#ifndef LINUX_PORTALS
    extern INT32 	cmd_slot;
#endif
    int rc, bufnum=-1, len, snid, spid;

    CMD_SET_VALS(snid, spid, cmd, bufnum, len, mh);

    if ( DBG_FLAGS( DBG_IO_1 ) ) {
	fprintf( stderr, "CMDhandler_open( nid %i, pid %i )\n", snid, spid );
    }

    if (len > MAXPATHLEN){
         ioErrno = EPROTOCOL;
	 goto done;
    }

    if (bufnum < 0){
         ioErrno = ERESOURCE;
	 goto done;
    }

    rc = srvr_comm_put_reply(mh, (fname=workBufData(bufnum)),  len);

    if (rc){
	ioErrno = CPerrno;
        goto done;
    }

    if ( iAmFyod() ) {
      /* this assumes /raid_0XY/blah... as the nameing scheme for fyod; 
         the first instruction ensures that the Y/blah... part gets 
         positioned so that the result of the second instruction is
         "/etc/var/syfodY/blah..."
      */
      memmove(fname+6, fname, CMD.fnameLen);
      memmove(fname, "/etc/var/sfyod", 14);
    }

    ack.your_seq_no = cmd->my_seq_no;

    if ( DBG_FLAGS( DBG_IO_1 ) ) {
        fprintf(stderr, "CMDhandler_open: host_open(%s, %x, %x, %x, %x)\n",
                 fname, CMD.flags, CMD.mode, cmd->uid, cmd->gid );
    }

    /* has the request failed already? */
    if ( iAmFyod() ) {
      if (cmd->fail == 1) {
          ack.retVal = ( off64_t ) fh;
          ack.hostErrno = EFAULT;

	  send_ack_to_app(mh, &ack);
	  goto done;
        }
    }  

    if ( ( fh =  host_open( fname, CMD.flags, CMD.mode, cmd->uid, cmd->gid ) ) ) {

	ACK.curPos = fh->curPos;
	ACK.isattyFlag = isatty( fh->fd );

#ifdef LINUX_PORTALS
	ACK.srvrNid = ( UINT16 ) _my_pnid;
	ACK.srvrPid = ( UINT16 ) _my_ppid;
	ACK.srvrPtl = ( int )get_app_srvr_portal();
#else
	ACK.srvrNid = ( UINT16 ) _myphysnode();
	ACK.srvrPid = ( UINT16 ) cmd_slot;
	ACK.srvrPtl = ( int )cmd_slot;
#endif /* LINUX_PORTALS */
    }

    ack.retVal = ( off64_t ) fh;
    ack.hostErrno = errno;

    if ( DBG_FLAGS( DBG_IO_1 ) ) {
        printf("CMDhandler_open: send ack %ld %d %d %d %d\n",
            ACK.curPos, ACK.isattyFlag, ACK.srvrNid, ACK.srvrPid,
            ACK.srvrPtl);

        if (fh){
            printf("CMDhandler_open: fh %p\n", fh);
	}
	 else{
            printf("CMDhandler_open: fh %p, errno %d\n",
                     fh, errno);
	}
    }
    send_ack_to_app(mh, &ack);

done:

    if (bufnum >= 0) free_work_buf(bufnum);

    return;
}  
