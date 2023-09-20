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
** $Id: pct.h,v 1.61 2002/02/23 23:35:04 pumatst Exp $
*/

#ifndef PCT_H
#define PCT_H

#include <time.h>
#include "sys_limits.h"
#include "appload_msg.h"
#include "rpc_msgs.h"
#include "pct_start.h"
#include "srvr_comm.h"

extern pct_ID State;
extern double dclock(void);
extern double td0, td1, td2, td3;
extern int Dbglevel;
extern int DebugSpecial;
extern load_data_buffer LoadData;
extern load_msg1 init_msg;
extern int collectiveWaitLimit;

extern char *ename, *gdbProc, *gwrapProc, *appProc;

extern time_t start_pending_clock;

/* time to wait from getting PCT_STATUS_ALLOC 
   from bebopd till getting MSG_INIT_LOAD from yod
 */
#define PENDING_STATE_TIME_OUT 30
/*
** seconds to wait for a node allocator
** to show up when we get started
*/
#define BEBOPD_GIVEUP  (60.0 * 10.0)
/*
** seconds between initial IM ALIVE
** messages to the bebopd
*/
#define BEBOPD_INTERVAL (10.0)
#define PCT_HOSTS_ONE_PROC

typedef struct _proc_list {
   /*
   ** global job info
   */
   int      job_id;                            /* application group id */
   int      session_id;         /* a group of jobs, like a PBS batch job */
   int      parent_id;   /* Cplant job ID of parent that spawned this application */
   int      nprocs;                       /* number processes in group */
   int      nmembers;        /* number of members (heterogeneous load) */
   int      job_status;  /* not keeping global job status at this time */
   server_id  srvr;
   
   nid_type nids[MAX_PROC_PER_GROUP];    /* node id map - indexed by rank */

   ppid_type IDmap[MAX_PROC_PER_GROUP];  /* portal id map, by rank */

   char *envlist;

   /*
   ** local job info
   */
   ppid_type      ppid;     /* portals pid */
   spid_type      pid;      /* system pid */

   char *user_exec;
   char *pname;
   char *arglist;

   int      status;
   int      subgroupRanks[MAX_PROC_PER_GROUP]; /* heterogeneous load sub group */
   int      subgroupSize;

   int      my_rank;

   int      bt_size;  /* also borrowed by determine_executable_location */

   load_msg2 yod_msg;

   gid_t *groupList;

   straceInfo *strace;

   time_t  nice_kill_1;
   time_t  nice_kill_2;

} proc_list;

#define NICE_KILL_INTERVAL  60*5

#define STARTUP_MASK (NEW_JOB | REQUESTED_PIDMAP | SENT_PIDMAP | REQUESTED_NIDMAP  | SENT_NIDMAP | REQUESTED_FILEIO | SENT_FILEIO | GOT_READY | GOT_PORTAL_ID | SENT_FYODMAP)

#define EXEC_FAILURE        (127)
#define PRE_EXEC_FAILURE    (126)

#define MAX_FANOUT_DEGREE                8
#define TREE_PARENT(rank,degree)        ( (int)((rank) - 1) / degree)
#define TREE_CHILD(rank,degree,which)   ( ((rank) * degree) + 1 + which)

/*
** job status fields - not really implemented yet
*/
#define  GLOBAL_PIDS_PROPAGATED  (1 << 0)

#define UNUSED (-1)

#define NOUPDATE (-1)

/*
** Defined in pct_comm.c
*/
int check_bebopd_id(nid_type nid, spid_type pid);
void set_bebopd_id(nid_type nid, spid_type pid, int ptl);
void find_bebopd_id(int *nid, int *pid, int *ptl);

INT32 initialize_listen(nid_type nid, spid_type pid, int ptl);
INT32 initialize_app_portals(int *ctl_ptl);
INT32 cleanup_pct(void);
INT32 check_host_health();

INT32 start_request_check(control_msg_handle *req);
INT32 free_app_message (control_msg_handle *req);
void clear_app_messages(void);

INT32 tvdsvr_request_check(control_msg_handle *req);
INT32 free_tvdsvr_message(control_msg_handle *req);
int process_tvdsvr_req(control_msg_handle* bt_req);

INT32 bt_request_check(control_msg_handle *req);
INT32 free_bt_message(control_msg_handle *req);
int process_bt_req(control_msg_handle* bt_req);

INT32 cgdb_request_check(control_msg_handle *req);
INT32 free_cgdb_message(control_msg_handle *req);
int process_cgdb_req(control_msg_handle* bt_req);

