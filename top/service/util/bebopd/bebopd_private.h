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
** $Id: bebopd_private.h,v 1.22 2002/03/14 07:43:10 jrjohns Exp $
*/

#include "srvr_comm.h"

#ifndef BEBOPD_PRIVATEH
#define BEBOPD_PRIVATEH

extern double dclock(void);

/*
** for remapping code
*/
#ifdef REMAP
#define DEFAULT_REMAP "/tmp/remap"
#endif

/*
** from bebopd_pbs.c
*/

extern char PBSsupport;
extern char PBSupdate;
extern int PBSinteractive;
extern int interactive_list[];
extern int interactive_list_req[];
extern int interactive_list_req_size;
extern int interactive_size_req;
extern int activeNodes;
extern int unusedNodes;

#ifdef STRIPDOWN
typedef enum{ UNSET_NODE, IACTIVE_NODE, PBS_NODE} nodeTypes;
extern int PbsFree;
extern int PbsBusy;
extern int PBSscheduled;
extern int IactiveFree;
extern int IactiveBusy;
extern int scheduled_list[];
extern nodeTypes nodeType[];
extern int Registered[];
extern int pbsMIN;
extern int pbsMAX;
extern int intMIN;
extern int intMAX;
#endif

int PBSfreeNodes(void);
int PBSdepartingScavengers(void);
int PBSscavengers(void);
int InteractiveFreeNodes(void);
void new_interactive_list(int *list, int len);
void new_interactive_size(int size);
void update_interactive_list(void);
char *make_interactive_node_string(int *ilist);
void clearOutlierScavengers(int type);
int clearScavengersForJob(int jobSize, int type);



int recover_free_pbs_nodes(void);
int clearScavengersForJob(int jobSize, int type);

void update_PBS_server_with_change(void);
void update_PBS_server(void);
void compute_active_nodes(void);

int qmgr_q_query(char **, time_t, char *);
int query_qstat(char **, time_t);
int qstat_list_queues(char **, time_t);
int query_pbs(char **, time_t, char *);

/*
** from bebopd_alloc.c
*/
int find_free_nodes(yod_request *yod_data, 
       control_msg_handle *mhandle, int **pctList, int jobID);
int solicit_pct_updates(nid_type *nids, int nquery,
            int request_for, int ageOfUpdate, int id, int sessionid);
void log_node_state_info();

extern int timing_queries, timing_updates;
extern int reservedNodes;

typedef enum{
    inactiveNode,           /* down, stale, whatever */
    interactiveFree,        /* free interactive node, not pending allocation */
    interactivePending,     /* free interactive node, pending allocation     */
    interactiveRegular,     /* int. node running reg priority int. job */
    interactiveLowRunning,  /* int. node running low priority int. job */
    interactiveDyingPending,  /* dying, already pending to next job */
    interactiveDying,         /* dying, not pending to a new job */
    pbsOutlier,                  /* pbs job running on interactive node */
    pbsFree,                /* free pbs node, not pending allocation */
    pbsPending,             /* free pbs node, pending allocation to new job */
    pbsRegular,             /* pbs node running reg. priority pbs job */
    pbsLowRunning,          /* pbs node running low priority job */
    pbsDyingPending,     /* being killed, already allocated to next job */
    pbsDying,            /* being killed, available to a new job */
    interactiveOutlier,     /* interactive job running on pbs node */
    numNodeStates
} nodeStateTypes;

/*
** from bebopd.c
*/

extern char *ename; 
extern char *pbsUpdateProc;
extern int niceKill;

int retry_send(INT32 nid, INT32 pid, int idx, INT32 tag,
	     CHAR *udata, INT32 len);
int check_yod_request(yod_request *yod_data);
int update_pct_pid_array(int countType);
void finish_bebopd(void);

extern int debug_toggle;
extern const char *routine_name;
extern const char *routine_where;
extern int update_ptl;
extern int Dbglevel;
extern int minPct, maxPct;
extern pct_ID larray[];
extern char loadCandidate[];
extern nodeStateTypes nodeState[];
extern int nodeStateTotals[];
extern int totalLoadCandidates;
extern int totalFreeCandidates;
extern int interactiveClearingCandidates;
extern int interactiveClearingNodes;

extern int reserved[];
extern int pct_spids[];
extern int timing_data;
extern int debug_toggle;

extern int daemonWaitLimit;
extern int clientWaitLimit;
extern int solicitPctsTmout;
  
#define LOCATION(a,b) {routine_name=a; routine_where=b;}

#define COUNT_NEW   1
#define COUNT_ALL   2

#define INTERACTIVE  1
#define SCHEDULED   2

/*
** from bebopd_log.c
*/
char * myLogFileName();
int write_log_record(char *s);
#endif
