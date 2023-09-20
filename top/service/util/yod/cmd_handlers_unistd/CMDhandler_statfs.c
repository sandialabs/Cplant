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
/* $Id: CMDhandler_statfs.c,v 1.6 2001/02/16 05:44:25 lafisk Exp $ */


#include <sys/types.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "CMDhandlerTable.h"
#include "util.h"
#include "host_msg.h"
#include "fillWorkBuf.h"
#include "config.h"
#include "srvr_comm.h"

#ifdef LINUX_PORTALS
#include <unistd.h>
#ifdef __linux__
#include <sys/vfs.h>
#endif
#ifdef __osf__
#include <sys/statvfs.h>
#endif
#endif



/******************************************************************************/

VOID CMDhandler_statfs( control_msg_handle *mh)
{
hostReply_t     ack;
INT32             retVal;
CHAR            fname[MAXPATHLEN+1];
int gid_real = getgid();
hostCmd_t   *cmd;
int         rc, snid, spid, len, bufnum=-1;

    CMD_SET_VALS(snid,spid,cmd,bufnum,len,mh)
    
    if (DBG_FLAGS(DBG_IO_1))
    {
        fprintf(stderr,"CMDhandler_statfs() nid %i pid %i\n", snid, spid);
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

    setreuid( -1, cmd->euid );
    setgid( cmd->gid );

    retVal = statfs( fname, (struct statfs *) workBufData(bufnum) );

    setreuid( -1, getuid() );
    setgid( gid_real );

    ack.your_seq_no = cmd->my_seq_no;
    ack.hostErrno = errno;
    ack.retVal = retVal;

    send_workbuf_and_ack(mh, bufnum, sizeof(struct statfs), &ack);

done:

    if (bufnum >= 0) free_work_buf(bufnum);

    return;
}  