INT32 yod_request_check(control_msg_handle *req);
INT32 free_yod_message(control_msg_handle *req);

INT32 bebopd_request_check(control_msg_handle *req);
INT32 free_bebopd_message(control_msg_handle *mh);

VOID init_child_job_id(int child_id, int session, int parent);
VOID init_child_pid(int child_pid, time_t start_time);
VOID init_child_owner(int child_owner);
VOID init_child_rank(int rank);
VOID init_child_priority(int priority);


VOID clear_reservation();
VOID update_reservation(uid_t euid);
int reservation_violation(uid_t euid);
VOID update_status(int pct_status, int child_status);
VOID update_terminator(uid_t euid);
INT32 send_to_bebopd(int type);

int pct_pending(void);
int pending_for(void);
void start_pending_for(int id, int session);
void restart_pending_timeout(void);
void stop_pending_for(void);


int user_status(void);

int send_failure_to_yod(proc_list *pl, int pid, int reason);
int send_child_failure_to_yod(proc_list *pl, int upid,
		    int reason, int return_code, int term_sig);
int send_child_nack_to_yod(proc_list *pl, int pid, child_nack_msg *nmsg);
int send_group_failure_to_yod(proc_list *pl, int upid);
int send_done_to_yod(proc_list *pl, int pid,
                    int return_code, int term_sig, time_t end_time);

char* status_string(int stat);
void display_load_request(proc_list *pl);

/*
** defined in pct_group.c
*/
int initialize_pct_group(int *pctList, int npcts, int groupID);
int takedown_pct_group(void);
int fanout_control_message(int msg_type, char *user_data, int len,
            unsigned int fanout_degree, int groupRoot, int groupSize);

/*
** main pct (node manager) to sub pct (app host) (pct_app_host.c)
*/

#define TODO_NULL     (1 << 0)   /* requests */
#define TODO_START    (1 << 1) 
#define TODO_GETPIDS  (1 << 2)
#define TODO_GOMAIN   (1 << 3)
#define TODO_SIGUSR1  (1 << 4)
#define TODO_SIGUSR2  (1 << 5)
#define TODO_ABORT1   (1 << 6)
#define TODO_ABORT2   (1 << 7)
#define TODO_RESET    (1 << 8)
#define TODO_ALLDONE  (1 << 9)
#define TODO_GETPEERS (1 << 10)
 
typedef struct _mgrData{
    int todo;               /* action requested of app host */
    int dbglevel;           /* log status data to log file */
    int debugSpecial;       /* log timing data to log file */
}mgrData;
/*
** sub pct to main pct
*/
#define APP_HOST_OK     (0)
 
typedef struct _appData{
    char attn;              /* set if app process state changes */
    char started;           /* app process has started */
    char done;              /* app process has terminated */
    char done_reported;     /* UNUSED termination has been reported to main pct */
 
    int pid;                /* pid of app process */

    int ppid;               /* portal pid of app process */

    int u_stat;             /* bit mask of new events of child process */
 
    child_nack_msg nack;
    int child_rc;           /* child terminates with exit code  */
    int child_term_sig;     /* child terminates from signal     */
 
    int bt_size;            /* size in bytes of debugger output */
 
    time_t start_time;      /* time of child fork */
    time_t end_time;        /* approximate time of termination */
 
    int rc;                 /* return code of app host */
}appData;

char *get_back_trace(void);
appData *go_app_host(mgrData *mdata, char *buf, int len);
int reset_app_host(void);
void display_app_state(void);

/*
** defined in pct_app.c
*/
void init_app_structures(void);
void display_active_list(void);
void display_proc(proc_list *pl);
proc_list *get_proc_list(int job_id);
proc_list *get_free_proc_entry(void);
proc_list *current_proc_entry(void);
int nice_kill_time_left();
int nice_kill_timeout();
void recover_proc_list_entry(proc_list *pl);

int start_new_job(proc_list *pl);
int init_new_job(void);
int allocate_load_data(proc_list *pl);
int determine_executable_location(proc_list *pl);

#define EXEC_VOTE_TYPE   0x11000011

extern const char *routine_name;
extern const char *routine_where;
extern const char *routine_name0;
extern const char *routine_where0;
extern const char *routine_name1;
extern const char *routine_where1;

#define LOCATION(a,b) {\
routine_name0=routine_name1;routine_where0=routine_where1; \
routine_name1=routine_name;routine_where1=routine_where; \
routine_name=a;routine_where=b;}

/*
#define DEBUG_STALE_NODES(a)  log_msg("%s",a)
*/
#define DEBUG_STALE_NODES(a)   

#endif /* PCT_H */
