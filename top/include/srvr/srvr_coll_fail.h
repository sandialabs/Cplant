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
** $Id: srvr_coll_fail.h,v 1.4 2001/03/23 03:02:19 lafisk Exp $
*/

#ifndef SRVRCOLLFAILH
#define SRVRCOLLFAILH

/*    ERROR REPORTING FOR COLLECTIVE COMMUICATION FAILURES
**    ====================================================
*/
 
 
/*
** dsrvr routines
*/
#define DSRVR_NOROUTINE  0
#define DSRVR_BCAST      1
#define DSRVR_GATHER     2
#define DSRVR_REDUCE     3
#define DSRVR_BARRIER    4
#define DSRVR_VOTE       5


/*
** what
*/
#define WHAT_NOT_SET     0
#define UNKNOWN_MEMBER   1
#define TIMEDOUT_WAITING 2
#define PROTOCOL_ERROR   3
#define CORRUPT_MESSAGE  4 

/*
** operation type
*/
#define UNKNOWN_OP 0
#define SEND_OP    1
#define RECEIVE_OP 2

char *dsrvr_who_failed(void);
void dsrvr_clear_fail_info(void);

typedef struct {
    int last_nid;
    int last_pid;
    int operation;
    int what;
    int dsrvrRoutine;
} failInfo;

extern failInfo dsrvr_failInfo;
extern char *dsrvr_routines[];
extern char *dsrvr_optype[];

#endif
