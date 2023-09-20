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
** $Id: srvr_coll_fail.h,v 1.1 2000/11/04 04:08:10 lafisk Exp $
*/

#ifndef SRVRCOLLFAILH
#define SRVRCOLLFAILH

/*    ERROR REPORTING FOR COLLECTIVE COMMUICATION FAILURES
**    ====================================================
** where possibilities
*/
#define PUMA_RECV_SEND_OK    1
#define PUMA_RECV_SINGLE_BLK 2
#define PUMA_RECV_IND_BLK    3
#define PUMA_SEND_DATA_BLIND  4
#define PUMA_SEND_DATA        5
#define PUMA_SEND_AWAIT_HANDSHAKE 6
#define PUMA_SEND  7
#define PUMA_RECV  8
#define GENERAL_SEND 9
#define GENERAL_RECV 10
 
/*
** collcore routines
*/
#define COLL_REDUCE_SHORT  1
#define COLL_BCAST_SHORT   2
#define COLL_GATHER        3
 
/*
** dsrvr routines
*/
#define DSRVR_BCAST      1
#define DSRVR_GATHER     2
#define DSRVR_REDUCE     3
#define DSRVR_BARRIER    4
#define DSRVR_VOTE       5
#define DSRVR_SYS_SYNC   6


/*
** what
*/
#define WHAT_NOT_SET     0
#define BAD_TOKEN        1
#define DUPLICATE_TOKEN  2
#define DUPLICATE_DATA   3
#define UNKNOWN_MEMBER   4
#define NOT_MY_RECEIVER  5
#define TIMEDOUT_WAITING 6

/*
** operation type
*/
#define UNKNOWN_OP 0
#define SEND_OP    1
#define RECEIVE_OP 2

int dsrvr_failed_op(void);
char *dsrvr_who_failed(void);
void dsrvr_clear_fail_info(void);

typedef struct {
    int last_nid;
    int last_pid;
    int ptl;
    int where;
    int what;
    int collRoutine;
    int dsrvrRoutine;
} failInfo;

extern failInfo dsrvr_failInfo;

#endif
