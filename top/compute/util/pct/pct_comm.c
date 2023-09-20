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
** $Id: pct_comm.c,v 1.68.2.2 2002/07/10 23:24:34 ktpedre Exp $
**
** Structures and functions for communication with yod, bebopd,
** and app.  Status information (on pct and on app process) is
** maintained here to be sent back on request.
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <puma.h>
#include <time.h>

#include "pct.h"
#include "pct_ports.h"
#include "appload_msg.h"
#include "bebopd.h"
#include "sys_limits.h"
#include "srvr_err.h"
#include "srvr_comm.h"
#include "srvr_coll_fail.h"
#include "cplant.h"

/*
**
**  August 2001 - A little change to support cycle-stealing jobs:
**
**  The PCT's mutually exclusive states used to be
**
**          FREE      - available to run a new job
**          PENDING   - allocated by bebopd to a new job but haven't heard
**                      from yod yet, so haven't started the load yet
**          BUSY      - loading or running a job 
**          TROUBLE   - not running a job, but detected problems on node so
**                         not available either
**
**  PENDING is no longer a state.  A PCT is normally either BUSY or FREE.
**  In either case, it can be pending allocation to a new Cplant job.  So:
**
**          FREE      - not loading or running a job and not in any trouble
**          BUSY      - loading or running a job 
**          TROUBLE   - same as before
**
**  "running a job" means hosting a job which is not yet done, even though
**  the process I started has terminated.  The job may still be running on
**  other nodes.
**
**  When the bebopd wants to allocate a node being used by a cycle-
**  stealing job to a new job, it sends the PCT a NICE_KILL_JOB_REQUEST.
**  The PCT begins to nicely kill the app process by issuing a SIGTERM, 
**  then waiting a while, then issuing a SIGKILL if required. 
**
**  If the PCT is engaged in a NICE_KILL and the bebopd wants
**  to allocate it to a new job, the PCT will set it's pending
**  job ID to that of the new job.  So it is BUSY, but is pending
**  allocation to the new job.
**
**  If the PCT is FREE, and bebopd wants to allocate it, the PCT
**  sets it's pending job ID to that of the new job.  Now it is
**  FREE, but pending allocation to the new job.
**
**
**  In particular, the pending ID is set when:
**
**     bebopd asks if PCT can run a new job AND 
**        the PCT is not pending allocation to another job AND 
**        the PCT is BUSY but is engaged in a NICE_KILL
**
**     bebopd asks if PCT can run a new job AND 
**        the PCT is not pending allocation to another job AND 
**	  the PCT is FREE
**
**  And the pending ID is cleared when:
**
**     yod begins the load with MSG_INIT_LOAD
**
**     bebopd sends a PCT_NOT_ALLOCATED
**
**     bebopd wants to allocate the PCT to another job AND
**       PCT has not heard from yod in PENDING_STATE_TIME_OUT seconds
**     
**
**  Note: We don't allow a NICE_KILL request to be sent to only
**  part of the job.  Every process in the job must get the NICE_KILL,
**  so the job as a whole is killed.  This way, it's safe for the
**  PCT to declare itself available to be allocated to another job
**  if it is in the midst of a NICE_KILL.
*/

/**************   PCT state, application process state ****************/

pct_ID State;                           /* info sent to bebopd */
static time_t cur_proc_start_time;
static time_t cur_proc_end_time;
time_t start_pending_clock;

static void
clear_job_state()
{
    State.rank              = 0;
    State.job_id            = INVAL;
    State.user_status       = 0;
    State.session_id        = INVAL;
    State.parent_id         = INVAL;
    State.job_pid           = INVAL;
    State.euid              = INVAL;
    State.final.exit_code   = 0;
    State.final.term_sig    = 0;
    State.final.terminator  = PCT_NO_TERMINATOR;
    State.elapsed           = 0;
    State.niceKillCountdown = 0;
    State.priority          = 0;

    cur_proc_start_time = 0;
    cur_proc_end_time   = 0;
}
static void
clear_pct_state()
{
    State.status = STATUS_FREE;
    State.pending_jid = INVAL;
    State.pending_sid = INVAL;
    State.reserved    = INVAL;

    start_pending_clock = 0;
}

