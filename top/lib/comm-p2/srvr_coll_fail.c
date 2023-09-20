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
** $Id: srvr_coll_fail.c,v 1.1 2000/11/04 03:56:31 lafisk Exp $
*/
#include "srvr_coll_fail.h"
#include <string.h>
#include <stdio.h>


/*    
**    ERROR REPORTING FOR COLLECTIVE COMMUICATION FAILURES
**    ====================================================
*/
 
static char whoFailed[256];
 
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
    dsrvr_failInfo.ptl = -1;
    dsrvr_failInfo.where = 0;
    dsrvr_failInfo.what = 0;
    dsrvr_failInfo.collRoutine = 0;
    dsrvr_failInfo.dsrvrRoutine = 0; 
}
int 
dsrvr_failed_op(void)
{
int optype;

    switch (dsrvr_failInfo.where)
    {
        case PUMA_RECV_SEND_OK:
        case PUMA_RECV_SINGLE_BLK:
        case PUMA_RECV_IND_BLK:
        case PUMA_SEND_AWAIT_HANDSHAKE :
        case PUMA_RECV :
        case GENERAL_RECV :

             optype = RECEIVE_OP;
             break;
 
        case PUMA_SEND_DATA_BLIND:
        case PUMA_SEND_DATA:
        case PUMA_SEND:
        case GENERAL_SEND:

             optype = SEND_OP;
             break;
 
        default:
             optype = UNKNOWN_OP;
             break;
    }
    return optype;
}
char *
dsrvr_who_failed(void)
{
    switch (dsrvr_failInfo.where)
    {
        case PUMA_RECV_SEND_OK:
             sprintf(whoFailed, "dsrvr_puma_recv, ptl %d, handshake from %d/%d",
	dsrvr_failInfo.ptl, dsrvr_failInfo.last_nid, dsrvr_failInfo.last_pid);
             break;
 
        case PUMA_RECV_SINGLE_BLK:
             sprintf(whoFailed,
		"dsrvr_puma_recv, ptl %d, recv into sing blk from %d/%d",
	    dsrvr_failInfo.ptl, dsrvr_failInfo.last_nid, dsrvr_failInfo.last_pid);
             break;
 
        case PUMA_RECV_IND_BLK:
             sprintf(whoFailed,
                       "dsrvr_puma_recv, ptl %d, recv into ind blk from %d/%d",
	   dsrvr_failInfo.ptl, dsrvr_failInfo.last_nid, dsrvr_failInfo.last_pid);
             break;
 
        case PUMA_SEND_DATA_BLIND:
             sprintf(whoFailed, "dsrvr_puma_send, send blind to %d/%d",
			   dsrvr_failInfo.last_nid, dsrvr_failInfo.last_pid);
             break;
 
        case PUMA_SEND_DATA:
             sprintf(whoFailed, "dsrvr_puma_send, send data to %d/%d",
			   dsrvr_failInfo.last_nid, dsrvr_failInfo.last_pid);
             break;
 
        case PUMA_SEND_AWAIT_HANDSHAKE :
             sprintf(whoFailed, "dsrvr_puma_send, ptl %d, no handshake from %d/%d",
	  dsrvr_failInfo.ptl, dsrvr_failInfo.last_nid, dsrvr_failInfo.last_pid);
             break;
 
        default:
             sprintf(whoFailed, "no data on who failed");
             break;
    }
 
    switch (dsrvr_failInfo.dsrvrRoutine)
    {
        case DSRVR_BCAST:
            strcat(whoFailed," (dsrvr_bcast)");
            break;
        case DSRVR_GATHER:
            strcat(whoFailed," (dsrvr_gather)");
            break;
        case DSRVR_REDUCE:
            strcat(whoFailed," (dsrvr_reduce)");
            break;
        case DSRVR_BARRIER:
            strcat(whoFailed," (dsrvr_barrier)");
            break;
        case DSRVR_VOTE:
            strcat(whoFailed," (dsrvr_vote)");
            break;
        case DSRVR_SYS_SYNC:
            strcat(whoFailed," (dsrvr_system_synchronization)");
            break;
    }

    switch (dsrvr_failInfo.collRoutine)
    {
        case COLL_REDUCE_SHORT:
            strcat(whoFailed," (_reduce_short)");
            break;
        case COLL_BCAST_SHORT:
            strcat(whoFailed," (_bcast_short)");
            break;
        case COLL_GATHER:
            strcat(whoFailed," (_gather)");
            break;
    }
 
    switch (dsrvr_failInfo.what){
 
        case BAD_TOKEN:
            strcat(whoFailed, " (bad token)");
            break;
 
        case DUPLICATE_TOKEN:
            strcat(whoFailed, " (duplicate token)");
            break;
 
        case DUPLICATE_DATA:
            strcat(whoFailed, " (duplicate data)");
            break;
 
        case UNKNOWN_MEMBER:
            strcat(whoFailed, " (unknown member)");
            break;
 
        case NOT_MY_RECEIVER:
            strcat(whoFailed, " (not my receiver)");
            break;

        case TIMEDOUT_WAITING:
            strcat(whoFailed, " (timedout waiting)");
            break;
    }
 
    return whoFailed;
}
