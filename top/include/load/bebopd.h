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
** $Id: bebopd.h,v 1.51.2.1 2002/03/21 18:32:04 jsotto Exp $
*/
 
#ifndef BEBOPD_H
#define BEBOPD_H

#include <time.h>
#include <pwd.h>
#include <sys/types.h>
#include <stdio.h>
#include "proc_id.h"
#include "puma_errno.h"

#define SUMMARY_ILIST_SIZE 64

/*********************************************/
/*   bebopd - pingd communications           */
/*********************************************/

typedef struct {
    int nodes;             /* total nodes in compute partition */
    int pbsSupportLevel;   /* 0 - no PBS, 1 - PBSsupport, 2 - PBSupdate */

#ifndef STRIPDOWN
    int pbsInteractive;  /* if PBS, number of nodes for interactive jobs */
#endif
    int pbsOutliers;     /* pbs processes on interactive nodes */
    int intOutliers;     /* interactive processes on pbs nodes */
    int reserved;        /* reserved by system administration */
    char ilist[SUMMARY_ILIST_SIZE]; /* current interactive list */
} pingd_summary;

/*********************************************/
/*   bebopd - remote pct communications      */
/*********************************************/
/* used in conjunction w/ */
extern const char *pct_status_strings[];
#define STATUS_NO_STATUS   0 /* we've never heard from the node */
#define STATUS_FREE        1 /* not running a job */
#define STATUS_ALLOCATED   2 /* contacted by allocator -- for STRIPDOWN */
#define STATUS_BUSY        3 /* busy running a job */
#define STATUS_DOWN        4 /* PCT has reported it is exiting */
#define STATUS_UNREPORTED  5 /* PCT had reported in, but now doesn't respond */
#define STATUS_TROUBLE     6 /* PCT found problems on the node during health check */
#define LAST_PCT_STATUS   STATUS_TROUBLE

#define REQ_FOR_YOD        1
#define REQ_FOR_PING       2
#define REQ_FOR_BEBOPD     3
#define REQ_FOR_SCAN       4

#define STATUS_UP(s)  ((s != STATUS_NO_STATUS) && (s != STATUS_DOWN))

#define STATUS_TALKING(s) \
   ((s != STATUS_NO_STATUS) && (s != STATUS_DOWN) && (s != STATUS_UNREPORTED))
 

#define PCT_UPDATE_MSG    0x0000ffff
#define PCT_INITIAL_MSG   0x0000eeee

typedef struct{
    unsigned char term_sig;
    unsigned char terminator;   /* who killed job, owner or admin? */
    unsigned short exit_code;
} final_status;

typedef struct {
    INT16     status;       /* pct's status              */
    INT16     pending_jid;  /* allocated to this job next (or INVAL)  */
    INT16     pending_sid;  /* this session (a.k.a. PBS job) */
    int       reserved;     /* UID of node owner, (or INVAL) */
#ifdef STRIPDOWN
    int       nodeType;     /* interactive or scheduled */
#endif

    /*
    ** The following fields get cleared when a job exits.  The
    ** fields above persist.
    */
    INT16        rank;        /* current job's rank        */
    INT16        job_id;      /* current user job id       */
    INT32        user_status; /* user process status       */
    INT32        session_id;  /* current user session id   */
    INT32        parent_id;   /* Cplant job ID of spawning parent */
    INT32        job_pid;     /* current user job system pid  */
    UINT32       euid;        /* job owner                 */
    final_status final;

    time_t       elapsed;    /* seconds since fork        */
    int   niceKillCountdown; /* non-zero if job is being killed */

    int          priority;   /* 1 - regular, 2 - scavenger */

} pct_ID;

typedef struct {
    int nid;       /* pct's nid        */
    int pid;       /* pct's system pid */
    pct_ID status;
} pct_rec;

#define INVALID_PCT_REQUEST   (-1)
#define PCT_DIE_REQUEST       (1)   
#define PCT_RESERVE_REQUEST   (2)  
#define PCT_UNRESERVE_REQUEST (3)
#define PCT_STATUS_REQUEST    (4)
#define PCT_FAST_STATUS       (5)
#define PCT_NOT_ALLOCATED     (6)
#define PCT_RESET_REQUEST     (7)
#define PCT_INTERRUPT_REQUEST (8)
#define PCT_GONE_UPDATE       (9)
#define PCT_GOTCHA            (10)
#define BEBOPD_PBS_SUPPORT_ON   (11)
#define BEBOPD_PBS_SUPPORT_OFF  (12)
#define BEBOPD_PBS_UPDATE_ON    (13)
#define BEBOPD_PBS_UPDATE_OFF   (14)
#define BEBOPD_PBS_INTERACTIVE  (15)
#define BEBOPD_PBS_LIST         (16)
#define PCT_SCAN                (17)
#define PCT_NICE_KILL_JOB_REQUEST   (18)
#ifdef STRIPDOWN
#define PCT_ALLOC_REQUEST   (19)
#endif

