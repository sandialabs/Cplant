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
** $Id: srvr_coll_fail.c,v 1.5 2001/03/23 17:40:25 lafisk Exp $
*/
#include "srvr_coll.h"
#include <string.h>
#include <stdio.h>


/*    
**    ERROR REPORTING FOR COLLECTIVE COMMUICATION FAILURES
**    ====================================================
*/
 
static char whoFailed[256];

char *dsrvr_routines[] = {
"unknown routine",
"dsrvr_bcast",
"dsrvr_gather",
"dsrvr_reduce",
"dsrvr_barrier",
"dsrvr_vote"
};

char *dsrvr_optype[] = {
"unknown type",
"send",
"receive"
};
 
/*
** Codes using dsrvr_ functions can access dsrvr_failInfo to learn
** the nid/pid of the peer involved in the last group communication
** operation.
**
** dsrvr_who_failed returns a string with this information.
*/
failInfo dsrvr_failInfo;

void
dsrvr_clear_fail_info(void)
{
    dsrvr_failInfo.last_nid = -1;
    dsrvr_failInfo.last_pid = -1;
    dsrvr_failInfo.operation = 0;
    dsrvr_failInfo.what = 0;
    dsrvr_failInfo.dsrvrRoutine = 0; 
}
char *
dsrvr_who_failed()
{
    switch (dsrvr_failInfo.operation){
 
        case SEND_OP:
            sprintf(whoFailed, "Sending to node %d, pid %d: ",
	        dsrvr_failInfo.last_nid,
	        dsrvr_failInfo.last_pid);
            break;
 
        case RECEIVE_OP:
            sprintf(whoFailed, "Receiving from node %d, pid %d: ",
	        dsrvr_failInfo.last_nid,
	        dsrvr_failInfo.last_pid);
            break;

        default:
            sprintf(whoFailed, "Unknown operation with node %d, pid %d: ",
	        dsrvr_failInfo.last_nid,
	        dsrvr_failInfo.last_pid);
            break;
    }

    strcat(whoFailed,dsrvr_routines[dsrvr_failInfo.dsrvrRoutine]);
    
    switch (dsrvr_failInfo.what){
 
        case UNKNOWN_MEMBER:
            strcat(whoFailed, " (not part of my group)");
            break;
 
        case PROTOCOL_ERROR:
            strcat(whoFailed, " (didn't obey protocol)");
            break;
 
        case TIMEDOUT_WAITING:
            strcat(whoFailed, " (timed out waiting)");
            break;
 
        case CORRUPT_MESSAGE:
            strcat(whoFailed, " (incoming message failed a check sum)");
            break;
    }
    strcat(whoFailed,"\n");
 
 
    return whoFailed;
}
