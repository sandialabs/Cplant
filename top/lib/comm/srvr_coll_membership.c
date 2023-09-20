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
** $Id: srvr_coll_membership.c,v 1.12 2001/11/17 01:23:31 lafisk Exp $
*/
#include "puma.h"
#include "puma_errno.h"
#include "srvr_lib.h"
#include "srvr_coll.h"
#include "srvr_coll_membership.h"
#include <malloc.h>
#include <string.h>

/*
** Distributed service membership functions.
**
** There are two types of groups as far as membership goes.  In
** one type, members may join and leave throughout the life of the
** service and the group continues to function fulfilling client 
** requests and maintaining some sort of global state.  If a member 
** fails, the remaining carry on.
** (Think of the bebopd as an example.)  
**
** In the other type, the members form a group to perform some service,
** and then disband when they are done.  If a member fails, the 
** group must disband.  (Think of the PCTs which form a group to host
** a parallel application.)
**
** So far, only the second type of membership behavior is supported.
** To form a group, the members must learn of one another's identity
** (nid and pid) and rank in the group.  (PCTs learn this from yod.) 
** Ranks need to be unique but not contiguous.   Then:
**
**    dsrvr_member_init         to initialize membership functions
**    dsrvr_member_add          to add each member
**    dsrvr_membership_commit   to go into global op that verifies
**                                   all members agree on membership.
**
** Now, assuming you have called dsrvr_comm_init, you can use the 
** collective communication routines in srvr_coll_comm.c.
**
** To tear down the group:
**
**    dsrvr_member_done
**
** Return codes indicate fault recovery required:
**
**    DSRVR_OK    - operation completed successfully
**    DSRVR_ERROR - invalid arguments to call, but no server recovery required
**    DSRVR_RESOURCE_ERROR - local failure in portal use, memory allocation etc,
**                           better leave distributed service.
**    DSRVR_EXTERNAL_ERROR - failure of an external member of distributed
**                           service, go into abnormal completion protocol
**
**    CPerrno is set with more information, printf("%s",CPstrerror(CPerrno))
**    to display error string.
**
*/
int dsrvrMaxMembers; /* max members allowed       */
int dsrvrMembers;    /* current number of members */
int dsrvrMyGroupRank;/* these are contiguous, 0 to p-1, may change */
int dsrvrMyRealRank; /* not nec. contig, don't change when members come & go */
int dsrvrMembersInited=0;
int dsrvrGroupId;

static dsrvrMemberState MemberList = {NULL, NULL, 0};

void
dsrvr_member_done()
{
    if (dsrvrMembersInited != 0){

        if (MemberList.mid)      free(MemberList.mid);
        if (MemberList.RealRank) free(MemberList.RealRank);

        memset(&MemberList, 0, sizeof(dsrvrMemberState));
	dsrvrMembersInited = 0;
	dsrvrMembers = 0;
    }
}
static void
reset_MemberList(void)
{
    if (MemberList.mid)      free(MemberList.mid);
    if (MemberList.RealRank) free(MemberList.RealRank);
    MemberList.mid = NULL;
    MemberList.RealRank = NULL;
    MemberList.len = 0;
}

