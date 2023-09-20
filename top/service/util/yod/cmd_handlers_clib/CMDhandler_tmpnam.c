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
/* $Id: CMDhandler_tmpnam.c,v 1.5 2001/02/16 05:44:19 lafisk Exp $ */


#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "util.h"
#include "CMDhandlerTable.h"
#include "host_msg.h"
#include "srvr_comm.h"




/******************************************************************************/

VOID CMDhandler_tmpnam( control_msg_handle *mh)
{

hostReply_t     ack;
char            *name;
int         bufnum=-1, len, snid, spid;
hostCmd_t   *cmd;

    CMD_SET_VALS(snid, spid, cmd, bufnum, len, mh);

    if (DBG_FLAGS(DBG_IO_1))
    {
        fprintf(stderr,"\nCMDhandler_tmpnam( nid %i, pid %i )\n", snid, spid );
    }
    if (bufnum < 0){
         ioErrno = ERESOURCE;
         goto done;
    }

    name = tmpnam( NULL );

    if ( name )
    {
        strcpy( workBufData(bufnum), name );
    }

    ack.your_seq_no = cmd->my_seq_no;
    ack.hostErrno = errno;
    ack.retVal = (long) name;

    send_workbuf_and_ack(mh, bufnum,
               strlen(workBufData(bufnum)) + 1, &ack);

done:

    if (bufnum >= 0) free_work_buf(bufnum);

    return;

}  

