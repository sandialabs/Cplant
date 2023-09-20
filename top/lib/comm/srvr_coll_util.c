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
** $Id: srvr_coll_util.c,v 1.14 2001/02/11 13:00:57 lafisk Exp $
*/

#include "srvr_lib.h"
#include "srvr_coll.h"
#include <malloc.h>


/*
**********************************************************************
** Fault tolerant collective operations:
**
**    voting
**********************************************************************
*/

/*
** Voting: This blocks until all votes are in, or we timeout.  Votes
** can be accessed with DSRVR_VOTE_VALUE.  Votes disappear after next
** vote.  Vote value is an int.
**
** list : list of ranks of participants in vote, if all participate
**        set list to NULL.
*/

int *dsrvr_ballot = NULL;
static int curBallotSize=0;
int dsrvr_invalVote=0;

int
dsrvr_vote(int voteVal, int tmout, int type, int *list, int listLen)
{
int buflen, rc;
int nblocks;
int myelt;

    buflen = sizeof(int) * dsrvrMembers;

    dsrvr_clear_fail_info();
    dsrvr_failInfo.dsrvrRoutine = DSRVR_VOTE;

    if (buflen > curBallotSize){

        dsrvr_ballot  = (int *)realloc(dsrvr_ballot, buflen);

        curBallotSize = buflen;

        if (!dsrvr_ballot){
	    CPerrno = ENOMEM;
	    return DSRVR_RESOURCE_ERROR;
        }
    }

    if (list){
	myelt = dsrvr_getMyRelativeRank(list, listLen);

	if (myelt < 0){
	    CPerrno = EINVAL;
	    return DSRVR_ERROR;
	}

	nblocks = listLen;
    }
    else{
	myelt = dsrvrMyGroupRank;

	nblocks = dsrvrMembers;
    }

    if ((dsrvrMembers == 1) || (list && (listLen == 1))){
        dsrvr_ballot[0]    = voteVal;
	return DSRVR_OK;
    }

    rc = DSRVR_OK;

    dsrvr_ballot[myelt] = voteVal;

    rc = dsrvr_gather((char *)dsrvr_ballot,
		      sizeof(int), nblocks,
		      tmout, type, list, listLen);

    if (rc == DSRVR_OK){
	/*
	** fan out vote buffer
	*/
        rc = dsrvr_bcast((char *)dsrvr_ballot, sizeof(int) * nblocks,
		     tmout, type, list, listLen);
    }

    return rc;
}

/*
**********************************************************************
**  server library helpers
**********************************************************************
*/

int fanInFromNids[MAXFANIN], fanInToNid;
int fanInFromPids[MAXFANIN], fanInToPid;
int fanInSources, fanInTargets;

/*
** Compute the fan in sources and target when all members participate.
** Do this only once after group is formed, in dsrvr_membership_commit.
*/
void
dsrvr_init_fanin_list()
{
int other, otherNid, otherPid, i;

    fanInSources = fanInTargets = 0;

    for (i=1; i < dsrvrMembers; i <<= 1){

        other = dsrvrMyGroupRank ^ i;

        if (other >= dsrvrMembers) continue;

        otherNid = memberNidByRank(other);
        otherPid = memberPidByRank(other);

        if (dsrvrMyGroupRank > other){
            fanInToNid = otherNid;
            fanInToPid = otherPid;
            fanInTargets = 1;    /* at most one */
            break;
        }
        else{
            fanInFromNids[fanInSources] = otherNid;
            fanInFromPids[fanInSources] = otherPid;

            fanInSources++;
        }
    }
}
/*
** Compute the fan in sources and targets when only a subset
** of the group participates.
*/
void
dsrvr_calc_fan_in(int *list, int listLen, int myelt,
            int *nsources, int *srcNids, int *srcPids, /* recv from these guys */
            int *ntargets, int *targetNid, int *targetPid)   /* send to this guy */
{
int i, other, targets, sources;
int otherNid, otherPid;

    if (!list){
       *nsources = fanInSources;

       if (fanInSources){
           memcpy(srcNids, fanInFromNids, sizeof(int) * fanInSources);
           memcpy(srcPids, fanInFromPids, sizeof(int) * fanInSources);
       }

       *ntargets = fanInTargets;

       if (fanInTargets){
           *targetNid = fanInToNid;
           *targetPid = fanInToPid;
       }

       return;
    }

    targets = sources = 0;

    for (i = 1; i < listLen; i <<= 1){

        other = myelt ^ i;

        if (other >= listLen) continue;

        otherNid = memberNidByRank(list[other]);
        otherPid = memberPidByRank(list[other]);

        if (myelt > other){
            *targetNid = otherNid;
            *targetPid = otherPid;

            targets++;   /* one at most */

            break;
        }
        else{

            srcNids[sources] = otherNid;
            srcPids[sources] = otherPid;

            sources++;
        }
    }

    *ntargets = targets;
    *nsources = sources;

    if (__SrvrDebug){

        printf("Fan In: %d sources %d targets\n",sources,targets);
        for (i=0; i<sources; i++) printf("  recv from %d\n",srcNids[i]);
        for (i=0; i<targets; i++) printf("  send to %d\n",targetNid[i]);
    }
}

int
dsrvr_getMyRelativeRank(int *list, int listLen)
{
int i;

    if (!list){
        return dsrvrMyGroupRank;
    }
    else{

        for (i=0; i<listLen; i++){
            if (list[i] == dsrvrMyGroupRank){
                return i;
            }
            else if ( (list[i] < 0) || (list[i] >= dsrvrMembers)){
                CPerrno = EINVAL;
                return -1;
            }
        }
        CPerrno = 0;
        return -1;
    }
}

