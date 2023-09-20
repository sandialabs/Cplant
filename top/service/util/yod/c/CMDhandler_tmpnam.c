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
/* $Id: CMDhandler_tmpnam.c,v 1.3 2001/02/16 05:42:22 lafisk Exp $ */ 

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "util.h"
#include "CMDhandlerTable.h"
#include "host_msg.h"


/******************************************************************************/

VOID CMDhandler_tmpnam( int buf_num, hostCmd_t *cmd, UINT16 snid, UINT16 spid)
{

	hostReply_t 	*ack;
	char			*name;
	INT32 			sz;

    if (DBG_FLAGS(DBG_IO_1))
	{
		fprintf(stderr,"\nCMDhandler_tmpnam( nid %i, pid %i )\n", snid, spid );
	}

	name = tmpnam( NULL );

	if ( name )
	{
		strcpy( workBufData(buf_num), name );
	}

	/* we need to write this even if the tmpnam failed */
    if ( ( sz = host_writemem(buf_num, L_tmpnam, cmd->write_portal, 
			0, snid, spid, 0) ) != L_tmpnam )   
	{
		fprintf(stderr, "CMDhandler_tmpnam send data error: data size %d\n", sz);
	}

    ack = (hostReply_t *) workBufData(buf_num);
    ack->hostErrno = errno;
	ack->retVal = (long) name;

    if ( ( sz = host_writemem(buf_num, sizeof(hostReply_t), cmd->ack_portal,
	       0, snid, spid, 0) ) != sizeof(hostReply_t))   
	{
		fprintf(stderr, "CMDhandler_tmpnam() send ack error: acksz %d\n", sz);
    }
}  

