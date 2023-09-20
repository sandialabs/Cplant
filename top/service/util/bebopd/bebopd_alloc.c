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
** $Id: bebopd_alloc.c,v 1.57.2.4 2002/09/27 01:00:41 jjohnst Exp $
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>

#include "sys_limits.h"
#include "bebopd.h"
#include "bebopd_private.h"
#include "config.h"
#include "appload_msg.h"
#include "srvr_comm.h"
#include "srvr_err.h"
#include "pct_ports.h"
#include "ppid.h"
#include "portal_assignments.h"

/* the following definition should be to a value 
   somewhat shorter than the one used in pingd.c
   (since pingd is waiting on bebopd, which is 
   waiting on a pct) */
#define WaitPCTstatus 15

static int pct_info[MAX_NODES];  /* list of allocated nodes to return to yod */

static bebopd_status bebopd_ok={-1, BEBOPD_OK};

int timing_queries, timing_updates;
int reservedNodes;

#ifdef REMAP
extern nid_type carray[];
extern int minMap, maxMap, Remap;
#endif

extern int smart_alloc(nid_type *free, int len, int want, nid_type *go);
static nid_type free_list[MAX_NODES];
static nid_type go_list[MAX_NODES];

#ifndef STRIPDOWN
static void compute_node_states(void);
#else

int IactiveFree=0;  /* # type IACTIVE_NODEs that are free */
int IactiveBusy=0;  /* # type IACTIVE_NODEs that are busy */
int PbsFree=0;      /* likewise for scheduled nodes */
int PbsBusy=0;
void compute_iactive_totals(void);
void compute_pbs_totals(void);
#endif

#ifndef STRIPDOWN
/*
** deallocate_pct and deallocate_free_pcts return the number
** of messages sent out to the compute partition.
*/
static int
deallocate_pct(int nid)
{
char reqinfo;
int rc, sent;

    LOCATION("deallocate_pct","top");
    reqinfo = REQ_FOR_BEBOPD;

    if ( (nid < minPct) || (nid > maxPct)) return 0;

    sent = 1;

    rc = srvr_send_to_control_ptl(nid,  PPID_PCT,
	 PCT_STATUS_ACTION_PORTAL, PCT_NOT_ALLOCATED, &reqinfo, 1);

    if (rc){
	log_warning("Can't send NOT_ALLOCATED msg to pct on %d, will retry\n",
		      nid);

        if (retry_send(nid,  PPID_PCT, PCT_STATUS_ACTION_PORTAL, 
		      PCT_NOT_ALLOCATED, &reqinfo, 1)){

	    log_warning("Retry failed as well\n");
	    sent = 0;
        }
    }

    return sent;

}
static int
deallocate_free_pcts(void)
{
int i, updates;

    LOCATION("deallocate_free_pcts","top");

    updates = 0;

    for (i=minPct; i <= maxPct; i++){

        if (larray[i].status == STATUS_NO_STATUS) continue;

        if (loadCandidate[i] == 1){

	    updates += deallocate_pct(i);
	}
    }
    return updates;
}
#endif /* !STRIPDOWN */


/*
** solicit_pct_updates will update the array of pct statuses.
**
** nids can be a list of nodes to query, or the first two entries 
** can specify a range of nodes, or a null nid list specifies all
** nodes.
**
** nquery values:  0   query all nodes
**                >0   query all nodes in the nid list of length nquery
**                -1   nid list contains two node numbers specifying a range
**                       of nodes to query.
**
** An ageOfUpdate request specifies how old in seconds the data can be,
** so we are not swamping the compute nodes with status requests.
**
** Return value is the number of requests sent to compute partition, or
** -1 on error.
*/
static double last_total_pct_update;
 
int
solicit_pct_updates(nid_type *nids,   /* list or range of nodes to query */
            int nquery,
            int request_for,  /* for yod, pingd or bebopd? pct wants to know */
            int ageOfUpdate,  /* how many seconds old can the updates be?    */
	    int jid,              /* job/PBS id for yod requests */
	    int sessionID)
 
