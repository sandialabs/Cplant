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
 

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include "util.h"
#include "CMDhandlerTable.h"
#include "host_msg.h"
#include "fillWorkBuf.h"

TITLE(CMDhandler_tempnam_c, "@(#) $Id: CMDhandler_tempnam.c,v 1.2 2001/02/16 05:42:22 lafisk Exp $");



/******************************************************************************/

VOID CMDhandler_tempnam( int buf_num, hostCmd_t *cmd, UINT16 snid, UINT16 spid)
{

#undef CMD
#define CMD	(cmd->info.tempnamCmd)

	hostReply_t 	*ack;
	INT32 			sz;
	char			*new;

	if (DBG_FLAGS(DBG_IO_1))
	{
		fprintf(stderr,"\nCMDhandler_tempnam( nid %i, pid %i )\n", snid, spid );
	}

	if ( fillWorkBuf( buf_num, CMD.directoryLen, 0,
			snid, spid, cmd->read_portal) == -1 )
	{
		/* Should abort */
		fprintf( stderr, "CMDhandler_tempnam: Error in workbuffer\n" );
	}

	new = (char *) tempnam( workBufData( buf_num ), CMD.prefix );

	if ( new )
	{
		strcpy( workBufData( buf_num ), new );
		free(new); 
	}

	/* we need to write this even if the tempnam failed */
    if ( ( sz = host_writemem(buf_num, MAXPATHLEN, cmd->write_portal, 
			0, snid, spid, 0) ) != MAXPATHLEN )   
	{
		fprintf(stderr, "CMDhandler_tempnam send data error: data size %d\n", sz);
	}

    ack = (hostReply_t *)workBufData( buf_num );
	ack->retVal = (long)new;
    ack->hostErrno = errno;

    if ( ( sz = host_writemem(buf_num, sizeof(hostReply_t), cmd->ack_portal,
	       0, snid, spid, 0) ) != sizeof(hostReply_t))   
	{
		fprintf(stderr, "CMDhandler_tempnam() send ack error: acksz %d\n", sz);
    }
}  

