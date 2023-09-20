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
** $Id: srvr_coll_util.c,v 1.2 2000/11/22 20:56:34 lafisk Exp $
*/

#include "puma.h"
#include "puma_errno.h"
#include "sysptl.h"
#include "srvr_comm.h"
#include "srvr_err.h"
#include "srvr_coll.h"
#include "srvr_coll_fail.h"
#include "srvr_coll_membership.h"
#include <string.h>
#include <malloc.h>

extern double dclock(void);

static int send_system_control_message(int msgtype, int nid, int pid, 
                                       int msglen, void *msg);
static int get_next_system_control_message(int *msgtype, int *srcnid, 
   int *srcpid, int *msglen, char **data, double timeout, long *which);
static int free_system_control_message(long which);
static int reset_system_control_portal(void);

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
** list : list of ranks of participants in vote, if all participates
**        set list to NULL.
*/

int *dsrvr_ballot = NULL;
static int curBallotSize=0;
int dsrvr_invalVote;

int
dsrvr_vote(int voteVal, double tmout, int type, int *list, int listLen)
{
int buflen, rc;

    buflen = sizeof(int) * 2 * dsrvrMembers;

    dsrvr_clear_fail_info();
    dsrvr_failInfo.dsrvrRoutine = DSRVR_VOTE;

    if (buflen > curBallotSize){

	dsrvr_invalVote = 0;

        dsrvr_ballot = (int *)realloc(dsrvr_ballot, buflen);

        curBallotSize = buflen;

        if (!dsrvr_ballot){
	    CPerrno = ENOMEM;
	    return DSRVR_RESOURCE_ERROR;
        }

	rc = portal_lock_buffer(dsrvr_ballot, buflen);

	if (rc){
	    CPerrno = ELOCK;
	    return DSRVR_RESOURCE_ERROR;
	}
    }

    memset(dsrvr_ballot, 0, buflen);

    dsrvr_ballot[dsrvrMyGroupRank*2]        = 0x0f0f0f0f;
    dsrvr_ballot[(dsrvrMyGroupRank*2)+1]    = voteVal;

    if (dsrvrMembers == 1){
	return 0;
    }
    rc = DSRVR_OK;

    rc = dsrvr_gather((char *)dsrvr_ballot,
		      sizeof(int) * 2, dsrvrMembers,
		      tmout, type, list, listLen);

    if (rc == DSRVR_OK){
	/*
	** fan out vote buffer
	*/
        rc = dsrvr_bcast((char *)dsrvr_ballot, sizeof(int) * 2 * dsrvrMembers,
		     tmout, type, list, listLen);
    }

    return rc;
}
/*
**********************************************************************
**  Fan in / fan out using the SYS_CTRL_PTL, which always exists
**  at a well known location for every process linked with startup.c
**  code.  So this routine can be used before collection communication
**  is established.
**
**  This routine assumes there's a valid membership list.
**********************************************************************
*/
int
dsrvr_system_synchronization(int fanout_degree, int msgtype, double timeout)
{
int chRank[MAX_FANOUT_DEGREE];
int chNid[MAX_FANOUT_DEGREE],chPid[MAX_FANOUT_DEGREE];
int nchildren, parent, target, i, ii, status, rc;
long handle;
int rank, srcnid, srcpid;
int mtype, nid, pid;

    if ( (fanout_degree <= 0) || (fanout_degree > MAX_FANOUT_DEGREE) ||
          (timeout < 0.0)){
        CPerrno = EINVAL;
	log_warning("dsrvr_system_synchronization: invalid arguments");
        return DSRVR_ERROR;
    }
    status = DSRVR_OK;

    dsrvr_clear_fail_info();
    dsrvr_failInfo.dsrvrRoutine = DSRVR_SYS_SYNC;
    dsrvr_failInfo.ptl      = SYS_CTRL_PTL;
    
    if (dsrvrMembers == 1){
	return status;
    }
        
    nchildren = 0;

    for (i=0; i<fanout_degree; i++){
 
        target = TREE_CHILD(dsrvrMyGroupRank, fanout_degree, i);
 
        if (target < dsrvrMembers){
            chRank[i] = target;
            chPid[i] = 0;
            nchildren++;
        }
        else{
            break;
        }
    }
 
    if (dsrvrMyGroupRank > 0){
        parent = TREE_PARENT(dsrvrMyGroupRank, fanout_degree);
    }
    else{
        parent = -1;
    }
 
    /*
    ** fan in membership verification
    */
    status = DSRVR_OK;

    dsrvr_failInfo.where = GENERAL_RECV;
 
    for (i=0; i<nchildren; i++){

#ifdef DBGMEMBERSHIP
	log_msg("await fan in on system control portal\n");
#endif

        rc = get_next_system_control_message(&mtype, &srcnid, &srcpid,
                                         NULL, NULL,
                                         timeout, &handle);
 
        if (rc == 0){   /* no new messages found */
            break;
        }
        if (rc == 1){
 
            rank = memberRankByNidPid(srcnid, srcpid);

#ifdef DBGMEMBERSHIP
	    log_msg("\tgot one from rank %d, %d/%d\n",rank,srcnid,srcpid);
#endif
 
            if (rank >= 0){
                for (ii=0; ii<nchildren; ii++){
		    if (chRank[ii] == rank){
			chNid[ii] = srcnid;
			chPid[ii] = srcpid;
			break;
		    }
                }
                if (ii==nchildren){
	            log_msg(
		    "dsrvr_system_synchronization: unexpected token from %d",
		    srcnid);
                    status = DSRVR_EXTERNAL_ERROR;
                }
            }
            else{
                log_msg(
		"dsrvr_system_synchronization: unexpected message from %d/%d",
		     srcnid,srcpid);
		status = DSRVR_EXTERNAL_ERROR;
            }
            if (status != DSRVR_OK){
                break;
            }
 
            if (mtype != msgtype){
                log_msg(
                "dsrvr_system_synchronization: unexpected type %x from %d",
			    msgtype, srcnid);
                status = DSRVR_EXTERNAL_ERROR;
                break;
            }
            free_system_control_message(handle);
        }
        else{
            status = rc;
            break;
        }
    }

    for (i=0; i<nchildren; i++){
        if (chPid[i] == 0){
	    dsrvr_failInfo.last_nid = memberNidByRank(chRank[i]);
            log_msg(
	    "dsrvr_system_synchronization: no token from member %d, nid %d\n",
               chRank[i],memberNidByRank(chRank[i]));
            status = DSRVR_EXTERNAL_ERROR;
	    break;
        }
    }
    if (status != DSRVR_OK){
        reset_system_control_portal();
        return status;
    }
 
    /*
    ** send membership verification to parent
    */
    if (parent >= 0){

	nid = memberNidByRank(parent);
	pid = memberPidByRank(parent);

        dsrvr_failInfo.where = GENERAL_SEND;
        dsrvr_failInfo.last_nid = nid;
        dsrvr_failInfo.last_pid = pid;

	if ( (nid == DSRVR_ERROR) || (pid == DSRVR_ERROR)){
            log_msg("dsrvr_system_synchronization: invalid tree parent?? %d",
		     parent);
            status = DSRVR_RESOURCE_ERROR;
	}
	else{

#ifdef DBGMEMBERSHIP
	    log_msg("Send fan in token to parent %d/%d\n",nid,pid);
#endif

	    rc = send_system_control_message(msgtype, nid, pid,
                                             0, NULL);

            if (rc != DSRVR_OK){
		log_msg(
		"dsrvr_system_synchronization: error sending to parent %d/%d",
		nid,pid);
		status = rc;
	    }
        }
	if (status != DSRVR_OK){
	    return status;
	}
 
	/*
	** await token back from parent
	*/
        dsrvr_failInfo.where = GENERAL_RECV;

#ifdef DBGMEMBERSHIP
	log_msg("Await fan out token from parent, timeout %f\n",timeout);
#endif
 
        rc = get_next_system_control_message(&mtype, &srcnid, &srcpid,
                                         NULL, NULL,
                                         timeout, &handle);
 
        if (rc == 0){   /* no new messages found */
	    log_msg(
	    "dsrvr_system_synchronization: timeout awaiting msg from parent %d",
	    nid);
	    status = DSRVR_EXTERNAL_ERROR;
        }
        else if (rc == 1){
            if ((srcnid != nid) || (srcpid != pid) || (mtype != msgtype)){
		log_msg("dsrvr_system_synchronization: unknown source %d/%d",
			 srcnid,srcpid);
		status = DSRVR_EXTERNAL_ERROR;
	    }
	    else{
#ifdef DBGMEMBERSHIP
		log_msg("got fan out token from parent\n");
#endif

		free_system_control_message(handle);
	    }
	} else{
	    log_msg(
	    "dsrvr_system_synchronization: error awaiting msg from parent %d",nid);
	    status = rc;
	}
    }
    if (status != DSRVR_OK){
        reset_system_control_portal();
        return status;
    }
 
    /*
    ** send token on to children
    */
    dsrvr_failInfo.where = GENERAL_SEND;
 
    for (i=0; i<nchildren; i++){

#ifdef DBGMEMBERSHIP
	log_msg("Fan out token to child %d/%d\n",chNid[i], chPid[i]);
#endif
	dsrvr_failInfo.last_nid = chNid[i];
	dsrvr_failInfo.last_pid = chPid[i];
 
        status = send_system_control_message(msgtype,
                   chNid[i], chPid[i], 0, NULL);

	if (status != DSRVR_OK){
	    log_msg(
	    "dsrvr_system_synchronization: error sending to child %d",chNid[i]);
	    break;
	}
    }

    return status;
}
/*
**********************************************************************
**  The SYS_CTRL_PTL is setup at a well known location before main.
**  It can be used for synchronizing when state of collective comm
**  buffers at other members is unknown.
**********************************************************************
*/
static int
send_system_control_message(int msgtype, int nid, int pid, int msglen,
			    void *msg)
{
int status, rc;
SEND_INFO_TYPE s_info;

    status = DSRVR_OK;

    memset(&s_info, 0, sizeof(SEND_INFO_TYPE));

    s_info.dst_matchbits.ints.i0 = msgtype;

    rc = send_user_msg_phys((char *)msg, msglen, 
			    nid, pid, SYS_CTRL_PTL, &s_info);

    if (rc){
	log_msg("send_system_control_message: error on msg send %d\n",rc);
	status = DSRVR_RESOURCE_ERROR;
    }
    else{
	rc = srvr_await_send_completion(&(s_info.send_flag));
	if (rc){
	    CPerrno = ESENDTIMEOUT;
            status = DSRVR_RESOURCE_ERROR;
	}
    }
    
    return status;
}
/*
** timeout == 0  - non blocking
** timeout > 0   - block up to timeout seconds for incoming message
**
** return values: 0   no new messages
**                1   found a new message
**                < 0 error
*/
static int
get_next_system_control_message(int *msgtype, int *srcnid, int *srcpid,
	      int *msglen, char **data, double timeout, long *which)
{
int rc, status;
IND_MD_BUF_DESC *buf_hdr;
PTL_MSG_HDR *hdr;
double t1;

    if (timeout < 0.0){
	CPerrno = EINVAL;
	return DSRVR_ERROR;
    }

    t1 = dclock();

    buf_hdr = NULL;

    do{

	rc = sptl_ind_msg_probe(&(IND_MD(SYS_CTRL_PTL)), IND_CIRC_SV_HDR_BDY,
			     &buf_hdr, NONBLOCKING);

        if (rc != ESUCCESS){
	     CPerrno = EPORTAL;
	     return DSRVR_RESOURCE_ERROR;
	}
	if (timeout == 0.0){
	    break;
	}

    } while (((dclock() - t1) < timeout) && (buf_hdr == NULL));
    
    if (buf_hdr){
        hdr = &(buf_hdr->hdr);

        if (msgtype){
	    *msgtype = hdr->dst_mbits.ints.i0;
	}
	if (srcnid){
	    *srcnid  = hdr->src_nid;
	}
	if (srcpid){
	    *srcpid  = hdr->src_pid;
	}
	if (msglen){
	    *msglen  = hdr->msg_len;
	}
        if (data){
	    *data = buf_hdr->buf;
	}
        if (which){
	    *which = (buf_hdr - IND_MD(SYS_CTRL_PTL).buf_desc_table);
	}
	else{
	    sptl_ind_msg_free(&(IND_MD(SYS_CTRL_PTL)), IND_CIRC_SV_HDR_BDY,
			         buf_hdr);
	}

	status = 1;
    }
    else{
	status = 0;
    }

    return status;
}
static int
free_system_control_message(long which)
{
int rc, status;

    status = DSRVR_OK;

    if ( (which < 0) || (which >= IND_MD(SYS_CTRL_PTL).num_buf_desc)){
	CPerrno = EINVAL;
	return DSRVR_ERROR;
    }

    rc = sptl_ind_msg_free(&(IND_MD(SYS_CTRL_PTL)), IND_CIRC_SV_HDR_BDY,
			    IND_MD(SYS_CTRL_PTL).buf_desc_table + which);

    if (rc != ESUCCESS){
	status = DSRVR_RESOURCE_ERROR;
    }

    return status;
}
/*
** A function to set the SYSTEM CONTROL PORTAL back to it's initial
** state, useful on cleaning up after a misunderstanding.
*/
static int
reset_system_control_portal()
{
int i, num_buf_desc;
IND_MD_BUF_DESC *buf_desc_table;
PTL_MSG_HDR *header;

    SPTL_DEACTIVATE(SYS_CTRL_PTL);

    PTL_CLR_FLAG(SYS_CTRL_PTL, &(_my_pcb->portals_dropped));
    PTL_CLR_FLAG(SYS_CTRL_PTL, &(_my_pcb->portals_pending));

    buf_desc_table = IND_MD(SYS_CTRL_PTL).buf_desc_table;
    num_buf_desc   = IND_MD(SYS_CTRL_PTL).num_buf_desc;

    buf_desc_table[0].first_read = 0;
    buf_desc_table[0].last_probe = -1;
    buf_desc_table[0].next_free  = 0;
    buf_desc_table[0].ref_count  = 1;

    for (i=0; i<num_buf_desc; i++) {
	header = &(buf_desc_table[i].hdr);
	header->msg_len = -1;

#ifdef LINUX_PORTALS
	buf_desc_table[i].stage = 0;
#endif
    }
    SPTL_ACTIVATE(SYS_CTRL_PTL);

    return 0;
}
