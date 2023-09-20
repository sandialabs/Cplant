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
#ifndef SRVR_COLL_MEMBERSH
#define SRVR_COLL_MEMBERSH

#define DEFAULT_MAX_MEMBERS   25

/*
** Member ID
*/
enum {
unused=0,     
activeMember,
inActiveMember,
timedOutOnCollectiveOp,
bizarreOnCollectiveOp,
joining,
leaving
} memberStatuses;

typedef struct _dsrvrMemberID{
    int nid;
    int pid;
    int realRank; /* unchanging rank of this member */
    int rank;     /* rank in contiguous list of members */
    int status;
} dsrvrMemberID;

typedef struct _dsrvrMemberState{
    dsrvrMemberID *mid;
    int *RealRank;       /* index into mid array */
    int len;
} dsrvrMemberState;

/*
** exported by membership module
*/
extern int dsrvrMaxMembers; /* max members allowed       */
extern int dsrvrMembers;    /* current number of members */
extern int dsrvrMyGroupRank;/* these are contiguous, 0 to p-1, may change */
extern int dsrvrMyRealRank; /* match file reg, not nec. contig, never changes*/
extern int dsrvrMembersInited;
extern int dsrvrGroupId;

void dsrvr_member_done(void);
int dsrvr_member_init(int maxm, int myRealRank, int groupId);
int dsrvr_add_member(int nid, int pid, int realRank);
int dsrvr_membership_commit(double timeout);

void memberTimedOutOnCollectiveOp(int rank);
void memberBizarreOnCollectiveOp(int rank);
int memberNidByRank(int rank);
int memberPidByRank(int rank);
int memberRankByNidPid(int nid, int pid);
int memberStatusByRank(int rank);
void memberTimedOutOnCollectiveOp(int groupRank);
void memberBizarreOnCollectiveOp(int groupRank);

#endif