/********************************************************************************
**   PCT sets up to talk to yod:
**
**      We get control messages from yod (load requests/aborts/etc).
**      We get PUTs from yod (load data).
**      We get a GET message from yod wanting an application pid map.
**      We send control messages (failures/short msgs) to yod.
**
**   PCT sets up to talk to bebopds:
**
**      We send control messages to bebopd (our ID).
**      We receive GET requests from bebopd for status info.
**
**   PCT sets up to talk to apps:
**
**      We send PUTS to app in it's startup (pid map).
**      We get control messages from app (ready message,errors).
**      We send control messages to app (go-main, etc.)
**
**      Apps can send a small amount of data to the pct for logging by
**      using the PCT_DUMP macro which calls out_of_band_pct_message.  
**      These come in to the control portal.
*********************************************************************************/

/*
** what's the most control messages from yods that you expect to roll
** into the pct before we get to consume one?  
*/
static int load_ctl_ptl  = SRVR_INVAL_PTL;

#define MAX_YOD_MSGS (10)
#define MAX_YOD_PUTS (3)
/*
** what's the most control messages from bebopds that you expect to roll
** into the pct before we get to consume one?  How many data buffers
** needed for the get requests?
*/
static int bebop_ctl_ptl = SRVR_INVAL_PTL;
#define MAX_BEBOPD_MSGS (10)
#define MAX_BEBOPD_GETS (5)


static nid_type bnid  = SRVR_INVAL_NID;
static spid_type bpid = SRVR_INVAL_PID;
static int bptl = SRVR_INVAL_PTL;

int
check_bebopd_id(nid_type nid, spid_type pid)
{
    return ((bnid == nid) && (bpid == pid));
}
void
set_bebopd_id(nid_type nid, spid_type pid, int ptl)
{
    bnid = nid; 
    bpid = pid; 
    bptl = ptl; 
}
void
find_bebopd_id(int *nid, int *pid, int *ptl)
{
    *nid = (int)bnid; 
    *pid = (int)bpid; 
    *ptl = (int)bptl; 
}

INT32
initialize_listen(nid_type nid, spid_type pid, int ptl)
{
int rc;

    CLEAR_ERR;

    set_bebopd_id(nid, pid, ptl);

    /*
    ** Set up portals required for YOD communication
    */
    rc = srvr_init_control_ptl_at(MAX_YOD_MSGS, PCT_LOAD_PORTAL);

    if (rc){
        log_msg("initialize yod control portal");
        return -1;
    }
    else{
        load_ctl_ptl = PCT_LOAD_PORTAL;
    }

    if (Dbglevel > 1){
        log_msg("yod control portal %d\n", load_ctl_ptl);
    }

    /*
    ** Set up portals required for TVDSVR communication
    */
    if ( srvr_init_control_ptl_at(1,PCT_TVDSVR_PORTAL) ) {
       log_msg("initialize_list: init tvdsvr ctl portal");
       return -1;
    }
    if (Dbglevel > 1){
        log_msg("tvdsvr control portal %d\n",PCT_TVDSVR_PORTAL);
    }

    /*
    ** Set up portals required for BT communication
    */
    rc = srvr_init_control_ptl_at(10,PCT_BT_PTL);
    if (rc) {
      log_msg("initialize_listen: init bt ctl portal");
      return -1;
    }

    /*
    ** Set up portals required for CGDB communication
    */
    rc = srvr_init_control_ptl_at(10,PCT_CGDB_PTL);
    if (rc) {
      log_msg("initialize_listen: init cgdb ctl portal");
      return -1;
    }

    /*
    ** Set up portals required for BEBOPD communication
    */
    rc = srvr_init_control_ptl_at(MAX_BEBOPD_MSGS, 
                             PCT_STATUS_ACTION_PORTAL);

    if (rc){
        log_msg("initialize bebopd control portal");
        return -1;
    }
    else{
        bebop_ctl_ptl = PCT_STATUS_ACTION_PORTAL;
    }

    if (Dbglevel > 1){
        log_msg("bebopd control portal %d\n", bebop_ctl_ptl);
    }

    clear_pct_state();
    clear_job_state();

    if (Dbglevel >= 1){
        log_msg("will send my info to bebopd nid/pid/ptl %d/%d/%d\n",
                                  bnid, bpid, bptl);
        if (Dbglevel >= 2){
            log_msg("load %d, query %d, status %d, nid/pid %d/%d, portal pid %d",
                 load_ctl_ptl, bebop_ctl_ptl, STATUS_FREE, 
                 _my_pnid, _my_pid, _my_ppid);
        }
    }

    return 0;
}
/*
** what's the most new application control messages that you expect?
*/
static int app_ctl_ptl = SRVR_INVAL_PTL;