int
dsrvr_member_init(int maxm, int myRealRank, int groupId)
{
char *newPtr;

/*
** initialize a BARRIER_PORTAL control for barriers
*/

    CLEAR_ERR;

    if ( (maxm < 0) || (myRealRank < 0) || (myRealRank >= maxm) ||
	dsrvrMembersInited){

	CPerrno = EINVAL;
        log_warning("dsrvr_member_init %d %d %d",
		    maxm, myRealRank, dsrvrMembersInited);
	return DSRVR_ERROR;
    }
    else if (maxm){
        dsrvrMaxMembers = maxm;
    }  
    else{
	dsrvrMaxMembers = DEFAULT_MAX_MEMBERS;
    }

    dsrvrGroupId = groupId;

    if (maxm > MemberList.len){

	newPtr = (char *)realloc(MemberList.mid, maxm*sizeof(dsrvrMemberID));

	if (!newPtr){
	    reset_MemberList();
	    CPerrno = ENOMEM;
	    log_warning("dsrvr_member_init");
	    return DSRVR_RESOURCE_ERROR;
	}
        else{
	    MemberList.mid = (dsrvrMemberID *)newPtr;
        }

	newPtr = (char *)realloc(MemberList.RealRank, maxm*sizeof(int));

	if (!newPtr){
	    reset_MemberList();
	    CPerrno = ENOMEM;
	    log_warning("dsrvr_member_init");
	    return DSRVR_RESOURCE_ERROR;
	}
	else{
	    MemberList.RealRank = (int *)newPtr;
	}
	MemberList.len = maxm;
    }
    memset((char *)MemberList.mid, 0, maxm*sizeof(dsrvrMemberID));
    memset((char *)MemberList.RealRank, 0, maxm*sizeof(int));

    dsrvrMembers = 0;
    dsrvrMyGroupRank = -1;

    MemberList.mid[myRealRank].nid = _my_pnid;
    MemberList.mid[myRealRank].pid = _my_ppid;
    MemberList.mid[myRealRank].realRank = myRealRank;
    MemberList.mid[myRealRank].rank     = 0;
    MemberList.mid[myRealRank].status = joining;

    dsrvrMyRealRank = myRealRank;

    dsrvrMembersInited = 1;

    return DSRVR_OK;
}
int
dsrvr_add_member(int nid, int pid, int realRank)
{
    CLEAR_ERR;

    if ((realRank < 0) || (realRank >= dsrvrMaxMembers) || !dsrvrMembersInited){
	CPerrno = EINVAL;
	return DSRVR_ERROR;
    }

    MemberList.mid[realRank].nid = nid;
    MemberList.mid[realRank].pid = pid;
    MemberList.mid[realRank].realRank = realRank;
    MemberList.mid[realRank].rank = 0;
    MemberList.mid[realRank].status = joining;

    return DSRVR_OK;
}
#ifdef MEMBERS_LEAVE
/*
** provide nid/pid, or real rank, or group rank of member
** to remove, -1 other arguments
*/
int 
dsrvr_remove_member(int nid, int pid, int realRank, int rank)
{
int idx;

   CLEAR_ERR;

   idx = -1;

   if (!dsrvrMembersInited){
       CPerrno = EINVAL;
       return DSRVR_ERROR;
   }

   if ((realRank >= 0) && (realRank < dsrvrMaxMembers)){
       idx = realRank;
   }
   else if ( (nid > 0) && (pid > 0)){
       for (i=0; i < dsrvrMaxMembers; i++){
	   if ((nid == MemberList.mid[i].nid) &&   
	       (pid == MemberList.mid[i].pid)    ){
	       idx = i;
	       break;
	   }
       }
   }
   else if ((rank >= 0) && (rank < dsrvrMaxMembers)){
       idx = MemberList.RealRank[rank];
   }

   if (idx == -1){
       CPerrno = EINVAL;
       return DSRVR_ERROR;
   }

   MemberList.mid[idx].status = leaving;
}
#endif
/*
** If the collection of active and joining members agree on the
** group membership, then all joining members transition to active
** (and all leaving members would transition to inactive, if we
** allowed members to leave).
**
** Sucessful completion means all members in the list agree on
** the membership list.  It doesn't mean their comm structures are
** set up yet.
**
** If return value is DSRVR_EXTERNAL_ERROR, then abort the group
** formation effort.  (If DSRVR_INTERNAL_ERROR, we would want to
** exit due to our own resource problems.)
*/
int
dsrvr_membership_commit(double timeout)
{
int i, groupRank, rc, rr;

    CLEAR_ERR;

    if (!dsrvrMembersInited){
        CPerrno = EINVAL;
        return DSRVR_ERROR;
    }

    /*
    ** Build group rank list.
    ** Group ranks are contiguous from 0 to dsrvrMembers-1, in order
    ** of increasing realRank.
    */
    for (i=0, groupRank=0; i< dsrvrMaxMembers; i++){

	if ((MemberList.mid[i].status == activeMember) ||
	    (MemberList.mid[i].status == joining)   ){

	       MemberList.RealRank[groupRank] = i;
	       MemberList.mid[i].rank         = groupRank;

	       groupRank++;
	}
    }
    dsrvrMembers     = groupRank;
    dsrvrMyGroupRank = MemberList.mid[dsrvrMyRealRank].rank;

    if (dsrvrMembers == 1){
	MemberList.mid[MemberList.RealRank[0]].status = activeMember; /* me */
	return DSRVR_OK;
    }

    /*
    ** For this barrier to work, all members must have called srvr_coll_init()
    ** prior to me going into the barrier.   
    **
    ** Once we successfully exit this barrier, the group is valid.
    */
    dsrvr_init_fanin_list();

    rc = dsrvr_barrier(timeout, NULL, 0);

    if (rc != DSRVR_OK){
	return rc;
    }

    /*
    ** Mark all members active
    */
    for (i=0; i<dsrvrMembers; i++){
	rr = MemberList.RealRank[i];

	MemberList.mid[rr].status = activeMember;
    }

    return DSRVR_OK;
}
/*
** query functions
*/
void
memberTimedOutOnCollectiveOp(int groupRank)
{
int rr;

    rr = MemberList.RealRank[groupRank];

    log_msg("member %d (%d/%d) timed out on collective op",
	      groupRank,
	      MemberList.mid[rr].nid,
	      MemberList.mid[rr].pid);

    MemberList.mid[rr].status = timedOutOnCollectiveOp;
}
void 
memberBizarreOnCollectiveOp(int groupRank)
{
int rr;

    rr = MemberList.RealRank[groupRank];

    log_msg("member %d (%d/%d) unruly on collective op",
	      groupRank,
	      MemberList.mid[rr].nid,
	      MemberList.mid[rr].pid);

    MemberList.mid[rr].status = bizarreOnCollectiveOp;
}
int 
memberNidByRank(int rank)
{

    CLEAR_ERR;

    if ( (rank<0) || (rank>=dsrvrMembers) || !dsrvrMembersInited){
	CPerrno = EINVAL;
	return DSRVR_ERROR;
    }

    return MemberList.mid[MemberList.RealRank[rank]].nid;
}
int 
memberPidByRank(int rank)
{

    CLEAR_ERR;

    if ( (rank<0) || (rank>=dsrvrMembers) || !dsrvrMembersInited){
	CPerrno = EINVAL;
	return DSRVR_ERROR;
    }

    return MemberList.mid[MemberList.RealRank[rank]].pid;
}
int 
memberStatusbyRank(int rank)
{
    CLEAR_ERR;

    if ( (rank<0) || (rank>=dsrvrMembers) || !dsrvrMembersInited){
	CPerrno = EINVAL;
	return DSRVR_ERROR;
    }

    return MemberList.mid[MemberList.RealRank[rank]].status;
}
int 
memberRankByNidPid(int nid, int pid)
{
int i,idx,rr;

    CLEAR_ERR;

    if (!dsrvrMembersInited){
	CPerrno = EINVAL;
	return DSRVR_ERROR;
    }

    idx = -1;

    for (i=0; i<dsrvrMembers; i++){
	rr = MemberList.RealRank[i];

	if ((MemberList.mid[rr].nid == nid) &&
	     (MemberList.mid[rr].pid == pid)     ){

	     idx = i; 
	     break;
	 }
    }
    return idx;
}

