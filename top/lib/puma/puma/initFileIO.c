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
/*
$Id: initFileIO.c,v 1.6 2001/02/16 05:39:25 lafisk Exp $
*/

#include <stdio.h>
#include <fcntl.h>
#include "puma.h"
#include "fdTable.h"

TITLE(initFileIO_c, "@(#) $Id: initFileIO.c,v 1.6 2001/02/16 05:39:25 lafisk Exp $");

void createFileBuffer( int fd, int size );

static VOID initFD( int fd, hostReply_t *ack ); 


void
initFileIO( hostReply_t *stdinAck, hostReply_t *stdoutAck, 
													hostReply_t *stderrAck)
{
	int num; 
	initFD( 0, stdinAck );
	initFD( 1, stdoutAck );
	initFD( 2, stderrAck );
	FD_FILE_STATUS_FLAGS( 0 ) = O_RDONLY;
	FD_FILE_STATUS_FLAGS( 1 ) = O_WRONLY;
	FD_FILE_STATUS_FLAGS( 2 ) = O_WRONLY;

    for (num = 3; num < _NFILE; num++) 
	{
		FD_ENTRY( num ) = NULL;
	}
}

static VOID initFD( int fd, hostReply_t *ack ) 
{
	
    FD_CLOSE_ON_EXEC_FLAG( fd ) = FD_CLOEXEC; 

	FD_ENTRY( fd ) =  createFdTableEntry( );

	/* Fill in the server that will handle requests for this file */
	FD_ENTRY_SRVR_NID( fd ) = ack->info.openAck.srvrNid;
	FD_ENTRY_SRVR_PID( fd ) = ack->info.openAck.srvrPid;
	FD_ENTRY_SRVR_PTL( fd ) = ack->info.openAck.srvrPtl;
					  
	FD_ENTRY_HOST_FILE_INDEX( fd ) = ack->retVal;
	FD_ENTRY_CURPOS( fd ) = ack->info.openAck.curPos;
	FD_ENTRY_IS_TTY( fd ) = ack->info.openAck.isattyFlag;
	FD_ENTRY_REFCOUNT( fd ) = 1;

	FD_ENTRY_PROTOCOL(fd) = YOD_IO_PROTO;

}
										 