#define MAX_OUT_OF_BAND_MSGS  (100)
#define MAX_APP_MSGS (3 + MAX_OUT_OF_BAND_MSGS)
#define MAX_APP_PUTS (3)

INT32
initialize_app_portals(int *ctl_ptl)
{
    CLEAR_ERR;

    app_ctl_ptl = srvr_init_control_ptl(MAX_APP_MSGS);

    if (app_ctl_ptl == SRVR_INVAL_PTL){
        log_warning("initialize application control portal");
        return -1;
    }

    if (Dbglevel > 1){
        log_msg("app control portal %d\n", app_ctl_ptl);
    }
    *ctl_ptl = app_ctl_ptl;

     return 0;
}
/*
** cleanup required when PCT is exiting.
*/
INT32
cleanup_pct()
{
    CLEAR_ERR;

    LOCATION("cleanup_pct","top");

    reset_app_host();

    if (Dbglevel){
       log_msg("pct cleaning up before exit");
    }
    if (load_ctl_ptl != SRVR_INVAL_PTL){
        srvr_release_control_ptl(load_ctl_ptl);
    }
    if (bebop_ctl_ptl != SRVR_INVAL_PTL){
        srvr_release_control_ptl(bebop_ctl_ptl);
    }

    if (bnid != SRVR_INVAL_NID){

        update_status(STATUS_DOWN, NOUPDATE);
        send_to_bebopd(PCT_UPDATE_MSG);
    }

    return 0;
}
/*
** check the portal for a yod message.  return:
**
**     -1 on error
**      0 if there's nothing there
**      1 if there's a message 
*/

INT32
yod_request_check(control_msg_handle *mh)
{
INT32 rc;

    CLEAR_ERR;

    SRVR_CLEAR_HANDLE(*mh);

    rc = srvr_get_next_control_msg(load_ctl_ptl, mh, NULL, NULL, NULL);

    return rc;
}
INT32
free_yod_message(control_msg_handle *mh)
{
INT32 rc;

    CLEAR_ERR;

    rc = srvr_free_control_msg(load_ctl_ptl, mh);

    return rc;
}
INT32
tvdsvr_request_check(control_msg_handle *mh)
{
INT32 rc;
    
    CLEAR_ERR;

    SRVR_CLEAR_HANDLE(*mh);

    rc = srvr_get_next_control_msg(PCT_TVDSVR_PORTAL, mh, NULL, NULL, NULL);

    return rc;
}
INT32
free_tvdsvr_message(control_msg_handle *mh)
{
INT32 rc;

    CLEAR_ERR;

    rc = srvr_free_control_msg(PCT_TVDSVR_PORTAL, mh);

    return rc;
}
INT32
bt_request_check(control_msg_handle *mh)
{
INT32 rc;
    
    CLEAR_ERR;

    SRVR_CLEAR_HANDLE(*mh);

    rc = srvr_get_next_control_msg(PCT_BT_PTL, mh, NULL, NULL, NULL);

    return rc;
}
INT32
free_bt_message(control_msg_handle *mh)
{
INT32 rc;

    CLEAR_ERR;

    rc = srvr_free_control_msg(PCT_BT_PTL, mh);

    return rc;
}
INT32
cgdb_request_check(control_msg_handle *mh)
{
INT32 rc;
    
    CLEAR_ERR;

    SRVR_CLEAR_HANDLE(*mh);

    rc = srvr_get_next_control_msg(PCT_CGDB_PTL, mh, NULL, NULL, NULL);

    return rc;
}
INT32
free_cgdb_message(control_msg_handle *mh)
{
INT32 rc;

    CLEAR_ERR;

    rc = srvr_free_control_msg(PCT_CGDB_PTL, mh);

    return rc;
}
/*
** check the portal for a starting app process message.  return:
**
**     -1 on error
**      0 if there's nothing there
**      1 if there's a message 
*/

