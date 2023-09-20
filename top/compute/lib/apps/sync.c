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
/* $Id: sync.c,v 1.9 2001/08/14 15:36:15 pumatst Exp $ */

#include <unistd.h>
#include "puma_unistd.h"
#include "puma.h"
#include "errno.h"
#include "rpc_msgs.h"

#ifdef LINUX24
#define LINUX_RETVAL VOID
#else
#define LINUX_RETVAL INT32
#endif

/******************************************************************************/

#if defined(__linux__)
LINUX_RETVAL
#else
VOID
#endif
__sync(VOID)
{
#if defined(__linux__)
    return
#endif
    sync();
}


#if defined(__linux__)
LINUX_RETVAL
#else
VOID
#endif
sync(VOID)
{
hostCmd_t 	cmd;
hostReply_t	ack;

    cmd.uid = getuid();
    cmd.gid = getgid();
    cmd.euid = geteuid();
    cmd.egid = getegid();

    if (  _host_rpc(&cmd, &ack, 
	    CMD_SYNC, YOD_NID, YOD_PID, YOD_PTL, NULL, 0, NULL, 0) != 0 ) {

        /* could not send! Use errno set by _host_rpc() */
#if defined(__linux__)
        return 0;
#else
        return;
#endif
    }

#if defined(__linux__)
    return 0;
#endif

}  /* end of sync() */