typedef struct{
    char         opsrc;
    int          bptl;
    int          bnid;
    int          bpid;
    int          jobID;
    int          sessionID;
} pctStatus_req;

typedef struct {
    INT32  nid1;
    INT32  nid2;
    INT32  jobID;
    INT32  sessionID;
    INT32  euid;
    INT32  reserve_uid;
} pingPct_req;

typedef struct {           /* pingd request sent to bebopd */
    pingPct_req args;      /* this part is forwarded to the pct */
    int pingPtl;  /* bebopd can reach pingd here */
} ping_req;

void display_pct_ID_title(int);
void display_pct_ID(pct_rec *pct_stat);
void display_pct_ID_title_parse(void);
void display_pct_ID_parse(pct_rec *pct_stat);

#define PCT_WAIT_SECONDS  (30)

#define display_pct_RES(x) {                       \
}

/********************************************/
/*   bebopd - local yod communications   */
/********************************************/

#define YOD_NODE_REQ_ANY        (1)
#define YOD_NODE_REQ_RANGE      (2)
#define YOD_NODE_REQ_LIST       (3)
#define YOD_NODE_REQ_COMPOUND   (4)

#define YOD_USERLOG_RECORD      (5)

#define USERLOG_MAX  (1024)

typedef struct _nodeRange{
    INT32 from_node;  /* starting with this physical node */
    INT32 to_node;    /* ending with this physical node */
} yodNodeRange;

typedef struct _nodeList{
    INT32 listsize;
} yodNodeList;

typedef struct _compReq{   /* for heterogeneous loads */
    INT32 numRequests;
} yodCompoundReq;

typedef union _nodeSpec{
    yodNodeRange range;
    yodNodeList  list;
    yodCompoundReq req;
}nodeSpec;

typedef struct _yod_request{
    INT32 nnodes;      /* yod requests pct IDs for this many nodes */
    INT32 session_id;  /* id for collection of yod instances, like a PBS job */
    INT32 nnodes_limit;  /* limit on node usage for session */
    int priority;        /* for PBS - regular or scavenger */
    int   myptl;
    uid_t euid;
    int specType;
    nodeSpec spec;
}yod_request;

typedef struct _bebopd_status{
    INT32 job_id;
    INT32 rc;
}bebopd_status;

#define BEBOPD_GET_NODE_LIST      0x0000a0f1
#define BEBOPD_PUT_PCT_LIST       0x0000a0f2
#define BEBOPD_GET_COMPOUND_REQ   0x0000a0f3
#define BEBOPD_GET_USERLOG_RECORD 0x0000a0f4

#define BEBOPD_OK                      (0)
#define BEBOPD_ERR_FREE_NODES          (1)
#define BEBOPD_ERR_SESSION_LIMIT       (2)
#define BEBOPD_ERR_INVALID             (3)
#define BEBOPD_ERR_NO_PBS_SUPPORT      (4)
#define BEBOPD_ERR_NO_INT_SUPPORT      (5)
#define BEBOPD_ERR_INSUFF_INT_NODES    (6)
#define BEBOPD_ERR_INTERNAL            (7)
#define BEBOPD_ERR_TRY_AGAIN           (8)
#ifdef STRIPDOWN
#define BEBOPD_ERR_CONTACT_NODES       (9)
#define BEBOPD_ERR_UNEXPECTED_NODE_STATE (10)
#endif

/****************************/
/* Dynanmic allocation info */
/****************************/

/* Define message types for dynamic node allocation portal */
#define BEBOPD_INFO_GET_JOB_SIZE        (1)
#define BEBOPD_JOB_GET_PBS_ID           (2)
#define BEBOPD_QUERY_QMGR_Q             (3)
#define BEBOPD_DO_QSTAT                 (4)
#define BEBOPD_LIST_QUEUES              (5)
#define BEBOPD_QUERY_SERVER             (6)
#define BEBOPD_SET_USER_DATA            (7)

#define REPLY_DOING_PUT                 (0)
#define REPLY_NO_DATA                   (1)



#endif
