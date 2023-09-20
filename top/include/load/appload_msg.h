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
** $Id: appload_msg.h,v 1.56 2002/02/23 23:44:28 pumatst Exp $
*/


/*
** definitions required by both yod and pct - their messages
*/
#ifndef APPLOAD_MSGH
#define APPLOAD_MSGH

#include <signal.h>
#include "puma_errno.h"
#include "rpc_msgs.h"
#include "sys_limits.h"
#include "proc_id.h"
#include "bebopd.h"

#define BELL  ((char)7)

/**********************************************************************
** yod to pct
*/
#define FEW_GROUPS       5

#define OPT_INCL_MPI     0x0001
#define OPT_INCL_NX      0x0002
#define OPT_UNUSED       0x0004
#define OPT_LOG          0x0008
#define OPT_SPECIAL      0x0010
#define OPT_BT           0x0020
#define OPT_ATTACH       0x0040
#define OPT_SLEEP_1      0x0080
#define OPT_SLEEP_2      0x0100
#define OPT_SLEEP_3      0x0200
#define OPT_SLEEP_4      0x0400

#define MSG_INIT_LOAD      (1)
#define MSG_PUT_DATA       (2)
#define MSG_PUT_PNAME      (3)
#define MSG_PUT_ARGS       (4)
#define MSG_PUT_ENV        (5)
#define MSG_PUT_NIDS       (6)
#define MSG_PUT_EXEC       (7)
#define MSG_PUT_PIDS       (8)
#define MSG_GO_MAIN        (9)
#define MSG_ALL_DONE      (10)
#define MSG_ABORT_LOAD_1  (11)
#define MSG_ABORT_LOAD_2  (12)
#define MSG_SEND_SIGUSR   (13)
#define MSG_ABORT_RESET   (14)
#define MSG_GET_PIDMAP    (15)
#define MSG_PUT_EXEC_PATH (16)
#define MSG_GET_BT        (17)
#define MSG_PUT_PORTAL_IDS (18)
#define MSG_PUT_GROUPS     (19)
#define MSG_PUT_STRACE     (20)
#define MSG_YODLESS_COMPLETION        (21)
#define MSG_REQUEST_TO_LOAD           (22)
#define MSG_CANCEL_REQUEST_TO_LOAD    (23)

#define YOD_TO_PCT_FIRST_MSG (MSG_INIT_LOAD)
#define YOD_TO_PCT_LAST_MSG  (MSG_CANCEL_REQUEST_TO_LOAD)

extern const char *yod_to_pct_strings[];
#define select_yod_to_pct_string(n)  \
   TYPE_TO_STRING(YOD_TO_PCT_FIRST_MSG, YOD_TO_PCT_LAST_MSG, n, yod_to_pct_strings)

#define REGULAR_JOB   1
#define SCAVENGER     2

typedef struct {
    int job_id;      /* each yod instance has a job id */
    int session_id;  /* an id for a collection of yod instances, like a PBS job */
    int parent_handle;    /* these two handles are for process spawning */
    int parent_job_id;    /* Cplant job id of parent process */
    int my_handle;
    int nprocs;
    int n_members;       /* heterogeneous load - num executables */
    server_id yod_id;    /* for pct -> yod communication */
} load_msg1;

typedef struct{
    int  fromRank;
    int  toRank;
    int  argbuflen;
    int  execlen;
} loadMemberData;

typedef struct{
    hostReply_t   fstdio[3];
    int           u_mask;
    uid_t         uid;
    uid_t         gid;
    uid_t         euid;
    uid_t         egid;
    int           envbuflen;

    int           option_bits;            /* mpi, nx? */
    int           fyod_nid;

    int           app_serv_ptl;  /* for app -> yod communication */

    loadMemberData  data;

    int    ngroups;
    gid_t  groups[FEW_GROUPS];  /* common case: all groups will fit here */
    int    straceMsgLen;
    int    priority;       /* 1 - regular job, 2 - scavenger */
} load_msg2;


typedef struct{
   int job_ID;   /* this must be first field */
   char type;
} send_sig;

typedef struct{
   int job_ID;   /* this must be first field */
   unsigned char cksum;
} sendExec;

typedef struct{
    load_msg2 msg2;
    int   thePcts[MAX_NODES];
} load_data_buffer;

typedef struct{
    int job_ID;
    int dirlen;
    int optlen;
    int listlen;
} straceInfo;

/**********************************************************************
** pct to yod
*/
/*
** child status fields - fixup strings in load_msgs.c if these
** bit meanings are changed
*/
#define  NO_STATUS           0
#define  NEW_JOB            (1 << 0)
#define  STARTED            (1 << 1)
#define  REQUESTED_PIDMAP   (1 << 2)
#define  SENT_PIDMAP        (1 << 3)
#define  REQUESTED_NIDMAP   (1 << 4)
#define  SENT_NIDMAP        (1 << 5)
#define  REQUESTED_FILEIO   (1 << 6)
#define  SENT_FILEIO        (1 << 7)
#define  GOT_READY          (1 << 8)
#define  SENT_TO_MAIN       (1 << 9)
#define  CHILD_DONE         (1 << 10)
#define  CHILD_NACK         (1 << 11)
#define  SENT_KILL_1        (1 << 12)
#define  SENT_KILL_2        (1 << 13)
#define  GOT_PORTAL_ID      (1 << 14)
#define  SENT_FYODMAP       (1 << 15)  
#define  NICE_KILL_JOB_STARTED  (1 << 16)  

