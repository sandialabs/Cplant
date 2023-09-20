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
/* $Id: gethostname.c,v 1.12 2001/04/01 01:30:59 pumatst Exp $ */

#define _XOPEN_SOURCE_EXTENDED /* hack for osf */
#define __USE_XOPEN /* being an equal hack employer, a hack for Linux */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "puma.h"
#include "puma_unistd.h"
#include "errno.h"
#include "rpc_msgs.h"

int
__gethostname( char *hname, size_t len)
{
    return gethostname( hname, len);
}

int
gethostname( char *hname, size_t len)
{
#undef CMD
#define CMD     (cmd.info.gethnameCmd)


hostCmd_t	cmd;
hostReply_t 	ack;

    CMD.hnameLen = len;   

    cmd.uid = getuid();
    cmd.gid = getgid();
    cmd.euid = geteuid();
    cmd.egid = getegid();

    if ( _host_rpc(&cmd, &ack, 
	  CMD_GETHOSTNAME, YOD_NID, YOD_PID, YOD_PTL,
	    NULL, 0, hname, len) != 0 ) {

	/* could not send! Use errno set by _host_rpc() */
	return -1;
    }

    if ( ack.retVal == -1 ) {
	    errno = ack.hostErrno;
    }

    return( ack.retVal );

}  /* end of gethostnamE() */
