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
** pct_group.c - functions performing PCT group operations
*/
#include "puma.h"
#include "appload_msg.h"
#include "bebopd.h"
#include "srvr_err.h"
#include "srvr_coll.h"
#include "srvr_comm.h"
#include "pct_ports.h"
#include "pct.h"
#include "srvr_coll_fail.h"

/*
** PCTs form groups to host a parallel application.  Group communication
** is limited to fanning out load information, fanning in process pids and
** tokens, fanning out signals, and voting on matters of mutual interest.
*/
extern int Dbglevel;

static int pctGroupActive=0;
static int myRank;

int
initialize_pct_group(int *pctList, int npcts, int groupID)
{
int rc, i;

    if (pctGroupActive){
        log_msg("initialize_pct_group: group already active");
        return -1;
    }
    myRank = -1;
    for (i=0; i<npcts; i++){
        if (pctList[i] == (int) _my_pnid){
            myRank = i;
            break;
        }
    }

    if (myRank < 0){
        log_msg("initialize_pct_group: I'm not on the nid list");
        return -1;
    }
    rc = dsrvr_member_init(npcts, myRank, groupID);

    if (rc != DSRVR_OK){
        return -1;        
    }

    for (i=0; i<npcts; i++){
        if (i==myRank) continue;

        rc = dsrvr_add_member(pctList[i], PPID_PCT, i);

        if (rc != DSRVR_OK){
            return -1;        
        }
    }
    if (Dbglevel){
        log_msg("go into membership commit");
    }

    rc = dsrvr_membership_commit(20.0);

    if (Dbglevel){
        log_msg("out of membership commit, rc %d, I am %d",rc,dsrvrMyGroupRank);
        for (i=0; i<dsrvrMembers; i++){
            log_msg("  rank %d  %d/%d",i,
               memberNidByRank(i), memberPidByRank(i));
        }
    }

    if (rc != DSRVR_OK){
	if ( (CPerrno == ERECVTIMEOUT) || (CPerrno == ESENDTIMEOUT)) {
	    log_msg("%s",dsrvr_who_failed());
	    send_group_failure_to_yod(current_proc_entry(), 0 );
	}
	else {
	    send_failure_to_yod(current_proc_entry(), 0, LAUNCH_ERR_PORTAL_ERR);
	}

        return -1;
    }
    pctGroupActive = 1;

    return 0;
}
int
takedown_pct_group()
{
    if (pctGroupActive){
        pctGroupActive = 0;
    }
    dsrvr_member_done();
    return 0;
}
int
fanout_control_message(int msg_type, char *user_data, int len, 
                        unsigned int fanout_degree, 
                        int groupRoot, int groupSize)
{
int chRank[MAX_FANOUT_DEGREE], nkids, i, rc;

    if (fanout_degree > MAX_FANOUT_DEGREE){
        fanout_degree = MAX_FANOUT_DEGREE;
    }
    
    for (i=0, nkids=0; i<(int) fanout_degree; i++){
        chRank[nkids] = TREE_CHILD(dsrvrMyGroupRank-groupRoot, fanout_degree, i);
        if (chRank[nkids] < groupSize){
            nkids++;
        }
    }

    for (i=0; i<nkids; i++){

        chRank[i] += groupRoot;

        rc = srvr_send_to_control_ptl(memberNidByRank(chRank[i]),
                                 memberPidByRank(chRank[i]),
                                 PCT_LOAD_PORTAL,
                                 msg_type, user_data, len);

        if (rc){
            log_warning("failure to send %x to %d/%d", msg_type,
                            memberNidByRank(chRank[i]),
                             memberPidByRank(chRank[i]));
	    
	     dsrvr_clear_fail_info();
	     dsrvr_failInfo.last_nid = memberNidByRank(chRank[i]);
	     dsrvr_failInfo.last_pid = memberPidByRank(chRank[i]);
	     dsrvr_failInfo.operation = SEND_OP;
	     dsrvr_failInfo.what      = WHAT_NOT_SET;

	     send_group_failure_to_yod(current_proc_entry(), 0 );

             return -1;
        }
    }
    return 0;
}
