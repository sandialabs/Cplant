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
/* $Id: CMDhandler_exit.c,v 1.4 2001/02/16 05:44:19 lafisk Exp $ */

#include <stdio.h>
#include "CMDhandlerTable.h"
#include "util.h"
#include "host_msg.h"
#include "srvr_comm.h"


#ifdef __GNUC__
#  define ATTR_UNUSED __attribute__ ((unused))
#else
#  define ATTR_UNUSED
#endif

extern INT32 nodes_running;
extern INT32 Server_done;


VOID 
CMDhandler_exit(control_msg_handle *mh)
{
int snid, spid;

    snid = SRVR_HANDLE_NID(*mh);
    spid = SRVR_HANDLE_PID(*mh);

    if (DBG_FLAGS(DBG_IO_1)) {
	fprintf(stderr, "CMDhandler_exit() node = %i, pid = %i\n",
			snid,spid);
    }

#ifdef DEBUG_ON
    debug_proc_term( ( spid << 16 ) | snid );
#endif DEBUG_ON

    nodes_running--;
    if (nodes_running <= 0) {
        Server_done= TRUE;
    }
}