/*
** msgs from pct back to yod
*/
#define  INVALID_MSG         (-1)
#define  LOCAL_PID_MSG        (0)
#define  APP_READY_MSG        (1)
#define  PROC_DONE_MSG        (2)
#define  ALL_DONE_ACK_MSG     (3)
#define  LAUNCH_FAILURE_MSG   (4)
#define  SEND_EXEC_MSG        (5)
#define  COPY_EXEC_MSG        (6)
#define  APP_PORTAL_ID        (7)
#define  OK_TO_LOAD_MSG       (8)
#define  TRY_AGAIN_MSG        (9)
#define  REJECT_LOAD_MSG      (10)

typedef struct _appID{
    spid_type pid;         /* process ID */
    ppid_type ppid;        /* portal ID  */
} appID;

#define PCT_TO_YOD_FIRST_MSG (LOCAL_PID_MSG)
#define PCT_TO_YOD_LAST_MSG  (REJECT_LOAD_MSG)

extern const char *pct_to_yod_strings[];
#define select_pct_to_yod_string(n)  \
   TYPE_TO_STRING(PCT_TO_YOD_FIRST_MSG, PCT_TO_YOD_LAST_MSG, n, pct_to_yod_strings)


#ifdef LINUX_PORTALS

extern const char *signal_names[];
#define select_signal_name(n)  \
   TYPE_TO_STRING(0, _NSIG-1, n, signal_names)

#else

#define select_signal_name(n) ""

#endif


#define LAUNCH_ERR_OK            (0)
#define LAUNCH_ERR_MISC          (1)
#define LAUNCH_ERR_FATAL         (2)
#define LAUNCH_ERR_LOAD_DATA     (3)
#define LAUNCH_ERR_TEMP_NAME     (4)
#define LAUNCH_ERR_JOBID         (5)
#define LAUNCH_ERR_FORK          (6)
#define LAUNCH_ERR_EXEC          (7)
#define LAUNCH_ERR_WAIT          (8)
#define LAUNCH_ERR_CRT_GDB_PIPE  (9)
#define LAUNCH_ERR_FORK_GDB      (10)
#define LAUNCH_ERR_LOC_GDB_BIN   (11)
#define LAUNCH_ERR_EXEC_GDB_BIN  (12)
#define LAUNCH_ERR_FORK_GDBWRAP  (13)
#define LAUNCH_ERR_LOC_GDBWRAP_BIN (14)
#define LAUNCH_ERR_EXEC_GDBWRAP_BIN (15)
#define LAUNCH_ERR_CHILD_COMM    (16)
#define LAUNCH_ERR_CHILD_NACK    (17)
#define LAUNCH_ERR_CHILD_EXIT    (18)
#define LAUNCH_ERR_PCT_GROUP     (19)
#define LAUNCH_ERR_YOD_REPLY     (20)
#define LAUNCH_ERR_EXEC_PATH     (21)
#define LAUNCH_ERR_YOD_PROTOCOL  (22)    /* yod / main pct */
#define LAUNCH_ERR_HOST_PROTOCOL (23)    /* main pct / app host */
#define LAUNCH_ERR_APP_PROTOCOL  (24)    /* app host / app process */
#define LAUNCH_ERR_PRE_EXEC      (25)
#define LAUNCH_ERR_CORRUPT_MSG   (26)
#define LAUNCH_ERR_PORTAL_ERR    (27)

#define FIRST_LAUNCH_ERR   (LAUNCH_ERR_OK)
#define LAST_LAUNCH_ERR    (LAUNCH_ERR_PORTAL_ERR)

extern const char *launch_err_strings[];
#define select_launch_err_string(n)  \
   TYPE_TO_STRING(FIRST_LAUNCH_ERR, LAST_LAUNCH_ERR, n, launch_err_strings)

extern const char *child_status_strings[];

typedef struct{
    nid_type nid;
   spid_type pids[MAX_PROC_PER_GROUP_PER_NODE];
}local_pid_msg;

typedef struct{
    int nid;
    int n_local_procs;
}ready_msg;

typedef struct _app_proc_done{
    int      job_id;
    int      nid;
    int      pid;
    int      rank;
    final_status final;
    int      elapsed;
    int      status;
    int      bt_size;
} app_proc_done;

typedef struct _all_done_ack{
    int      job_id;
} all_done_ack;

typedef struct _failedPeer{
    int nid;
    int pid;
    int ptl;
    int op;
} failedPeer;

typedef struct _childErr{
    int     Errno;
    int     CPerrno;
    int     reason_code;
}childErr;

typedef struct {
    int     job_id;
    int     rank;
    int     nid;
    int     pid;
    int     reason_code;
    union{
	failedPeer peer;
	childErr child;
    }type;
}launch_failure;

typedef struct {
    unsigned int pct_pnid;
    unsigned int host_pnid;
    unsigned int port;
    unsigned int hipassword;
    unsigned int lopassword;
} tvdsvr_exec_info_t;

/**********************************************************************
** physical node number to node name
*/

int print_node_name(int phys_nid, FILE *fp);

#endif