{
    pctStatus_req reqinfo;
    int i, j, nreq, rc, idx, nfound, nid1, nid2, queryall, num;
    double td;

#ifdef RELIABLE
    int   num_targets;
    int * target_nids;
    int   target_ppid= PPID_PCT;
    int   target_ptl= PCT_STATUS_ACTION_PORTAL;
    int   wait_for[MAX_NODES];
    int   done;
    int   num_to_wait_for;
    int   tmout_occured = 0;
#endif
 
    LOCATION("solicit_pct_updates", "top");

    if ((nquery != 0) && !nids){
	log_msg("solicit_pct_updates: nquery %d but no nid list",nquery);
	return -1;
    }

    if (maxPct < minPct) return 0;
 
    reqinfo.opsrc = ((request_for == REQ_FOR_SCAN) ? 
		       REQ_FOR_PING : request_for);
    reqinfo.bnid  = _my_pnid;
    reqinfo.bpid  = _my_ppid;
    reqinfo.bptl  = update_ptl;

    reqinfo.jobID     = jid;
    reqinfo.sessionID = sessionID;
 
    if ( last_total_pct_update &&
         ageOfUpdate &&
         ((dclock() - last_total_pct_update) < (double)ageOfUpdate)){
 
        if (Dbglevel){
            log_msg("skipping query of PCTs since my data is recent enough");
        }
	return 0;
    }
    queryall = FALSE;

    if (nquery < 1){
	if (nquery == 0){
	    nid1 = minPct;
	    nid2 = maxPct;
	    queryall = TRUE;
	}
	else{
	    if (nids[0] < nids[1]){
		nid1 = nids[0];
		nid2 = nids[1];
	    }
	    else{
		nid2 = nids[0];
		nid1 = nids[1];
	    }
	    nid1 = ((nid1 < minPct) ? minPct : nid1);
	    nid2 = ((nid2 > maxPct) ? maxPct : nid2);

	    if ((nid1==minPct) && (nid2==maxPct)) queryall = TRUE;
	}
	nids = NULL;
	nquery = nid2 - nid1 + 1;

    }

#ifdef RELIABLE
    num_targets = 0;
    target_nids  = malloc(nquery * sizeof(int));

    if (target_nids == NULL) {
        log_warning("out of memory in solicit pct updates\n");
        finish_bebopd();
    }
#endif
 
    for (i=0, nreq=0; i<nquery; i++){
 
	if (nids){
	    idx = nids[i];
	    if ((idx < minPct) || (idx > maxPct)) continue;
	}
	else{
	    idx = nid1 + i;
	}
 
	if ((request_for == REQ_FOR_SCAN) &&
	    (larray[idx].status == STATUS_NO_STATUS)){

            /*
	    ** only skip nodes we have never had an update from
	    */
	    continue;
        }	    
        else if ((request_for != REQ_FOR_SCAN) &&
	         !STATUS_TALKING(larray[idx].status)){

            /*
	    ** skip all nodes we believe are down
	    */
            continue;
        }

#ifdef RELIABLE
        target_nids[num_targets]  = idx;
        ++num_targets;
#else
        rc = srvr_send_to_control_ptl(idx, PPID_PCT,
                     PCT_STATUS_ACTION_PORTAL, PCT_STATUS_REQUEST,
                    (char *)&reqinfo, sizeof(pctStatus_req));
 
        if (rc){
            log_warning("Can't send status request to node %d, will retry\n", idx);

	    if (retry_send(idx, PPID_PCT,
		     PCT_STATUS_ACTION_PORTAL, PCT_STATUS_REQUEST,
		     (char *)&reqinfo, sizeof(pctStatus_req))){

                log_warning("Retry failed as well\n");
	    } 
	}

#endif

        larray[idx].status = STATUS_UNREPORTED;
        ++nreq;
    }

#ifdef RELIABLE
    if (num_targets > 0) {
        rc = srvr_send_to_control_ptls(num_targets, target_nids, &target_ppid, &target_ptl, 
                                       PCT_STATUS_REQUEST,
                                       (char *)&(reqinfo), 
                                       sizeof(pctStatus_req), 1, wait_for);
        if (rc < 0) {
            log_error("error sending status requests to pcts\n");
        }

        /* rc if > 0 indicates actual # targets msg was sent to */
        num_to_wait_for = rc;

        /* mark nodes we failed to send to as down */
        for (i = 0; i < num_targets; i++) {
            if (wait_for[i] == 0) {
              j = target_nids[i]; 
              larray[j].status = STATUS_DOWN;
#ifdef STRIPDOWN
              /* unregister the node -- it will re-register
                 if it comes up again */
              Registered[j] = 0;
              if (interactive_list[j]) {
                interactive_list[j] = 0;
              }
              else {
                scheduled_list[j] = 0;
                activeNodes--;            
              }
#endif
            }
        }
    }
#endif

 
    LOCATION("solicit_pct_updates", "await updates");

    td = dclock();

    num = nreq;

#ifdef RELIABLE
    nreq = num_to_wait_for;
    num  = nreq;
    done = 0;

    while (done == 0) {
#else
    while (nreq > 0){
#endif
 
        if ((nfound = update_pct_pid_array(COUNT_NEW)) > 0){
	    /*
	    ** nfound is the count of messages from PCTs
	    ** whose former status was STATUS_UNREPORTED.
	    */
            nreq -= nfound;
        }

#ifdef RELIABLE
        /* wait for responses for all of the targets that were actually solicited */
        done = 1;
        for (i = 0; i < num_targets; i++) {
            if (wait_for[i] == 1) {
                if (larray[target_nids[i]].status == STATUS_UNREPORTED) {
                    done = 0;
                    break;
                }
            }
        }
#endif

        /* note, while bebopd is waiting here on the pct, pingd
           is waiting on bebopd... don't use the same timeout...
           uh, bebopd's should be somewhat SHORTER.
        */
#ifdef RELIABLE
        if ((dclock() - td) > solicitPctsTmout) {
            tmout_occured = 1;
            break;
        }
#else
        if ((dclock() - td) > WaitPCTstatus) break;
#endif
    }

#ifdef RELIABLE
    if (tmout_occured) {
       log_msg("Sent status request %d seconds ago, missing %d status updates.\n",
                           solicitPctsTmout, nreq);
    }
#else
    if (nreq){
       log_msg("Sent status request %d seconds ago, missing %d status updates.\n",
                           WaitPCTstatus, nreq);
    }
#endif
    
    if (queryall){ 
 
       last_total_pct_update = dclock();

    }

#ifndef STRIPDOWN
       if (PBSsupport){

           update_interactive_list();
       }
       compute_node_states();
#endif
 
    if (PBSsupport){

           compute_active_nodes();
 
        if (PBSupdate){
            update_PBS_server_with_change();
        }
    }

#ifdef RELIABLE
    free(target_nids);
#endif

    return num;
}

#ifndef STRIPDOWN
static void
compute_node_states()
{
int i, intNode;
int pendingInt, pendingPbs, lowPri, killing, intJob, pending;
int killIntOutlierScavengers, killPbsOutlierScavengers;


    memset(loadCandidate, 0, MAX_NODES);
    memset(nodeState, inactiveNode, MAX_NODES);
    memset(nodeStateTotals, 0, sizeof(int) * numNodeStates);
    reservedNodes = 0;

    killIntOutlierScavengers = 0;
    killPbsOutlierScavengers = 0;

    for (i=minPct; i<=maxPct; i++){

        if (larray[i].status == STATUS_NO_STATUS) continue;

        if (larray[i].reserved != -1) {
	        reservedNodes++;
	}

        if (!PBSsupport || (PBSinteractive && (interactive_list[i] == 1))){

	    intNode = 1;
	}
	else{
	    intNode = 0;
	}

	pendingInt = ((larray[i].pending_jid != INVAL) && (larray[i].pending_sid == INVAL));
	pendingPbs = ((larray[i].pending_jid != INVAL) && (larray[i].pending_sid != INVAL));
	pending = (pendingInt || pendingPbs);

	nodeState[i] = inactiveNode;

	if (larray[i].status == STATUS_FREE){            /* free ******************/

	    if (pending){                             /* but pending allocation */

	        if (intNode){
		    if (pendingPbs){
		        nodeState[i] = pbsOutlier;
		    }
		    else{
		        nodeState[i] = interactivePending;
		    }
		}
		else{
		    if (pendingInt){
		        nodeState[i] = interactiveOutlier;
		    }
		    else{
		        nodeState[i] = pbsPending;
		    }
		}
	    }
	    else{                                        /* not pending allocation */

                if (intNode){
		    nodeState[i] = interactiveFree;
		}
		else{
		    nodeState[i] = pbsFree;
		}

		loadCandidate[i] = 1;   /* allocatable */
	    }
	}
	else if (larray[i].status == STATUS_BUSY){       /* busy *******************/

            intJob = (larray[i].session_id == INVAL);
	    lowPri = (larray[i].priority == SCAVENGER);
	    killing = (larray[i].user_status & NICE_KILL_JOB_STARTED);

	    if (intNode && !intJob){                   /* running an outlier */
	        nodeState[i] = pbsOutlier;

		if (lowPri) killPbsOutlierScavengers = 1;
	    }
	    else if (!intNode && intJob){
	        nodeState[i] = interactiveOutlier;

		if (lowPri) killIntOutlierScavengers = 1;
	    }
	    else if (killing){          /* running a job that is being killed */
		/*
		** being killed and already allocated to a regular job
		*/
	        if (pending){
		     if (intNode){
	                nodeState[i] = interactiveDyingPending;
		     }
		     else{
	                nodeState[i] = pbsDyingPending;
		     }
		}
		/*
		** being killed but not already allocated - we can use it
		*/
		else{

		     if (intNode){
	                nodeState[i] = interactiveDying;
		     }
		     else{
	                nodeState[i] = pbsDying;
		     }
		}
	    }
	    else if (lowPri){                      /* scavenger job (killable) */

		if (intNode){
		    nodeState[i] = interactiveLowRunning;
		}
		else{
		    nodeState[i] = pbsLowRunning;
		}
	    }
	    else{                                       /* regular job */
	        if (intNode){
		    nodeState[i] = interactiveRegular;
		}
		else{
		    nodeState[i] = pbsRegular;
		}
	    }

	    if (killing && !pending){
		 loadCandidate[i] = 1;  /* allocatable */
	    }
	}

	nodeStateTotals[nodeState[i]]++;
    }

    if (killIntOutlierScavengers){
        clearOutlierScavengers(INTERACTIVE);
    }
    if (killPbsOutlierScavengers){
        clearOutlierScavengers(SCHEDULED);
    }
}
#endif /* !STRIPDOWN */

void
log_node_state_info()
{
int intLow, pbsLow;

    log_msg("NODE STATE");

    intLow = nodeStateTotals[interactiveLowRunning]+
	 nodeStateTotals[interactiveDyingPending]+
	 nodeStateTotals[interactiveDying];

    pbsLow = nodeStateTotals[pbsLowRunning]+
	 nodeStateTotals[pbsDyingPending]+
	 nodeStateTotals[pbsDying];

    log_msg( "Interactive nodes: Free %d, Pending %d, Busy running interactive job %d\n",
	 nodeStateTotals[interactiveFree],
	 nodeStateTotals[interactivePending],
	 nodeStateTotals[interactiveRegular]+intLow);

    if (nodeStateTotals[pbsOutlier]){
	log_msg("    PBS jobs on interactive nodes: %d\n",
			     nodeStateTotals[pbsOutlier]);
	log_msg("    (nodes will return to interactive use when the PBS jobs complete)\n");
    }
    if (intLow){
        log_msg("    B|   Regular priority: %d\n",
	                  nodeStateTotals[interactiveRegular]);
	log_msg("    U|   Low priority, running: %d\n",
	                  nodeStateTotals[interactiveLowRunning]);
	log_msg("    S|   Low priority, dying, not allocated: %d\n",
	                  nodeStateTotals[interactiveDying]);
	log_msg("    Y|   Low priority, dying, already allocated: %d\n",
	                  nodeStateTotals[interactiveDyingPending]);
    }

    log_msg( "PBS nodes: Free %d, Pending %d, Busy running PBS jobs %d\n",
	 nodeStateTotals[pbsFree],
	 nodeStateTotals[pbsPending],
	 nodeStateTotals[pbsRegular]+pbsLow);

    if (nodeStateTotals[interactiveOutlier]){
	log_msg("    interactive jobs on PBS nodes: %d\n",
			     nodeStateTotals[interactiveOutlier]);
	log_msg("    (nodes will return to scheduled use when the interactive jobs complete)\n");
    }
    if (pbsLow){
        log_msg("    B|   Regular priority: %d\n",
	                  nodeStateTotals[pbsRegular]);
	log_msg("    U|   Low priority, running: %d\n",
	                  nodeStateTotals[pbsLowRunning]);
	log_msg("    S|   Low priority, dying, not allocated: %d\n",
	                  nodeStateTotals[pbsDying]);
	log_msg("    Y|   Low priority, dying, already allocated: %d\n",
	                  nodeStateTotals[pbsDyingPending]);
    }

    log_msg( "PBS active count %d\n", activeNodes);
    log_msg( "PBS unused count %d\n", unusedNodes);

    if (interactive_size_req || interactive_list_req_size){

	log_msg("%d Interactive nodes: %s\n",
		      PBSinteractive,
		      make_interactive_node_string(interactive_list));

	if (interactive_size_req){
	    log_msg("interactive partition size requested was %d\n",
			   interactive_size_req);
	}
	else{
	    log_msg("interactive node list requested was %s\n",
		      make_interactive_node_string(interactive_list_req));
	}

    }
}

 
#define BUSY_CANDIDATE(num, euid, type) \
  ( (loadCandidate[num] == 1)       &&  \
    ( (type == INTERACTIVE) ?           \
     (nodeState[num] == interactiveDying) : (nodeState[num] == pbsDying) )  && \
    ((larray[num].reserved == -1) || (larray[num].reserved == euid)) )
 
#define FREE_CANDIDATE(num, euid, type) \
  ( (loadCandidate[num] == 1)       &&  \
    ( (type == INTERACTIVE) ?           \
     (nodeState[num] == interactiveFree) : (nodeState[num] == pbsFree) )  && \
    ((larray[num].reserved == -1) || (larray[num].reserved == euid)) )

static int CompoundReqListSize=0;
static yod_request *CompoundReqList=NULL;
static nid_type    **nidLists=NULL;
static int         *listElt=NULL;
static char        *specificity=NULL;
static int         *fromRank=NULL;

static int
allocRequestStructs(int size)
{
   if (size > CompoundReqListSize){

	CompoundReqList = (yod_request *)realloc(CompoundReqList, size * sizeof(yod_request));

	nidLists        = (nid_type **)realloc(nidLists, size * sizeof(nid_type *));

	listElt         = (int *)realloc(listElt, size * sizeof(int));

	specificity     = (char *)realloc(specificity, size);

	fromRank        = (int *)realloc(fromRank, size * sizeof(int));

	if (!CompoundReqList || !nidLists || !specificity || !listElt || !fromRank){
	    CPerrno = ENOMEM;
	    log_warning("out of memory to hold %d allocation requests\n",size);
	    return BEBOPD_ERR_INTERNAL;
	}
	memset((void *)nidLists, 0, sizeof(nid_type *)*size);

	CompoundReqListSize = size;
    }
    return 0;
}
static void
freeNidLists(void)
{
int i;

    LOCATION("freeNidLists","top");
    
    for (i=0; i< CompoundReqListSize; i++){
	if (nidLists[i]){
	    free(nidLists[i]);
	    nidLists[i] = NULL;
        }
    }
	
}

#ifndef STRIPDOWN
static int
make_pct_list(int nreq, int reqnodes, uid_t euid, int scavenger, int type)
{
int i, rc, status, nid;
int accumnodes, req, nnodes;
int freeCount, freeNow, freeOnly;
int nlistsize, elt, numfree, msgtype;
nodeSpec *spec;
int *allocNodes;
int node1, node2;

    LOCATION("make_pct_list","top");

    if (type == INTERACTIVE){
        freeNow = nodeStateTotals[interactiveFree];
    }
    else if (type == SCHEDULED){
        freeNow = nodeStateTotals[pbsFree];
    }
    else{
       return BEBOPD_ERR_INVALID;
    }

    status = BEBOPD_OK;
    freeCount = 0;
    accumnodes  = 0;

    LOCATION("make_pct_list","find nodes");

    for (req = 0; req < nreq; req++){

        elt = listElt[req];

        numfree = 0;

        msgtype = CompoundReqList[elt].specType;
        spec    = &(CompoundReqList[elt].spec);
        nnodes =  CompoundReqList[elt].nnodes;

        if ((accumnodes+nnodes) > reqnodes){
            log_warning("Compound request individual node requests sum to\n");
            log_warning("more than total node request.???\n");
            status = BEBOPD_ERR_INVALID;
            break;
        }

        allocNodes = pct_info + fromRank[listElt[req]];

        if (msgtype == YOD_NODE_REQ_LIST){
            /*
            ** We will make a list of the first nnodes pcts on the list
            ** found to be available.  We will maintain the order of the
            ** node list passed in.  It may be significant to yod if compute
            ** partition contains heterogeneous nodes.
            */

            LOCATION("make_pct_list", "YOD_NODE_REQ_LIST");
            nlistsize = spec->list.listsize;

            for (i=0; (i<nlistsize) && (numfree<nnodes); i++){

                nid = nidLists[elt][i];

                if ( (nid < minPct) || (nid > maxPct) ) continue;

                if (!scavenger && BUSY_CANDIDATE(nid, euid,type)){
                    allocNodes[numfree++] = nid;
                    loadCandidate[nid]    = 2;
                }
                else if (FREE_CANDIDATE(nid, euid,type)){
                    allocNodes[numfree++] = nid;
                    loadCandidate[nid]    = 2;
                    freeCount++;
                }
            }
        }
        else if (msgtype == YOD_NODE_REQ_RANGE){

            LOCATION("make_pct_list", "YOD_NODE_REQ_RANGE");
            node1 = spec->range.from_node;
            node2 = spec->range.to_node;

            if (node1 < node2){

                node1 = ((node1 < minPct) ? minPct : node1);
                node2 = ((node2 > maxPct) ? maxPct : node2);

                for (nid=node1; (nid<=node2) && (numfree<nnodes); nid++){

                    if (!scavenger && BUSY_CANDIDATE(nid, euid, type)){
                        allocNodes[numfree++] = nid;
                        loadCandidate[nid]    = 2;
                    }
                    else if (FREE_CANDIDATE(nid, euid, type)){
                        allocNodes[numfree++] = nid;
                        loadCandidate[nid]    = 2;
                        freeCount++;
                    }
                }

            }
            else{
                node2 = ((node2 < minPct) ? minPct : node2);
                node1 = ((node1 > maxPct) ? maxPct : node1);

                for (nid=node1; (nid>=node2) && (numfree<nnodes); nid--){

                    if (!scavenger && BUSY_CANDIDATE(nid, euid, type)){
                        allocNodes[numfree++] = nid;
                        loadCandidate[nid]    = 2;
                    }
                    else if (FREE_CANDIDATE(nid, euid, type)){
                        allocNodes[numfree++] = nid;
                        loadCandidate[nid]    = 2;
                        freeCount++;
                    }
                }
            }
        }
        else{   /* request is for any nodes out there*/

            freeOnly = (scavenger || ((freeNow - freeCount) >= nnodes));
#ifdef REMAP
            if (Remap) {
              for (nid=minMap; nid<=maxMap; nid++) {

                if (carray[nid] != -1) {

                  if (larray[carray[nid]].status == STATUS_NO_STATUS) continue;

                  if (FREE_CANDIDATE(carray[nid], euid, type) ||
                    (!freeOnly && BUSY_CANDIDATE(carray[nid], euid, type)) ){

                    free_list[numfree++] = carray[nid];

                  }
                }
              }
              rc = smart_alloc(free_list, numfree, nnodes, go_list);
            }
            else {
#endif

              for (nid=minPct; nid<=maxPct; nid++){

                  if (larray[nid].status == STATUS_NO_STATUS) continue;

                  if (FREE_CANDIDATE(nid, euid, type) ||

                      (!freeOnly && BUSY_CANDIDATE(nid, euid, type)) ){

                      free_list[numfree++] = nid;
                  }

              }

              rc = smart_alloc(free_list, numfree, nnodes, go_list);
#ifdef REMAP
            }
#endif

            if (rc == 0){
                for (i=0; i<nnodes; i++){

                    if (larray[go_list[i]].status == STATUS_FREE){
                        freeCount++;
                    }

                    allocNodes[i]             = go_list[i];
                    loadCandidate[go_list[i]] = 2;
                }
                numfree = nnodes;
            }
            else{
                numfree = 0;   /* flags an error later on */
            }
        }

        if (numfree < nnodes){
            break;
        }
        else{
            accumnodes += nnodes;
        }
    }
    /*
    ** Free up the PCTs we didn't allocate
    */
    timing_updates += deallocate_free_pcts();

    if (accumnodes != reqnodes){

        if (status == BEBOPD_OK){

            status = BEBOPD_ERR_FREE_NODES;
        }

        /*
        ** can not use the allocated ones either
        */
        for (nid=minPct; nid<=maxPct; nid++){

	    if (larray[nid].status == STATUS_NO_STATUS) continue;

            if (loadCandidate[nid] == 2){
                timing_updates += deallocate_pct(nid);
            }
        }
    }
    return status;
}
static int
allocate_nodes(int nrequests, int reqnodes, int jid, uid_t euid, int priority)
{
int status, rc;
int freeNow, freeSoon, killable;
int scavenger;
 
    LOCATION("allocate_nodes","top");

    status      = BEBOPD_OK;

    timing_queries = timing_updates = 0;

    LOCATION("allocate_nodes","query all pcts");

    timing_queries = solicit_pct_updates(NULL, 0, REQ_FOR_YOD, 0, jid, INVAL);

    freeNow  = nodeStateTotals[interactiveFree];
    freeSoon = nodeStateTotals[interactiveDying];
    killable = nodeStateTotals[interactiveLowRunning];

    scavenger = (priority == SCAVENGER);

    if (scavenger){
    
        if (reqnodes > freeNow){

	    if (PBSsupport){
		status = BEBOPD_ERR_INSUFF_INT_NODES;
	    }
	    else{
		status = BEBOPD_ERR_FREE_NODES;
	    }
	}
    }
    else{      /* regular priority jobs */

	if (reqnodes > (freeNow + freeSoon)) {

	    if (reqnodes <= (freeNow + freeSoon + killable)){

		rc = clearScavengersForJob(reqnodes, INTERACTIVE);

		if (rc == 0){
		    /*
		    ** We killed off enough scavengers, begin the
		    ** load process again.  These guys will
		    ** transition from "killable" to "freeSoon"
		    ** after the next PCT update.
		    */
		    status = BEBOPD_ERR_TRY_AGAIN;
		}
            }

	    if (status != BEBOPD_ERR_TRY_AGAIN){
		if (PBSsupport){
		    status = BEBOPD_ERR_INSUFF_INT_NODES;
		} 
		else{
		    status = BEBOPD_ERR_FREE_NODES;
		}
	    }
	}
    }

    if (status != BEBOPD_OK){
	timing_updates = deallocate_free_pcts();
	return status;
    }

    /*
    ** Now allocate nodes to requests.
    */

    rc = make_pct_list(nrequests, reqnodes, euid, scavenger, INTERACTIVE);

    return rc;
}

static int
allocate_nodes_for_pbs(int nrequests, int reqnodes,
                       int sessionID, int sessionLimit,
		       int jid, uid_t euid, int priority)
{

int status, i, pbsJobNodes, rc;
int freeNow, freeSoon, killable;
int scavenger;

    LOCATION("allocate_nodes_for_pbs", "top");

    status = BEBOPD_OK;
    pbsJobNodes = 0;

    timing_queries = timing_updates = 0;

    scavenger = (priority == SCAVENGER);

    /*
    ** All PCTs will report their status.  If they are not pending allocation
    ** to another job, AND if they are either FREE or the job they are running
    ** is in the process of being killed off, they are considered eligible for 
    ** allocation to the requesting yod process.
    **
    ** Upon receiving this status request from bebopd, such a PCT considers 
    ** itself as pending allocation to the yod job.  If the PCT
    ** receives a NOT_ALLOCATED message from bebopd, it goes back to "not
    ** pending allocation".  If the yod process does not eventually load an
    ** application onto the PCT, the PCT will also give up and go back
    ** to "not pending allocation".
    */
    
    LOCATION("allocate_nodes_for_pbs", "solicit_pct_updates");

    if (debug_toggle){
	log_msg("allocate_nodes_for_pbs: seeking %d nodes for %s job\n",
	         reqnodes, (priority==SCAVENGER) ? "scavenger" : "regular");
    }

    timing_queries = solicit_pct_updates(NULL,0,REQ_FOR_YOD,0, jid, sessionID);

    if (debug_toggle){
	log_msg("    %d queries",timing_queries);
    }

    freeNow  = nodeStateTotals[pbsFree];
    freeSoon = nodeStateTotals[pbsDying];
    killable = nodeStateTotals[pbsLowRunning];

    for (i=minPct; i<=maxPct; i++){

        if (larray[i].status == STATUS_NO_STATUS) continue;

	if ( (larray[i].session_id == sessionID) ||     /* running now */
	     (larray[i].pending_sid == sessionID)   ){  /* running next */

		pbsJobNodes++;   /* node "i" is already in use by this PBS job */
	}
    }

    if (debug_toggle){
        log_msg("    minPct %d maxPct %d",minPct,maxPct);
	log_msg("    %d free, %d running regular, %d running scavenger, %d scavengers dying\n",
		nodeStateTotals[pbsFree], 
		nodeStateTotals[pbsRegular],
		nodeStateTotals[pbsLowRunning],
		nodeStateTotals[pbsDyingPending] + nodeStateTotals[pbsDying]);
    }

    if ((pbsJobNodes + reqnodes) > sessionLimit){   /* PBS job over node limit */
	status = BEBOPD_ERR_SESSION_LIMIT;
    }
    else if (scavenger){       /* cycle-stealing PBS jobs */

        if (reqnodes > freeNow){
	    status = BEBOPD_ERR_FREE_NODES;
	}
    }                
    else{                    /* regular priority PBS jobs */

        if (reqnodes > (freeNow + freeSoon)){

            /*
            ** Not enough nodes available to run the job.  Try two
            ** things.  First see if there are low priority cycle-
            ** stealing jobs out there we can kill off.  That
            ** should take care of it, since PBS doesn't schedule
            ** more jobs than we have nodes for.  If there still
            ** are insufficient nodes, look for rogue PBS jobs -
            ** those are jobs that PBS believes have terminated,
            ** but they are still using compute nodes.  This can
            ** happen when messages get lost in the network.
            */

	    if (reqnodes <= (freeNow + freeSoon + killable)){

               rc = clearScavengersForJob(reqnodes, SCHEDULED);

                if (rc == 0){
                    /*
                    ** We killed off enough scavengers, begin the
                    ** load process again.  These guys will
                    ** transition from "killable" to "freeSoon"
                    ** after the next PCT update.
                    */
                    status = BEBOPD_ERR_TRY_AGAIN;
                }

	    }
	    if (status != BEBOPD_ERR_TRY_AGAIN){
		/*
		** Check the compute partition for
		** rogue PBS jobs and kill them.
		** Then try again.  Their nodes should
		** transition from busy to freeNow.
		*/
                rc = recover_free_pbs_nodes();

                if (rc == 0){
                    status = BEBOPD_ERR_TRY_AGAIN;
                } 
	    }
	    if (status != BEBOPD_ERR_TRY_AGAIN){
		status = BEBOPD_ERR_FREE_NODES;
	    }
	}
    }

    if (status != BEBOPD_OK){
        timing_updates = deallocate_free_pcts();

        if (debug_toggle){
	    log_msg("    failed: %d pcts released",timing_updates);
        }
        return status; 
    }
    LOCATION("allocate_nodes_for_pbs", "find nodes to satisfy request");

    rc = make_pct_list(nrequests, reqnodes, euid, scavenger, SCHEDULED);

    return rc;
}
#endif /* !STRIPDOWN */

#ifdef STRIPDOWN
static int
make_pct_list_iactive(int nreq, int rnodes, uid_t euid)
{
  int i, status, nid;
  int accumnodes, req, nnodes;
  int nlistsize, elt, msgtype;
  nodeSpec *spec;
  int *allocNodes;
  int node1, node2;
  int acquired;
  int numfree, rc;

  LOCATION("make_pct_list_iactive","top");

  status = BEBOPD_OK;
  accumnodes = 0;

  for (req = 0; req < nreq; req++){

    elt = listElt[req];

    msgtype = CompoundReqList[elt].specType;
    spec    = &(CompoundReqList[elt].spec);
    nnodes =  CompoundReqList[elt].nnodes;

    if ((accumnodes+nnodes) > rnodes){
      log_warning("Compound request individual node requests sum to\n");
      log_warning("more than total node request.???\n");
      status = BEBOPD_ERR_INVALID;
      break;
    }

    allocNodes = pct_info + fromRank[listElt[req]];
    acquired   = 0;

    switch (msgtype) {

      case YOD_NODE_REQ_ANY:

        /* look in the list of interactive nodes for any
           free ones and allocate them, do not mark them as
           BUSY yet... that only happens after we've been
           able to successfully send all nodes the PCT_ALLOC_REQUEST...
        */
#if 0
        for (i=intMIN; (i<=intMAX && acquired<nnodes); i++) {
          if ( interactive_list[i] && (larray[i].status == STATUS_FREE) ) {
            allocNodes[acquired++] = i;
          }
        } 
#else /* code that uses smart_alloc() */
        numfree = 0;
#ifdef REMAP
        if (Remap) {
          for (i = intMIN; i <= intMAX; i++) {
            if ( (carray[i] != -1) && 
               interactive_list[carray[i]] && (larray[carray[i]].status == 
               STATUS_FREE) && ((larray[carray[i]].reserved == -1) ||
               (larray[carray[i]].reserved == euid)) ) {
                   
                    free_list[numfree++]=carray[i];
             }
           }
         }
         else {
#endif
            for (i=intMIN; i<=intMAX; i++) {
              if ( interactive_list[i] && (larray[i].status == STATUS_FREE) &&
                ((larray[i].reserved == -1) || (larray[i].reserved == euid)) ) {
                    free_list[numfree++] = i;
              }
            }
#ifdef REMAP
        }
#endif
        rc = smart_alloc(free_list, numfree, nnodes, go_list);

        if (rc == 0){
          for (i=0; i<nnodes; i++){
            allocNodes[i]             = go_list[i];
          }
          acquired = nnodes;
        }
        else{
          acquired = 0;   /* flags an error later on */
        }
#endif
        break;

      case YOD_NODE_REQ_LIST:
        /* we will make a list of the first nnodes pcts on 
           the list found to be available. we will maintain
           the order of the node list passed in. it may be 
           significant to yod if compute partition contains
           heterogeneous nodes.  
         */
        LOCATION("make_pct_list_iactive", "YOD_NODE_REQ_LIST");
        nlistsize = spec->list.listsize;

        for (i=0; (i<nlistsize) && (acquired<nnodes); i++){

          nid = nidLists[elt][i];

          if ( (nid < minPct) || (nid > maxPct) ) continue;

          if ( (interactive_list[nid]) && 
                 (larray[nid].status == STATUS_FREE) ) {
            allocNodes[acquired++] = nid;
          }
        }
        break;

      case YOD_NODE_REQ_RANGE:
        LOCATION("make_pct_list_iactive", "YOD_NODE_REQ_RANGE");
        node1 = spec->range.from_node;
        node2 = spec->range.to_node;

        if (node1 < node2){
          node1 = ((node1 < minPct) ? minPct : node1);
          node2 = ((node2 > maxPct) ? maxPct : node2);

          for (nid=node1; (nid<=node2) && (acquired<nnodes); nid++){
            if ( (interactive_list[nid]) && 
                 (larray[nid].status == STATUS_FREE) ) {
              allocNodes[acquired++] = nid;
            }
          }
        }
        else{
          node2 = ((node2 < minPct) ? minPct : node2);
          node1 = ((node1 > maxPct) ? maxPct : node1);
          for (nid=node1; (nid>=node2) && (acquired<nnodes); nid--){
            if ( (interactive_list[nid]) && 
                 (larray[nid].status == STATUS_FREE) ) {
              allocNodes[acquired++] = nid;
            }
          }
        }
        break;

      default:
        status = BEBOPD_ERR_FREE_NODES;
        break;
    }
    if (acquired < nnodes) {
      status = BEBOPD_ERR_FREE_NODES;
      return status;
    }
    else {
      accumnodes += acquired;
    }
  }
  /* "for" level */
  if ( accumnodes != rnodes ) {
    status = BEBOPD_ERR_FREE_NODES;
  }
  return status;
}

/* basically the same as make_pct_list_iactive() except
   search the pbs list of nodes rather than the iactives
*/
static int
make_pct_list_pbs(int nreq, int rnodes, uid_t euid, 
                      int jid, int sessionID)
{
  int i, status, nid;
  int accumnodes, req, nnodes;
  int nlistsize, elt, msgtype;
  nodeSpec *spec;
  int *allocNodes;
  int node1, node2;
  int acquired, numfree, rc;

  LOCATION("make_pct_list_pbs","top");

  status = BEBOPD_OK;
  accumnodes = 0;

  for (req = 0; req < nreq; req++){
    elt = listElt[req];

    msgtype = CompoundReqList[elt].specType;
    spec    = &(CompoundReqList[elt].spec);
    nnodes =  CompoundReqList[elt].nnodes;

    if ((accumnodes+nnodes) > rnodes){
      log_warning("Compound request individual node requests sum to\n");
      log_warning("more than total node request.???\n");
      status = BEBOPD_ERR_INVALID;
      break;
    }

    allocNodes = pct_info + fromRank[listElt[req]];
    acquired   = 0;

    switch (msgtype) {

      case YOD_NODE_REQ_ANY:

        /* look in the list of pbs nodes for any
           free ones and allocate them, mark as busy...
        */
#if 0
        for (i=pbsMIN; (i<=pbsMAX && acquired<nnodes); i++) {
          if ( scheduled_list[i] && (larray[j].status == STATUS_FREE) ) {
            allocNodes[acquired++] = i;
          }
        } 
#else /* use smart_alloc() */
        numfree=0;
#ifdef REMAP
        if (Remap) {
          for (i=pbsMIN; i<=pbsMAX; i++) {
            if ( (carray[i] != -1) && scheduled_list[carray[i]] &&
                 (larray[carray[i]].status == STATUS_FREE) &&
                 ((larray[carray[i]].reserved == -1 ) ||
                 (larray[carray[i]].reserved == euid)) ) {

                    free_list[numfree++] = carray[i];
            }
          }
        }
        else {
#endif
          for (i=pbsMIN; i<=pbsMAX; i++) {
            if ( scheduled_list[i] && (larray[i].status == STATUS_FREE) &&
              ((larray[i].reserved == -1) || (larray[i].reserved == euid)) ) {
                  free_list[numfree++] = i;
            }
          }
#ifdef REMAP
        }
#endif
        rc = smart_alloc(free_list, numfree, nnodes, go_list);

        if (rc == 0){
          for (i=0; i<nnodes; i++){
            allocNodes[i]             = go_list[i];
          }
          acquired = nnodes;
        }
        else{
          acquired = 0;   /* flags an error later on */
        }
#endif
        break;

      case YOD_NODE_REQ_LIST:
        /* we will make a list of the first nnodes pcts on 
           the list found to be available. we will maintain
           the order of the node list passed in. it may be 
           significant to yod if compute partition contains
           heterogeneous nodes.  
         */
        LOCATION("make_pct_list_pbs", "YOD_NODE_REQ_LIST");
        nlistsize = spec->list.listsize;

        for (i=0; (i<nlistsize) && (acquired<nnodes); i++){

          nid = nidLists[elt][i];

          if ( (nid < minPct) || (nid > maxPct) ) continue;

          if ( (scheduled_list[nid] == 1) && 
                 (larray[nid].status == STATUS_FREE) ) {
            allocNodes[acquired++] = nid;
          }
        }
        break;

      case YOD_NODE_REQ_RANGE:
        LOCATION("make_pct_list_pbs", "YOD_NODE_REQ_RANGE");
        node1 = spec->range.from_node;
        node2 = spec->range.to_node;

        if (node1 < node2){
          node1 = ((node1 < minPct) ? minPct : node1);
          node2 = ((node2 > maxPct) ? maxPct : node2);

          for (nid=node1; (nid<=node2) && (acquired<nnodes); nid++){
            if ( (scheduled_list[nid] == 1) && 
                 (larray[nid].status == STATUS_FREE) ) {
              allocNodes[acquired++] = nid;
            }
          }
        }
        else{
          node2 = ((node2 < minPct) ? minPct : node2);
          node1 = ((node1 > maxPct) ? maxPct : node1);
          for (nid=node1; (nid>=node2) && (acquired<nnodes); nid--){
            if ( (scheduled_list[nid] == 1) && 
                 (larray[nid].status == STATUS_FREE) ) {
              allocNodes[acquired++] = nid;
            }
          }
        }
        break;

      default:
        status = BEBOPD_ERR_FREE_NODES;
        break;
    }
    if (acquired < nnodes) {
      status = BEBOPD_ERR_FREE_NODES;
      return status;
    }
    else {
      accumnodes += acquired;
    }
  }
  /* "for" level */
  if ( accumnodes != rnodes ) {
    status = BEBOPD_ERR_FREE_NODES;
  }
  return status;
}
static int
allocate_nodes(int nreq, int rnodes, int jid, uid_t euid, 
               int priority)
{
  int status= BEBOPD_OK;
  int rc;
 
    LOCATION("allocate_nodes","top");

    compute_iactive_totals();

    if (rnodes > IactiveFree) {
      status = BEBOPD_ERR_INSUFF_INT_NODES;
    }

    if (status != BEBOPD_OK){
	return status;
    }

    /* now allocate nodes to requests */
    rc = make_pct_list_iactive(nreq, rnodes, euid);

    return rc;
}

static int
allocate_nodes_for_pbs(int nreq, int nnodes, int sessionID, 
                       int sessionLimit,
                       int jid, uid_t euid, int priority)
{
  int status= BEBOPD_OK; 
  int i, rc, freeNow, pbsJobNodes=0;

    LOCATION("allocate_nodes_for_pbs", "top");

    timing_queries = timing_updates = 0;

    LOCATION("allocate_nodes_for_pbs", "solicit_pct_updates");

    if (debug_toggle){
      log_msg("allocate_nodes_for_pbs: seeking %d nodes for job\n",
                                                  nnodes);
    }

    freeNow = 0;
        /* this is a crude way of ensuring the job stays under the
           PBS session limit -- keeping track of the nodes being
           allocated to and freeing from a session might be more
           efficient -- if we did not have to perform the session
           limit check, we could exit this loop when freeNow =
           nnodes */

#ifdef REMAP
  if (Remap) {
      for (i=pbsMIN; i<=pbsMAX; i++) {
        if (scheduled_list[carray[i]] ) {
          if ( (larray[carray[i]].session_id == sessionID) || /*running now*/
               (larray[carray[i]].pending_sid == sessionID)){ /*running next*/
            pbsJobNodes++; /* node "i" is already in use by this PBS job */
          }
          if ( larray[carray[i]].status == STATUS_FREE ) {
            freeNow++;
          }
        }
      }
    }
    else {
#endif
      for (i=pbsMIN; i<=pbsMAX; i++) {
        if ( scheduled_list[i] ) {
          if ( (larray[i].session_id == sessionID) ||     /* running now */
               (larray[i].pending_sid == sessionID)   ){  /* running next */
            pbsJobNodes++;  /* node "i" is already in use by this PBS job */
          }
          if ( larray[i].status == STATUS_FREE ) {
            freeNow++;
          }
        }
      }
#ifdef REMAP
    }
#endif

    if ((pbsJobNodes + nnodes) > sessionLimit){ /* PBS job over node limit */
      status = BEBOPD_ERR_SESSION_LIMIT;
      log_msg("PBSscheduled=%d, sessionID=%d",PBSscheduled,sessionID);
      log_msg("pbsJobNodes=%d, nnodes=%d, sessionLimit=%d",pbsJobNodes,nnodes,sessionLimit);
      return status;
    }

    if (nnodes > freeNow){
      status = BEBOPD_ERR_FREE_NODES;
      return status; 
    }

    LOCATION("allocate_nodes_for_pbs", "find nodes to satisfy request");

    rc = make_pct_list_pbs(nreq, nnodes, euid, jid, sessionID);

    return rc;
}
#endif /* STRIPDOWN */


#define WANTS_ALL_OF_LIST  1
#define WANTS_PART_OF_LIST 2
#define WANTS_ANY          3

static int 
request_specificity(int type, yod_request *req)
{
int listsize, status;

    if (type == YOD_NODE_REQ_ANY){
        status = WANTS_ANY;
    }
    else if (type == YOD_NODE_REQ_RANGE){
	if (req->spec.range.to_node > req->spec.range.from_node) 
	  listsize = req->spec.range.to_node - req->spec.range.from_node + 1;
	else
	  listsize = req->spec.range.from_node - req->spec.range.to_node + 1;

	if (req->nnodes == listsize){
	    status = WANTS_ALL_OF_LIST;
	} 
	else{
	    status = WANTS_PART_OF_LIST;
	}
    }
    else if (type == YOD_NODE_REQ_LIST){
	if (req->nnodes == req->spec.list.listsize){
	    status = WANTS_ALL_OF_LIST;
	}
	else{
	    status = WANTS_PART_OF_LIST;
	}
    }

    return status;
}

int
find_free_nodes(yod_request *yod_data, control_msg_handle *mhandle,
		  int **pctList, int jid)
{
uid_t euid;
int rc, i, ii, type;
int nreq, reqnodes, priority, compound;
int sessionID, sessionLimit, pbsJob;
nid_type yodnid;
int yodpid, len;
int yodptl;
int nlistsize, rank;
int status;


    reqnodes = yod_data->nnodes;
    sessionID = yod_data->session_id;
    sessionLimit = yod_data->nnodes_limit;
    priority = yod_data->priority;
    euid = yod_data->euid;
    type = yod_data->specType;

    compound = (type == YOD_NODE_REQ_COMPOUND);

    yodnid = SRVR_HANDLE_NID(*mhandle);
    yodpid = SRVR_HANDLE_PID(*mhandle);
    yodptl = yod_data->myptl;

    if (sessionID == INVAL){
	pbsJob = 0;
    }
    else{
	pbsJob = 1;
    }

    status = BEBOPD_OK;

    if (pbsJob && !PBSsupport){
	status = BEBOPD_ERR_NO_PBS_SUPPORT;
    }

#ifndef STRIPDOWN
    if (!pbsJob && PBSsupport && (PBSinteractive < reqnodes)){
 
        if (PBSinteractive == 0){
            status = BEBOPD_ERR_NO_INT_SUPPORT;
        }
        else{
            status = BEBOPD_ERR_INSUFF_INT_NODES;
        }
    }
#endif

    if (status != BEBOPD_OK){
	return status;
    }

    if (CompoundReqListSize == 0){
        rc = allocRequestStructs(1);
        if (rc) return rc;
    }

    freeNidLists();
    memset((char *)pct_info, 0, sizeof(int) * reqnodes);

    /*
    ** Load files may contain compound requests.  Go get compound
    ** request from yod.
    */

    if (compound){

        nreq     = yod_data->spec.req.numRequests;

        /*
        ** wait for yod to send the list of node requests
        */
        if (Dbglevel){
            log_msg("Request compound node request from yod\n");
        }

        if (nreq > CompoundReqListSize){
	    rc = allocRequestStructs(nreq);

            if (rc) return rc;
	}

        rc = srvr_comm_get_req( (char *)CompoundReqList, 
                nreq * sizeof(yod_request), BEBOPD_GET_COMPOUND_REQ, 
		(char *)&bebopd_ok, sizeof(bebopd_status),
		yodnid, yodpid, yodptl, 1, clientWaitLimit);
 
        if (rc < 0){
            log_warning("can not get compound request from yod\n");
            return BEBOPD_ERR_INTERNAL;
        }
        for (i=0; i<nreq; i++){
	    rc = check_yod_request(CompoundReqList+i);

            if (rc < 0){
		log_warning("Bad compound node allocation request, ignored.\n");
                return BEBOPD_ERR_INVALID;
            }
        }
        if (Dbglevel){
            log_msg("Compound node request received\n");
        }
        /*
        ** Some of the requests may ask for nodes from a restricted 
        ** set of nodes.  Others ask for any nodes.  In order to more
        ** likely satisfy the request, process them in this order:
	**
        **   1.  requests that ask for all members of a specific
        **             set of nodes  "yod -l 0..8 myprog"
        **   2.  requests that ask for some subset of a specific
	**             set of nodes  "yod -l 0..15 -sz 2 myprog"
	**   3.  requests that ask for nodes, without specifying 
	**             particular node numbers  "yod -sz 2 myprog"
	**
        ** We still need to return a node list that maintains the 
	** order of the original request.
        */
        ii = 0;
        rank = 0;
        for (i=0; i<nreq; i++){

            fromRank[i] = rank;
            rank += CompoundReqList[i].nnodes;

	    specificity[i] = request_specificity(CompoundReqList[i].specType, 
	                                         CompoundReqList+i);
        }
        for (i=0; i<nreq; i++){
	    if (specificity[i] == WANTS_ALL_OF_LIST){
                listElt[ii++] = i;
            }
        }
        for (i=0; i<nreq; i++){
	    if (specificity[i] == WANTS_PART_OF_LIST){
                listElt[ii++] = i;
            }
        }
        for (i=0; i<nreq; i++){
	    if (specificity[i] == WANTS_ANY){
                listElt[ii++] = i;
            }
        }
    }
    else{
	/*
	** Store the simple node request in the first entry of
	** the compound list.
	*/

        memcpy((void *)CompoundReqList, yod_data, sizeof(yod_request));

        nreq = 1;
        listElt[0] = 0;
        fromRank[0] = 0;
    }
    /*
    ** The node allocation request may specify a list of nodes, or some
    ** members of a compound request may specify a list of nodes.  Get
    ** the list or lists from yod.
    */

    for (i=0; i<nreq; i++){

	if (CompoundReqList[i].specType != YOD_NODE_REQ_LIST){
	    continue;
	}
	nlistsize = CompoundReqList[i].spec.list.listsize;
	
	len = nlistsize * sizeof(nid_type);

	nidLists[i] = malloc(len);

	if (!nidLists[i]){
	    log_warning("error allocating nid list memory %d\n",len);
	    finish_bebopd();
	}
        if (Dbglevel){
            log_msg("Request node list from yod\n");
        }
	
	rc = srvr_comm_get_req( (char *)nidLists[i], len,
		BEBOPD_GET_NODE_LIST, 
		(char *)&bebopd_ok, sizeof(bebopd_status),
		yodnid, yodpid, yodptl, 1, clientWaitLimit);

	if (rc < 0){
	    log_warning("can not get node list from yod\n");
	    freeNidLists();
	    return BEBOPD_ERR_INTERNAL;
	}
        if (Dbglevel){
            log_msg("Node list received\n");
        }
    }

    if (pbsJob){
        
	rc = allocate_nodes_for_pbs(nreq, reqnodes, sessionID,
		      sessionLimit, jid, euid, priority);

        if (rc == BEBOPD_ERR_TRY_AGAIN){
	    rc = allocate_nodes_for_pbs(nreq, reqnodes, sessionID,
		      sessionLimit, jid, euid, priority);
	}
    }
    else{
	rc = allocate_nodes(nreq, reqnodes, jid, euid, priority);

        if (rc == BEBOPD_ERR_TRY_AGAIN){
	    rc = allocate_nodes(nreq, reqnodes, jid, euid, priority);
	}
    }
    freeNidLists();

    *pctList = pct_info;

    return rc;
}

#ifdef STRIPDOWN
void compute_iactive_totals(void)
{
  int i;
  INT16 status;


  IactiveFree = 0;
  IactiveBusy = 0;
#ifdef REMAP
  if (Remap) {
    for (i=intMIN; i<=intMAX; i++) {
      if ( interactive_list[carray[i]] ) {
        status = (larray[carray[i]].status);
        if ( status == STATUS_BUSY || status == STATUS_ALLOCATED ) {
          IactiveBusy++;
        }
        if ( status == STATUS_FREE ) IactiveFree++;
      }
    }
  }
  else {
#endif
    for (i=intMIN; i<=intMAX; i++) {
      if ( interactive_list[i] ) {
        status = larray[i].status;
        if ( status == STATUS_BUSY || status == STATUS_ALLOCATED ) {
          IactiveBusy++;
        }
        if ( status == STATUS_FREE ) IactiveFree++;
      }
    }
#ifdef REMAP
  }
#endif
}


/*** NOTE TO JOHN ***
 ** We never, ever, ever use this. The function is all wrong now. It uses a 
 ** variable that is never anything but 0 (PBSscheduled). If I don't
 ** take this out of the head before you merge, please please remove it.
***/
void compute_pbs_totals(void)
{
  int i;
  INT16 status;

  PbsFree = 0;
  PbsBusy = 0;
#ifdef REMAP
  if (Remap) {
    for (i=0; i<PBSscheduled; i++) {
      status = larray[scheduled_list[carray[i]]].status;
      if ( status == STATUS_BUSY || status == STATUS_ALLOCATED ) {
        PbsBusy++;
      }
      if ( status == STATUS_FREE ) PbsFree++;
    }
  }
  else {
#endif
    for (i=0; i<PBSscheduled; i++) {
      status = larray[scheduled_list[i]].status;
      if ( status == STATUS_BUSY || status == STATUS_ALLOCATED ) {
        PbsBusy++;
      }
      if ( status == STATUS_FREE ) PbsFree++;
    }
#ifdef REMAP
  }
#endif
  activeNodes = PbsBusy + PbsFree;
}
#endif