INT32
start_request_check(control_msg_handle *req)
{
INT32 rc;

    CLEAR_ERR;

    LOCATION("start_request_check", "get msg");

    SRVR_CLEAR_HANDLE(*req);

    rc = srvr_get_next_control_msg(app_ctl_ptl, req, NULL, NULL, NULL);

    return rc;
}
INT32
free_app_message(control_msg_handle *mh)
{
INT32 rc;

    CLEAR_ERR;

    LOCATION("free_app_msg", "free msg");

    rc = srvr_free_control_msg(app_ctl_ptl, mh);

    return rc;
}
void
clear_app_messages()
{
    srvr_free_all_control_msgs(app_ctl_ptl);
}
/*
** check the portal for a bebopd message.  return:
**
**     -1 on error
**      0 if there's nothing there
**      1 if there's a message 
*/

INT32
bebopd_request_check(control_msg_handle *req)
{
INT32 rc;

    CLEAR_ERR;

    SRVR_CLEAR_HANDLE(*req);

    rc = srvr_get_next_control_msg(bebop_ctl_ptl, req, NULL, NULL, NULL);

    return rc;
}
INT32
free_bebopd_message(control_msg_handle *mh)
{
INT32 rc;

    CLEAR_ERR;

    rc = srvr_free_control_msg(bebop_ctl_ptl, mh);

    return rc;
}
/*****************************************************************************/
/*         pct -> bebopd messages                                            */
/*                                                                           */ 
/* PCT notifies bebopd of it's existence at startup, thereafter responds to  */
/* bebopd requests for it's status (free, busy, child status info, etc.)     */
/*****************************************************************************/
VOID
init_child_job_id(int child_id, int session_id, int parent_id)
{
    State.job_id = child_id;
    State.session_id = session_id;
    State.parent_id = parent_id;
}
VOID
init_child_pid(int child_pid, time_t start_time)
{
    State.job_pid = child_pid;
    State.user_status = STARTED;
    cur_proc_start_time = start_time;
}
VOID
init_child_owner(int child_owner)
{
    State.euid = (unsigned int)child_owner;
}
VOID
init_child_priority(int priority)
{
    State.priority = (unsigned int)priority;
}
VOID
init_child_rank(int rank)
{
    State.rank = rank;
}
VOID
update_terminator(uid_t euid)
{
    State.final.terminator = 
       (unsigned char)((euid == State.euid) ? 
		       PCT_JOB_OWNER:PCT_ADMINISTRATOR);
}
VOID
clear_reservation()
{
    State.reserved = INVAL;
}
VOID
update_reservation(uid_t euid)
{
    State.reserved = euid;
}
int
reservation_violation(uid_t euid)
{
    return ((State.reserved == INVAL) ? 0 : ((State.reserved == euid) ? 0 : 1) );
}
VOID
update_status(int pct_status, int child_status)
{
    if (pct_status != NOUPDATE){
#ifndef STRIPDOWN
        if (pct_status == STATUS_FREE){
#else
        if (pct_status == STATUS_FREE || 
            pct_status == STATUS_ALLOCATED){
#endif
	    clear_job_state();
        }
        State.status = pct_status;
    }

    if (child_status != NOUPDATE){
        if (child_status){
             if (child_status & SENT_TO_MAIN){
                 /*
                 ** don't care about progress through startup
                 ** code once application process hits main
                 */
                 State.user_status &= (~STARTUP_MASK);
             }
             State.user_status |= child_status;

        }
        else{
             State.user_status = 0;
        }
    }
}

int
pct_pending()
{

    if (State.pending_jid != INVAL){

        if (start_pending_clock){

	    if ((time(NULL) - start_pending_clock) > PENDING_STATE_TIME_OUT){

	        stop_pending_for();
            }
        }
	else{
	    start_pending_clock = time(NULL);
	}
    }

    return (State.pending_jid != INVAL);
}
int
pending_for()
{

    if (pct_pending()){
        return State.pending_jid;
    }
    else{
        return -1;
    }

}
void
start_pending_for(int job_id, int session)
{
    if (Dbglevel){
	log_msg("now pending allocation to job %d\n",job_id);
    }

/* in the STRIPDOWN case, bebopd will get these w/ the status update
   on allocation -- we may want bebopd to use them to set its
   status->job_id and status->session_id fields for this node even tho
   true allocation is technically pending... note, pct sets those fields
   on getting INIT_LOAD msg from yod (by calling init_child_job_id()) */ 
    State.pending_jid = job_id;
    State.pending_sid = session;
    start_pending_clock = time(NULL);
}
void
stop_pending_for()
{
    if (Dbglevel){
	log_msg("stop pending allocation for job %d\n",State.pending_jid);
    }

    State.pending_jid = INVAL;
    State.pending_sid = INVAL;
    start_pending_clock = 0;
}
void
restart_pending_timeout()
{
    start_pending_clock = time(NULL);
}

INT32
send_to_bebopd(int type)
{
int rc;

    LOCATION("send_to_bebopd","top");

    if ((bnid == SRVR_INVAL_NID) ||
        (bpid == SRVR_INVAL_PID) ||
        (bptl == SRVR_INVAL_PTL)    ){

        return -1;
    }

    if (cur_proc_start_time){
        if (cur_proc_end_time){
            State.elapsed = cur_proc_end_time - cur_proc_start_time;
        }
        else{
            State.elapsed = time(NULL) - cur_proc_start_time;
        }
    }
    else{
        State.elapsed = 0;
    }
    if (State.status == STATUS_DOWN){
        clear_job_state();
	clear_pct_state();
        State.status = STATUS_DOWN;
    }

    if (type == PCT_INITIAL_MSG){
        DEBUG_STALE_NODES("send initial msg to bebopd");
    }
    else{
        DEBUG_STALE_NODES("send update msg to bebopd");
    }

    if (State.status == STATUS_BUSY){
        State.niceKillCountdown = nice_kill_time_left();
    }

    rc = srvr_send_to_control_ptl(bnid, bpid, bptl, type,
                           (char *)&State, sizeof(pct_ID));

    if (rc){
        log_warning("send to bebopd");
    }
    else{
        DEBUG_STALE_NODES("send DONE");
    }

    return rc;
}

/*****************************************************************************/
/*         pct -> yod messages                                               */
/*                                                                           */ 
/* If the pct receives a message from yod that doesn't make sense, it ignores*/
/* it.  Yod can timeout waiting for a response.  If an exception of some sort*/
/* occurs as a result of a valid yod message, the pct sends yod a failure    */
/* message.                                                                  */
/*                                                                           */
/*****************************************************************************/
int
send_failure_to_yod(proc_list *pl, int upid, int reason)

{
int rc;
launch_failure msg;

    CLEAR_ERR;

    if (!pl){
         return 0;
    }

    LOCATION("send_failure_to_yod","top");

    msg.job_id = pl->job_id;
    msg.rank   = pl->my_rank;
    msg.nid    = _my_pnid;
    msg.pid    = upid;
    
    msg.reason_code = reason;

    memset(&(msg.type), 0, sizeof(msg.type));

    rc = srvr_send_to_control_ptl(pl->srvr.nid, pl->srvr.pid, pl->srvr.ptl,
                LAUNCH_FAILURE_MSG, (char *)&msg, sizeof(launch_failure));
    
    if (rc){
        log_warning("Error sending failure %d to to %d/%d/%d\n",
                   reason,
                   pl->srvr.nid, pl->srvr.pid, pl->srvr.ptl);
    }
 
    return 0;
}

int
send_child_failure_to_yod(proc_list *pl, int upid,
                    int reason, int return_code, int term_sig)

{
int rc;
launch_failure msg;

    CLEAR_ERR;

    if (!pl){
        return 0;
    }

    LOCATION("send_child_failure_to_yod","top");

    msg.job_id = pl->job_id;
    msg.rank   = pl->my_rank;
    msg.nid    = _my_pnid;
    msg.pid    = upid;
    
    msg.reason_code = reason;

    msg.type.child.Errno = 0;    /* child didn't die gracefully and send */
    msg.type.child.CPerrno = 0;  /* us it's errno values                 */

    msg.type.child.reason_code = ((term_sig << 16) | return_code);

    rc = srvr_send_to_control_ptl(pl->srvr.nid, pl->srvr.pid, pl->srvr.ptl,
                LAUNCH_FAILURE_MSG, (char *)&msg, sizeof(launch_failure));
    
    if (rc){
        log_warning("Error sending failure %d to to %d/%d/%d\n",
                   reason,
                   pl->srvr.nid, pl->srvr.pid, pl->srvr.ptl);
    }
 
    return 0;
}
int
send_child_nack_to_yod(proc_list *pl, int upid, child_nack_msg *nmsg)
{
int rc;
launch_failure msg;

    CLEAR_ERR;

    if (!pl){
        return 0;
    }

    LOCATION("send_child_nack_to_yod","top");

    msg.job_id = pl->job_id;
    msg.rank   = pl->my_rank;

    msg.nid = _my_pnid;
    msg.pid  = upid;
    
    msg.reason_code = LAUNCH_ERR_CHILD_NACK;

    msg.type.child.Errno       = nmsg->lerrno;
    msg.type.child.CPerrno     = nmsg->CPerrno;
    msg.type.child.reason_code = nmsg->start_log;

    rc = srvr_send_to_control_ptl(pl->srvr.nid, pl->srvr.pid, pl->srvr.ptl,
                LAUNCH_FAILURE_MSG, (char *)&msg, sizeof(launch_failure));
    
    if (rc){
        log_warning("Error sending child nack to %d/%d/%d\n",
                   pl->srvr.nid, pl->srvr.pid, pl->srvr.ptl);
    }
 
    return 0;
}
int
send_group_failure_to_yod(proc_list *pl, int upid)

{
int rc;
launch_failure msg;

    CLEAR_ERR;

    if (!pl){
        return 0;
    }

    LOCATION("send_group_failure_to_yod","top");

    msg.job_id = pl->job_id;
    msg.rank   = pl->my_rank;
    msg.nid = _my_pnid;
    msg.pid  = upid;
    
    if (dsrvr_failInfo.what == CORRUPT_MESSAGE){
        msg.reason_code = LAUNCH_ERR_CORRUPT_MSG;
    }
    else{
        msg.reason_code = LAUNCH_ERR_PCT_GROUP;
    }

    msg.type.peer.nid = dsrvr_failInfo.last_nid;
    msg.type.peer.pid = dsrvr_failInfo.last_pid;
    msg.type.peer.ptl = 0;
    msg.type.peer.op  = dsrvr_failInfo.operation;

    rc = srvr_send_to_control_ptl(pl->srvr.nid, pl->srvr.pid, pl->srvr.ptl,
                LAUNCH_FAILURE_MSG, (char *)&msg, sizeof(launch_failure));
    
    if (rc){
        log_warning("Error sending PCT group comm failure to %d/%d/%d\n",
                   pl->srvr.nid, pl->srvr.pid, pl->srvr.ptl);
    }

    if (msg.type.peer.op == SEND_OP) {
        /* tell bebopd we are in trouble and not to use us anymore */
        update_status(STATUS_DOWN, NOUPDATE);
        send_to_bebopd(PCT_UPDATE_MSG);

        log_warning("PCT pruning itself\n");
        log_warning("DONE\n");

        exit(-1);
    }
 
    return 0;
}

int
send_done_to_yod(proc_list *pl, int upid, int return_code, int term_sig,
                  time_t end_time)
{
int rc;
app_proc_done msg;

    if (!pl){
         return 0;
    }

    LOCATION("send_done_to_yod","top");

    State.final.term_sig  = (unsigned char)term_sig;
    State.final.exit_code = (unsigned short)return_code;

    msg.final = State.final;

    msg.job_id = pl->job_id;
    msg.nid    = _my_pnid;
    msg.rank   = pl->my_rank;
    msg.pid    = upid;
    msg.status = State.user_status;
    msg.bt_size = pl->bt_size;

    cur_proc_end_time = end_time;

    msg.elapsed = end_time - cur_proc_start_time;

    rc = srvr_send_to_control_ptl(pl->srvr.nid, pl->srvr.pid, pl->srvr.ptl,
                PROC_DONE_MSG, (char *)&msg, sizeof(app_proc_done));

    if (rc){
        log_warning("error sending done msg to yod for app pid %d",
                        msg.pid);
    }

    return 0;
}
/*****************************************************************************/
/*         pct -> log debugging info                                         */
/*****************************************************************************/

static void display_nidlist(nid_type *nlist, int n);
static void display_idlist(ppid_type *plist, int n);
static void display_string(char *s);
static void display_file_info(hostReply_t *hr);

static char status_events[200];
static char linebuf[80];

char*
status_string(int stat)
{

    strcpy(status_events,"events:");

    if (stat == 0){
        strcat(status_events," none");
        return status_events;
    }

    if (stat & STARTED){
        strcat(status_events," started");

        if (stat & REQUESTED_PIDMAP){
            strcat(status_events," req-pid-map");
        }

        if (stat & SENT_PIDMAP){
            strcat(status_events," has-pid-map");
        }

        if (stat & REQUESTED_NIDMAP){
            strcat(status_events," req-nid-map");
        }

        if (stat & SENT_NIDMAP){
            strcat(status_events," has-nid-map");
        }

        if (stat & REQUESTED_FILEIO){
            strcat(status_events," req-stdio");
        }

        if (stat & SENT_FILEIO){
            strcat(status_events," has-stdio");
        }

        if (stat & GOT_READY){
            strcat(status_events," ready");
        }

        if (stat & SENT_TO_MAIN){
            strcat(status_events," go-main");
        }

        if (stat & CHILD_DONE){
            strcat(status_events," done");
        }

        if (stat & CHILD_NACK){
            strcat(status_events," NACK");
        }

        if (stat & SENT_KILL_1){
            strcat(status_events," send SIGTERM");
        }

        if (stat & SENT_KILL_2){
            strcat(status_events," send SIGKILL");
        }
    }
    else{
        strcat(status_events,"new job, not started");
    }

    return status_events;
}

void
display_load_request(proc_list *pl)
{

    if (!pl) return;

    log_msg("Load request:");
    log_msg("pname: %s",pl->pname);
    log_msg("%d processes, job id %d, session %d, parent job %d",
	pl->nprocs, pl->job_id, pl->session_id, pl->parent_id);
 
    if (pl->arglist){
        log_msg("args: "); display_string(pl->arglist);
    }
    if (pl->envlist){
        log_msg("env: "); display_string(pl->envlist);
    }
    log_msg("option bits 0x%08x",LoadData.msg2.option_bits);

    log_msg("nodes:");  display_nidlist(pl->nids, pl->nprocs);

    log_msg("pids:");  display_idlist(pl->IDmap, pl->nprocs);

    log_msg("uid %d, euid %d, gid %d, egid %d, umask 0x%08x",
          LoadData.msg2.uid, LoadData.msg2.euid,
          LoadData.msg2.gid, LoadData.msg2.egid, LoadData.msg2.u_mask);

    log_msg("stdin: "); display_file_info(&(LoadData.msg2.fstdio[0]));
    log_msg("stdout: "); display_file_info(&(LoadData.msg2.fstdio[1]));
    log_msg("stderr: "); display_file_info(&(LoadData.msg2.fstdio[2]));
}
static void
display_file_info(hostReply_t *hr)
{
    log_msg("fh 0x%08x, curPos %d, isatty %d, srvr nid/pid/ptl %d/%d/%d",
      hr->retVal, hr->info.openAck.curPos, hr->info.openAck.isattyFlag,
      hr->info.openAck.srvrNid, hr->info.openAck.srvrPid, 
      hr->info.openAck.srvrPtl);
}
static void
display_idlist( ppid_type *plist, int n)

{
int i;
char *p;
 
    p = linebuf;

    for (i=1; i<=n; i++){

        if ( (*plist == 0) || (*plist > 9999)){
             log_msg("bad list");
             return;
        }
        sprintf(p, "%04d ", *plist++);
        p += 5;

        if (p - linebuf > 75){
            log_msg("%s",linebuf);
            p = linebuf;
        }
    }

    if (p > linebuf) log_msg("%s",linebuf);
}
static void
display_nidlist(nid_type *nlist, int n)
{
int i;
char *p;
 
    p = linebuf;

    for (i=1; i<=n; i++){

        if ( (*nlist < 0) || (*nlist > 9999)){
             log_msg("bad list");
             return;
        }
        sprintf(p, "%04d ", *nlist++);
        p += 5;

        if (p - linebuf > 75){
            log_msg("%s",linebuf);
            p = linebuf;
        }
    }

    if (p > linebuf) log_msg("%s",linebuf);
}
static void
display_string(char *s)
{
int i;
char *p;
 
    p = linebuf;

    for (i=0; ; i++){

        if (i && (s[i] == 0) && (s[i-1] == 0)) break;

        if ((s[i] < 32) || (s[i] > 126)){

            sprintf(p, "<%03d>", s[i]);
            p += 5;
        } 
        else{
            sprintf(p, "%c", s[i]);
            p += 1;
        } 

        if (p - linebuf > 40){
            log_msg("%s", linebuf);
            p = linebuf;
        }
    }
    if (p > linebuf) log_msg("%s",linebuf);
}
