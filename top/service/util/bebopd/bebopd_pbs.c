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
** $Id: bebopd_pbs.c,v 1.17.2.4 2002/09/28 00:20:33 jjohnst Exp $
*/
#include "bebopd.h"
#include "bebopd_private.h"
#include "srvr_err.h"
#include "appload_msg.h"
#include "sys_limits.h"
#include "pct_ports.h"
#include "config.h"
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys_limits.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>

char PBSsupport;
char PBSupdate;

/*
** Administrator may request a number of interactive nodes,
** or may specify a node list containing the nodes in the
** interactive partition.  This request may be on the
** bebopd command line, which is processed before the bebopd
** has heard from any PCTs, so we need to claim the nodes
** for interactive use as they check in.  The request may
** also come from pingd while the bebopd is running.
*/
int interactive_size_req=0;      /* number of interactive nodes desired */
int interactive_list_req[MAX_NODES];    /* interactive node list request*/

int interactive_list[MAX_NODES];            /* actual interactive nodes */

int interactive_list_req_size=0;  /* number of nodes on requested list  */
int PBSinteractive=0;             /* number of actual interactive nodes */

#ifdef STRIPDOWN
int scheduled_list[MAX_NODES];
int PBSscheduled=0;
int Registered[MAX_NODES];
int intMIN, intMAX, pbsMIN, pbsMAX;
#endif

int activeNodes=0;
int unusedNodes;
static int reportedActiveNodes=0;
static int reportedUnusedNodes;

extern int minPct, maxPct;
#ifdef REMAP
extern int Remap,maxMap,minMap;
extern nid_type carray[];
#endif

/*
** we need to include UNREPORTED and TROUBLE nodes as active nodes or else
** every time a node goes stale or reports a problem, bebopd would remove
** it from the interactive list and substitute a clean node from the PBS
** partition.  This could cause thrashing of the interactive list and
** problems for PBS jobs.
*/

/* Note : This is no longer true, because the PBS and interactive partitions
** are fixed, so nodes do not get moved from one list to another unless
** done so externally and explicitly. This code should be reevaluated
** at some point, but is probably not harmful at this time.
*/

#define ACTIVE_NODE(i) \
   ((larray[i].status == STATUS_FREE) || (larray[i].status == STATUS_BUSY) || \
    (larray[i].status == STATUS_UNREPORTED) || (larray[i].status == STATUS_TROUBLE))

#define RUNNING_INTERACTIVE(i) \
  ( (larray[i].status == STATUS_BUSY) && (larray[i].session_id == INVAL))

#define RUNNING_SCHEDULED(i) \
  ( (larray[i].status == STATUS_BUSY) && (larray[i].session_id != INVAL))

#define PENDING_INTERACTIVE(i) \
  ((larray[i].pending_sid == INVAL) && (larray[i].pending_jid != INVAL))

#define PENDING_SCHEDULED(i) \
  ((larray[i].pending_sid != INVAL) && (larray[i].pending_jid != INVAL))

#define TYPE_STRING(t) ((t == SCHEDULED) ? "PBS" : "interactive")

/*  If PBSsupport is requested, the bebopd can 
**
**    o  keep track of the number of compute nodes available to run
**       PBS jobs.  If this number changes, for example when a compute node 
**       stops responding to status requests, the bebopd can notify the PBS 
**       server of the change.   
**
**    o  keep track of the number of PBS nodes that are not being used, the
**       PBS scheduler can allocate these to scavenger jobs
**
**    o  enforce the limit of nodes allocated to a particular PBS job. 
**       (Remember a PBS job can include many invocations of yod.)
**
**    o  kill off PBS scavenger jobs to make room for regular PBS jobs
**
**    o  enforce a partitioning of the compute nodes into nodes for 
**       interactive use and nodes for PBS use.
**

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
**  State variables:
**  ===============
**
**  nodeState array - defines the NodeStateType of each node.
**
**  activeNodes      - number of nodes under PBS control, the PBS server's
**                        "resources_available.size"
**
**  unusedNodes      - number of activeNodes which are neither running a regular
**                        PBS job nor pending allocation to a regular PBS job,
**                        the PBS server's "resources_available.unused".
**                        These are nodes that may have been allocated to PBS jobs, but
**                        the jobs are not using them right now.  PBS may wish to schedule 
**                        low priority scavenger jobs on these nodes.  bebopd kills
**                        them off when the official PBS job wants it's nodes.
**
**  reportedActiveNodes - the most recent activeNodes value reported to the
**                          PBS server
**
**  reportedUnusedNodes - the most recent unusedNodes value reported to the PBS server
**
**  PBSsupport - if TRUE, bebopd will maintain the above counts
**
**  PBSupdate - if TRUE, bebopd will report changes in activeNodes and
**                unusedNodes to the PBS server
**
**    ******** interactive partition variables *****************
**
**  PBSinteractive - a count of nodes reserved for interactive use
**
**  interactive_list - specific nodes to be used by interactive jobs
**
**  interactive_size_req - number of nodes administrator requests to be allocated
**                         to interactive use (could be more than PBSinteractive,
**                         we'll try to grab more as they become available)
**       OR
**  interactive_list_req - list of nodes administrator requests to be allocated
**                  to interactive use (could include nodes not on interactive_list
**                  we'll try to get these when they become available)
**
**  pbsOutliers - PBS jobs running on interactive nodes.  We'll reclaim the
**                the nodes for interactive use when the job completes.
**
**  intOutliers - interactive jobs on PBS nodes.  We'll reclaim the
**                the nodes for PBS use when the job completes.
**
** Nodes reserved for interactive use:  On the bebopd command line or with
**  pingd, administrators can request a list of nodes to be used for interactive
**  jobs, or a number of nodes to be used for interactive jobs.  In either case,
**  bebopd build a list of nodes to be used for interactive jobs.  If there
**  are PBS jobs on those nodes, they are allowed to complete before interactive
**  jobs are scheduled on the nodes.  If PBS is started after interactive jobs
**  have been loaded, interactive jobs on PBS nodes will be allowed to complete
**  before the nodes are included in the activeNodes count.
**
*/

void
update_PBS_server_with_change()
{
    if ((activeNodes == reportedActiveNodes) &&
        (unusedNodes == reportedUnusedNodes))   {

        return;
    }
    else{
        update_PBS_server();
    }
    
}

#ifndef STRIPDOWN
void
new_interactive_list(int *list, int len)
{
int i, nid;

    /*
    ** Administrator specified a node list to be the
    ** interactive partition.  We make a second list
    ** which is the nodes from the node list that
    ** are available to be interactive nodes.  
    **
    ** If more of the nodes become available later
    ** on, we'll add them to the interactive_list
    ** with update_interactive_list().
    */

    memset(interactive_list_req, 0, MAX_NODES * sizeof(int));
    memset(interactive_list, 0, MAX_NODES * sizeof(int));

    interactive_size_req = 0;   /* can specify list OR size, not both */

    interactive_list_req_size = 0;
    PBSinteractive            = 0;

    for (i=0; i<len; i++){

        nid = list[i];

	if ( (nid < 0) || (nid >= MAX_NODES)) continue;

	if (interactive_list_req[nid] == 1) continue;

	interactive_list_req_size++;

	interactive_list_req[nid] = 1;

	if ( ACTIVE_NODE(nid)){

	    interactive_list[nid] = 1;
	    PBSinteractive++;
	}
    }
}
#else /* STRIPDOWN */
/* set nodeType field for interactive nodes */
void
new_interactive_list(int *list, int len)
{
int i, nid;

    /* initialize everyone to UNSET */
    intMIN = MAX_NODES;
    intMAX = -1;
    pbsMIN = MAX_NODES;
    pbsMAX = -1;
    for (i=0; i<MAX_NODES; i++){
      larray[i].nodeType = UNSET_NODE;
      Registered[i] = 0;
      scheduled_list[i] = 0;
      interactive_list[i] = 0;
    }

    if ( !PBSsupport ) { /* all nodes are interactive */
      len = MAX_NODES; 
    }

    /* interactive list */
    for (i=0; i<len; i++){
      if ( !PBSsupport ) { /* there is no "list" */
        nid = i;
      }
      else {
        nid = list[i];
      }
	  if ( (nid < 0) || (nid >= MAX_NODES)) continue;

      larray[nid].nodeType = IACTIVE_NODE;
    }
    PBSinteractive= 0;
    PBSscheduled= 0;
    activeNodes= 0;
}
#endif /* STRIPDOWN */

void
update_interactive_list()
{
int nid;


    if (interactive_list_req_size > 0) {    /* SPECIFIED LIST OF INTERACTIVE NODES */
	/*
	** Check the requested interactive node list and
	** update the list of actual interactive nodes.
	*/
	for (nid=minPct; nid<=maxPct; nid++){
	    
            if (larray[nid].status == STATUS_NO_STATUS) continue;

	    if (interactive_list_req[nid] == 0) continue;

            if ( (interactive_list[nid] == 0) && ACTIVE_NODE(nid)){

		interactive_list[nid] = 1;
		PBSinteractive++;
	    }
	    else if ((interactive_list[nid] == 1) && !ACTIVE_NODE(nid)){

		interactive_list[nid] = 0;
		PBSinteractive--;
	    }
        }
    }
    else if (interactive_size_req > 0){   /* SPECIFIED SIZE OF INTERACTIVE PARTITION */

        if (PBSinteractive > 0){
	    for (nid=minPct; nid<=maxPct; nid++){

	        if (larray[nid].status == STATUS_NO_STATUS) continue;

		if ((interactive_list[nid] == 1) && !ACTIVE_NODE(nid)){

		    interactive_list[nid] = 0;
		    PBSinteractive--;
		}
	    }
	}
        /*
	** If the number of actual interactive nodes we have
	** is less than requested, see if we can aquire more.
	*/
	if (PBSinteractive < interactive_size_req){

	    for (nid=minPct; nid <= maxPct; nid++){

	        if (larray[nid].status == STATUS_NO_STATUS) continue;
	
		if ( (interactive_list[nid] == 0) && ACTIVE_NODE(nid)){

		    interactive_list[nid] = 1;
		    PBSinteractive++;

		    if (PBSinteractive == interactive_size_req) break;

		}
	    }
	}
    }
}
void
new_interactive_size(int size)
{
int i;

    /*
    ** Administrator may request a number of nodes to be
    ** the interactive nodes.  We need to create a list
    ** of the actual nodes that will serve as interactive
    ** nodes.  If we don't have enough yet (for example
    ** if bebopd just started up) we'll acquire them
    ** later in update_interactive_list().
    */

    if (size <= 0){
        memset(interactive_list_req, 0, sizeof(int) * MAX_NODES);
        memset(interactive_list, 0, sizeof(int) * MAX_NODES);
	interactive_list_req_size = 0;
	PBSinteractive = 0;
	interactive_size_req = 0;
	log_msg("Interactive partition size set to zero\n");
	return;
    }

    memset(interactive_list_req, 0, sizeof(int) * MAX_NODES);
    interactive_list_req_size = 0;   /* can specify list OR size, not both */

    interactive_size_req = size;

    if (PBSinteractive > size){

        /* need to remove nodes from interactive partition */

#ifdef REMAP
      if (Remap) {
        for (i=maxMap; i>=minMap; i--){

          if (larray[carray[i]].status == STATUS_NO_STATUS) continue; 

          if (interactive_list[carray[i]]){

            if (!RUNNING_INTERACTIVE(carray[i]) &&
                !PENDING_INTERACTIVE(carray[i])) {
         
              PBSinteractive--;
              interactive_list[carray[i]] = 0;
            }
          }  
          if (PBSinteractive == size) break;
        }
      }
      else {
#endif
	    for (i=maxPct; i>=minPct; i--){
  
          if (larray[i].status == STATUS_NO_STATUS) continue;
  
          if (interactive_list[i]){

            if (!RUNNING_INTERACTIVE(i) && !PENDING_INTERACTIVE(i)){
              PBSinteractive--;
              interactive_list[i] = 0;
		      if (PBSinteractive == size) break;
		    }
          }
	    }
#ifdef REMAP
      }
#endif
    }

    if (PBSinteractive == size) return;

    /* need to add nodes to interactive partition */

    for (i=minPct; i<=maxPct; i++){

        if (larray[i].status == STATUS_NO_STATUS) continue;

        if ((interactive_list[i] == 0) &&
	    ( RUNNING_INTERACTIVE(i) || PENDING_INTERACTIVE(i)) ){

		interactive_list[i] = 1;
		PBSinteractive++;
		if (PBSinteractive == size) break;
	    }
    }

    if (PBSinteractive == size) return;

#ifdef REMAP
    if (Remap) {

      for (i=minMap; i<=maxMap; i++){

        if (larray[carray[i]].status == STATUS_NO_STATUS) continue;

        if ((interactive_list[carray[i]] == 0) &&
           (larray[carray[i]].status == STATUS_FREE) &&
            !PENDING_SCHEDULED(carray[i]) ) {

            interactive_list[carray[i]] = 1;
            PBSinteractive++; 
            if (PBSinteractive == size) break;
        }
      }
    }
    else {
#endif
      for (i=minPct; i<=maxPct; i++){
 
         if (larray[i].status == STATUS_NO_STATUS) continue;

          if ((interactive_list[i] == 0) && 
             (larray[i].status == STATUS_FREE) && !PENDING_SCHEDULED(i)  ){

            interactive_list[i] = 1;
            PBSinteractive++;
		    if (PBSinteractive == size) break;
         }
       }
#ifdef REMAP
    }
#endif
    if (PBSinteractive == size) return;

#ifdef REMAP
  if (Remap) {
    for (i=minMap; i<=maxMap; i++) {
      if (larray[carray[i]].status == STATUS_NO_STATUS) continue;

      if ((interactive_list[carray[i]] == 0) && ACTIVE_NODE(carray[i])){
      
        interactive_list[carray[i]] = 1;
        PBSinteractive++;
        if (PBSinteractive == size) break;
      }
    }
  }
  else {
#endif
    for (i=minPct; i<=maxPct; i++){

      if (larray[i].status == STATUS_NO_STATUS) continue;

      if ((interactive_list[i] == 0) && ACTIVE_NODE(i)){

        interactive_list[i] = 1;
        PBSinteractive++;
        if (PBSinteractive == size) break;
      }
    }
#ifdef REMAP
  }
#endif
}

static char *listStr=NULL;
static char *emptyList="( no nodes )";
static char *badList="( ??? )";

char *
make_interactive_node_string(int *ilist)
{
int i, ii;
nid_type *list;
char *retStr;

    if (listStr){
        free(listStr);
	listStr = NULL;
    }

    retStr = listStr = (char *)malloc(800);
    list = (nid_type *)malloc(MAX_NODES * sizeof(nid_type));

    if (!listStr || !list){
       if (list) free(list); 
       if (listStr) free(listStr); 
       listStr = NULL;
       return badList;
    }

    for (i=0, ii=0; i<MAX_NODES; i++){

        if (ilist[i] == 1){
	    list[ii++] = i;
	}
        
    }
    if (ii > 0){
        write_node_list(list, ii, listStr, 800);
    }
    else{
        retStr = emptyList;
    }

    free(list);

    return retStr;
}




/*
** start a subprocess to get information from PBS, but kill it after
** timeout seconds if it's not done
*/

int
qmgr_q_query(char **rstring, time_t timeout, char *queue_name) {

int BUFSIZE=1028;
char buf[BUFSIZE];
char *buf2, *tmp, *tmp2;
char runqmgr[128];
int status, rc, n, bsize, size, fd[2];
pid_t pid;
time_t t1;
FILE *fp;


    status=1;

    sprintf(&runqmgr[0],"%s/bin/qmgr -c \"l q %s\"",pbs_prefix(),queue_name);

    if (pipe(fd) < 0) {
        log_msg("can't open pipe for qmgr check");
        status = -1;
        return status;
    }

    pid = fork();

    if (pid == -1){
        log_msg("can't fork subprocess for qmgr command");
        status = -1;
        return status;
    }
    if (pid == 0 ) { /* forked process to do command */

        close(fd[0]); /* Close pipe for reading, we won't use it. */

        status = 0;

        fp = popen(runqmgr,"r");


        while (fgets(buf, BUFSIZE, fp) !=NULL) {
            write(fd[1],buf,strlen(buf));
        }
        close(fd[1]); /* Done writing to pipe */
        pclose(fp);

        exit(0);
    }
    else { /* we're still parent process */

        t1 = time(NULL);

        bsize=BUFSIZE * sizeof(char);
        n=0;
        size=0;

        buf2 = malloc(bsize);
        tmp = buf2;

        close(fd[1]); /* We won't be writing */

        while (1) {

            rc = waitpid(pid,NULL,WNOHANG);
            if ( rc > 0 ) {
                status = 1;
                break;
            }
            if (rc < 0) {
                log_warning("waiting for completion of queue query");
                status = -1;
                break;
            }
            if ((time(NULL) -t1) > timeout){

                if (status == -1) break; /* already killed it */

                kill(pid,SIGKILL); /* timed out waiting */
                status = -1;
                t1 = time(NULL); /* now wait for killed process */
                timeout = 4;
            }
        }

        if (status) {
            while ((n = read(fd[0],buf,BUFSIZ)) != 0) {
                size += n;

                if (size < bsize) { /* If we don't have enough memory */
                                    /* allocated for what we've read */
                    bsize+=BUFSIZE*sizeof(char);
                    tmp2 = malloc(bsize);
                    memcpy(tmp2, buf2, size-n);
                    tmp=tmp2 + size - n;
                    free(buf2);
                    buf2=tmp2;
                }
                memcpy(tmp,buf,n);
                tmp+=n;
            }
            *rstring =  buf2;
        }
        status = size+1;
        close(fd[0]);
    }


    return status;
}

/*
** start a subprocess to execute the command, but kill it
** after timeout seconds if it's not done
*/

static int
qmgr_command(char *cmd, time_t timeout) {

char runqmgr[128];
int status, rc;
pid_t pid;
time_t t1;
FILE *fp;

    status = 0;

    sprintf(&runqmgr[0],"%s/bin/qmgr",pbs_prefix());

    pid = fork();

    if (pid == -1){
        log_msg("can't fork subprocess for qmgr command");
        status = -1;
    }
    else if (pid == 0){  /* do command, but don't wait forever */

        ename = pbsUpdateProc;

        fp = popen(runqmgr, "w");

        rc = fwrite(cmd, strlen(cmd), 1, fp);

        pclose(fp);

        exit(0);

    }
    else{

        t1 = time(NULL);

        while (1){
            rc = waitpid(pid, NULL, WNOHANG);

            if (rc == pid){  /* command is done */
                break;
            }

            if (rc < 0){
               log_warning("waiting for completion of %s command",cmd);
               status = -1;
               break;
            }


            if ((time(NULL) - t1) > timeout){

               if (status == -1) break; /* already killed it */

               kill(pid, SIGKILL);  /* timed out waiting */
               status = -1;
               t1 = time(NULL);   /* now wait for killed process */
               timeout = 4;
            }
        }

    }
    return status;
}

void
update_PBS_server()
{
char qmgrcmd[128];
int rc;

    /*
    ** Update the PBS server with number of live compute nodes under
    ** PBS management.
    */

    /*
    ** resources_available: The number of nodes either running PBS
    **                      jobs or available to run PBS jobs.
    */
    sprintf(&qmgrcmd[0],"s s resources_available.size=%d\n",activeNodes);

    rc = qmgr_command(qmgrcmd, 5);

    if (Dbglevel){
	log_msg("%s",qmgrcmd);
    }
    if (rc){
        log_msg("Error issuing qmgr command.\n");
        return; 
    }

    /*
    ** resources_max: The bound on the number of nodes that users
    **                can request.
    */
    sprintf(&qmgrcmd[0],"s s resources_max.size=%d\n",activeNodes);

    rc = qmgr_command(qmgrcmd, 5);

    if (Dbglevel){
	log_msg("%s",qmgrcmd);
    }
    if (rc){
        log_msg("Error issuing qmgr command.\n");
        return; 
    }

    reportedActiveNodes = activeNodes;

#ifdef PBS_UNUSED_RESOURCE
    /*
    ** resources_available.unused: The number of active nodes that
    **       are not running regular jobs.  cycle-stealers can ask 
    **       for this resource.
    */
    sprintf(&qmgrcmd[0],"s s resources_available.unused=%d\n",unusedNodes);

    rc = qmgr_command(qmgrcmd, 5);

    if (Dbglevel){
        log_msg("%s",qmgrcmd);
    }
    if (rc){
        log_msg("Error issuing qmgr command.\n");
        return; 
    }
#endif

    reportedUnusedNodes = unusedNodes;
}

static void
nice_kill_node(int nid)
{
pingPct_req req;

    req.nid1 = minPct;
    req.nid2 = maxPct;
    req.euid = -1;
    req.jobID = INVAL;
    req.sessionID = INVAL;

    srvr_send_to_control_ptl(nid, PPID_PCT,
        PCT_STATUS_ACTION_PORTAL,  PCT_NICE_KILL_JOB_REQUEST,
        (char *)&req, sizeof(pingPct_req));
}

static void
reset_node(int nid)
{
pingPct_req req;

    req.nid1 = minPct;
    req.nid2 = maxPct;
    req.euid = -1;
    req.jobID = INVAL;
    req.sessionID = INVAL;

    srvr_send_to_control_ptl(nid, PPID_PCT,
	PCT_STATUS_ACTION_PORTAL, PCT_RESET_REQUEST,
	 (char *)&req, sizeof(pingPct_req));
}

#ifndef STRIPDOWN
void
compute_active_nodes()
{
    /*
    ** Pool of nodes on which PBS can run low priority
    ** scavenger jobs.  Don't include the nodes on which
    ** low priority jobs are dying.  They are dying because
    ** regular priority jobs want their nodes.
    */
    unusedNodes = nodeStateTotals[pbsFree] + 
                  nodeStateTotals[pbsLowRunning];

    /*
    ** Pool of nodes on which PBS can schedule regular
    ** priority PBS jobs.
    */
    activeNodes = unusedNodes +
		    nodeStateTotals[pbsPending] +
		    nodeStateTotals[pbsRegular] +
		    nodeStateTotals[pbsDying] +
		    nodeStateTotals[pbsDyingPending];

}
#else /* STRIPDOWN */
void
compute_active_nodes()
{
/*
** Compute the number of nodes that PBS should have in its
** resources.available.size
*/
int i,j;
INT16 status;

activeNodes=0;
/* NOTE TO SELF - Redefine ACTIVE_NODE in head once REMAP and 
 * STRIPDOWN are all pulled out. */
#ifdef REMAP
    if (Remap) {
      for (i=pbsMIN; i <= pbsMAX; i++) {
        j=carray[i];
        status=larray[j].status;
        if (((status == STATUS_FREE) || (status == STATUS_ALLOCATED) ||
            (status == STATUS_BUSY)) && (larray[j].nodeType!=IACTIVE_NODE))
        {
          activeNodes++;
        }
      } 
    }
    else {
#endif
      for (i=pbsMIN; i <= pbsMAX; i++) {
        status=larray[i].status;
        if (((status == STATUS_ALLOCATED) || (status == STATUS_FREE) ||
            (status == STATUS_BUSY)) && (larray[i].nodeType!=IACTIVE_NODE))
        { 
          activeNodes++;
        }
      }
#ifdef REMAP
    }
#endif
} 
#endif /* STRIPDOWN */

/*
** need an ordered list of jobs PBS has allocated nodes to, and jobs that are
** actually running, so we can look for rogues
*/

static int hopcount, sanity_failed;

#define INIT_SANITY   hopcount = 0 ; sanity_failed = 0;

#define DO_SANITY(s)             \
   if (hopcount++ > 10000){      \
       log_msg(s);               \
       sanity_failed=1;          \
       break;                    \
   }

struct _JobIds{
   int id;
   short *nidList;
   int nnids;
   struct _JobIds *next;
};

static struct _JobIds *running=NULL;
static struct _JobIds *allocated=NULL;
static struct _JobIds *rogue=NULL;


static void
addNid(struct _JobIds *jid, int nid)
{
    if (jid->nidList == NULL){
        jid->nidList = (short *)malloc(MAX_NODES * sizeof(short));
        jid->nidList[0] = nid;
        jid->nnids      = 1;
        return;
    }

    if (jid->nnids == MAX_NODES) return;  /* error */

    /*
    ** doesn't check for duplicates - just adds the id 
    */

    jid->nidList[jid->nnids] = nid;
    jid->nnids++;

    return;
}
static struct _JobIds  *
newIdSlot()
{
struct _JobIds *new;

   new = (struct _JobIds *)malloc(sizeof (struct _JobIds));

   if (new){
       new->next = NULL;
       new->nidList = NULL;
       new->nnids = 0;
       new->id = -1;
   }

   return new;
}

static int
addJob(int id, struct _JobIds **top, int nid)
{
struct _JobIds *current, *prev, *new;

   if (! *top){
       *top = newIdSlot();

       if (! *top) return -1;

       (*top)->id = id;

       if (nid >= 0){
           addNid(*top, nid);
       }

       return 0;
   }

   prev = NULL;
   current = *top;

   INIT_SANITY;

   while (current && (id > current->id)){

      prev = current;
      current = prev->next;

      DO_SANITY("addJob");

   }
   if (sanity_failed) return -1;

   if (current && (current->id == id)){
       if (nid >= 0){
           addNid(current, nid);
       }
       return 0;
   }

   new = newIdSlot();

   new->id = id;

   if (nid >= 0){
       addNid(new, nid);
   }

   if (prev){
      prev->next = new;
   }
   else{
      *top = new;
   }

   if (current){
      new->next = current;
   }

   return 0;
}
static void
clearJobs(struct _JobIds *top)
{
struct _JobIds *current, *prev;

    current = top;

    INIT_SANITY;

    while (current){
        prev = current;
	current = prev->next;

        if (prev->nidList) free(prev->nidList);
	free(prev);

        DO_SANITY("clearJobs");
    }
}
static void
showJobs(struct _JobIds *current)
{
    INIT_SANITY;

    while (current){
        log_msg("    %d",current->id);
	current = current->next;

        DO_SANITY("showJobs");
    }
}
static int
invalidPBSJob(int id)
{
struct _JobIds *badjob;

    badjob = rogue;

    INIT_SANITY;

    while (badjob){
	if (badjob->id > id) break;
        if (badjob->id == id) return 1;
	badjob = badjob->next;

        DO_SANITY("invalidPBSJob");
    }

    if (sanity_failed) return -1;

    return 0;
}

/*
** invoke qstat to determine which PBS jobs are legit
*/

int
validPbsJobs()
{
int status, rc;
pid_t pid;
time_t t1;
FILE *fp;
char jobid[10], cmd[128];
int c, id, filedes[2], readfrom, writeto, nomore, i;

    status = 0;

    rc = pipe(filedes);

    if (rc){
        perror("can't create pipe");
	exit(0);
    }

    readfrom = filedes[0];
    writeto = filedes[1];

    nomore = -1;

    pid = fork();

    if (pid == -1){
	printf("can't fork subprocess for qstat command");
	status = -1;
    }
    else if (pid == 0){  /* run qstat and send job IDs back to parent */

        close(readfrom);

        sprintf(cmd,"%s/bin/qstat",pbs_prefix());

	fp = popen(cmd, "r");

	if (!fp){
	     log_warning("validPbsJobs: can't run qstat command");
	     exit(0);
	}

	while ((c = fgetc(fp)) != EOF){

            /*
	    ** job ID is first thing on the line 
	    */
	    for (i=0; isdigit(c) ; i++){
		jobid[i] = (char)c;
		c = fgetc(fp);

		if (i==9){
		    log_msg("validPbsJobs: bogus qstat output\n");
		    exit(0);
		}
	    }
	    if (i > 0){
		jobid[i] = 0;

		id = atoi(jobid);

		rc = write(writeto, &id, sizeof(int));

		if (rc != sizeof(int)){
		    log_warning("validPbsJobs: writing back");
		    exit(0);
		}
	    }
	    /*
	    ** go to the end of the line
	    */
	    while ( ((c = fgetc(fp)) != EOF) && (c != '\n') );

	    if (c == EOF) break;
	}
	write(writeto, &nomore, sizeof(int));

	pclose(fp);
	close(writeto);
	exit(0);
    }
    else{

	t1 = time(NULL);

	close(writeto);

	rc = fcntl(readfrom, F_SETFL, O_NONBLOCK);

	if (rc){
	    log_warning("validPbsJobs: making non-blocking pipe");
	    return -1;
	}

	while (1){

	    if ((time(NULL) - t1) > 10.0){
	        log_msg("validPbsJobs: Have to kill qstat process - timeout\n");
		status = -1;
                break;	
	    }

	    rc = read(readfrom, &id, sizeof(int));

	    if ((rc == -1) && (errno == EAGAIN)){
	        continue;
            }

	    if (rc == -1){
	        log_warning("validPbsJobs: can't read from pipe");
		status = -1;
		break;
	    }
	    else if (rc < sizeof(int)){
	        log_msg("validPbsJobs: got less than an int! %d\n",rc );
		status = -1;
		break;
	    }
	    
	    if (id == -1){ /* done */
		break;
	    }
	    else{
	        addJob(id, &allocated, -1);
	    }
	}
    }
    kill(pid, SIGKILL);

    t1 = time(NULL);

    while ( (time(NULL) - t1) < 10.0){
        rc = waitpid(pid, NULL, WNOHANG);

	if (rc == pid) break;

	if (rc < 0){
	    log_warning("validPbsJobs: waiting for qstat process");
	    break;
        }
    }
    return status;
}

/*
** Returns 0 if new nodes were freed by killing off rogue PBS jobs,
**  returns -1 otherwise.
*/
int
recover_free_pbs_nodes()
{
int i, rc, newNodes;
struct _JobIds *rptr, *aptr;

    newNodes = 0;

    if (Dbglevel){
        log_msg("Attempting to recover nodes used by rogue PBS jobs");
    }

    /*
    ** create a list of PBS jobs that are using compute nodes
    */
    clearJobs(running);
    running = NULL;

    for (i=minPct; i<=maxPct; i++){

        if (larray[i].status == STATUS_NO_STATUS) continue;

        if ((larray[i].status == STATUS_BUSY) &&
	    (larray[i].session_id != INVAL)              ){

	         addJob(larray[i].session_id, &running, -1); 
	}
    }
    if (Dbglevel){
        log_msg("List of PBS jobs currently using compute nodes:");
	showJobs(running);
    }

    /*
    ** invoke qstat to learn which jobs are legit
    */
    clearJobs(allocated);
    allocated = NULL;

    rc = validPbsJobs();

    if (rc){
       return -1;
    }

    if (Dbglevel){
        log_msg("List of jobs in any state currently known to PBS:");
	showJobs(allocated);
    }

    /*
    ** Find running jobs that should not be out there
    */

    clearJobs(rogue);
    rogue = NULL;

    rptr = running;
    aptr = allocated;

    INIT_SANITY;

    while (rptr && aptr){

        while (aptr->id < rptr->id){ 
	    /* 
	    ** some legit jobs may not be running right now 
	    */
	    aptr = aptr->next;
	    continue;
	}

        if (aptr->id == rptr->id){

	    rptr = rptr->next;
	    aptr = aptr->next;
	    continue;
	}
	/*
	** this running job is not on PBS' list of jobs
	*/
	addJob(rptr->id, &rogue, -1);

	rptr = rptr->next;

	DO_SANITY("recover_free_pbs_nodes");
    }
    if (sanity_failed){
        return -1;
    }

    /*
    ** OK, if there were invalid PBS jobs kill them off
    */
    if (rogue){

	log_msg("List of rogue PBS jobs:");
	showJobs(rogue);
	log_msg("We'll send RESET requests to PCTs to kill them");

	for (i=minPct; i<=maxPct; i++){

            if (larray[i].status == STATUS_NO_STATUS) continue;

	    if ((larray[i].status == STATUS_BUSY) &&
	        (larray[i].session_id != INVAL) &&
		(invalidPBSJob(larray[i].session_id)) ){

		reset_node(i);

		 if (Dbglevel){
                     log_msg("Reset request sent to node %d for PBS job %d",
		                        i, larray[i].session_id);
		 }
            }
	}

	return 0;
    }
    else{
        return 1;
    }
}

/*
** Some PBS jobs are low-priority cycle-stealing jobs.  They run on nodes allocated
** to regular PBS jobs while those regular jobs are not using them.  When the
** regular jobs wants nodes, it's bebopd's job to kill the scavengers.
**
** Can use this to kill off low priority interactive (non-PBS) jobs as well.
**
** We kill interactive scavengers for interactive jobs, and PBS
** scavengers for PBS jobs, otherwise our totals get all messed up.
*/

static struct _JobIds *scav=NULL;

int
clearScavengersForJob(int jobSize, int type)
{
int needNodes, gotNodes, i, nid, killable, havenodes;
struct _JobIds *killJob;

    if (jobSize <= 0) return -1;

    if (type == SCHEDULED){
	havenodes = nodeStateTotals[pbsFree] + nodeStateTotals[pbsDying];
        killable = nodeStateTotals[pbsLowRunning];
    }
    else if (type == INTERACTIVE){
        havenodes =  nodeStateTotals[interactiveFree] + nodeStateTotals[interactiveDying];
        killable = nodeStateTotals[interactiveLowRunning];
    }
    else{
        return -1;
    }

    needNodes = jobSize - havenodes;

    if (needNodes <= 0) return 0;

    if (needNodes > killable) return -1;

    /*
    ** create a list of all cycle-stealers we can kill, plus every node
    ** they run on
    */

    for (i=minPct; i<=maxPct; i++){

        if (larray[i].status == STATUS_NO_STATUS) continue;

        if ( ((type == INTERACTIVE) && (nodeState[i] == interactiveLowRunning)) ||

             ((type == SCHEDULED) && (nodeState[i] == pbsLowRunning)) ){

            addJob(larray[i].job_id, &scav, i);
        }
    }
    /*
    ** We'll kill off scavengers in job ID order.  Normally this means
    ** we kill off the longest running jobs first.  Occasionally job IDs
    ** wrap around to 0 and this will not be the case.  I don't think
    ** we care too much about that.  Cycle-stealers are not guaranteed
    ** any minimum slice of time or fairness.
    */

    gotNodes = 0;

    killJob = scav;

    while (gotNodes < needNodes){

        if (!killJob) break;

        for (i=0; i < killJob->nnids; i++){

            nid = killJob->nidList[i];

            nice_kill_node(nid);

	    larray[nid].user_status &= NICE_KILL_JOB_STARTED;
	    larray[nid].niceKillCountdown = niceKill;

        }
        gotNodes += killJob->nnids;

	log_msg(
	"ClearScavengersForJob: nice-killed %s scavenger job %d for regular job\n",
                     TYPE_STRING(type), killJob->id);	

        killJob = killJob->next;
    }

    clearJobs(scav);
    scav = NULL;

    if (gotNodes >= needNodes){
        return 0;
    }
    else{
        return -1;
    }
}
/*
** We can have interactive jobs running on PBS nodes (for example if
** PBS is started on a machine already running jobs) or PBS jobs running
** on interactive nodes (if the interactive partition is set or changed
** after PBS jobs have started.)
**
** Normally we let an outlier finish before reclaiming the nodes, but 
** if the job is a scavenger, let's kill it off.  They won't get killed
** in the normal course of events since we only kill PBS scavengers
** to run a PBS job, and interactive scavengers to run an interactive job.
**
** type == SCHEDULED    kill low priority PBS outliers
** type == INTERACTIVE  kill low priority interactive outliers
*/

#define SCAVENGER_JOB(type, nid)         \
  ( (larray[nid].status == STATUS_BUSY) && \
    (larray[nid].priority == SCAVENGER) && \
    ( ((type == INTERACTIVE) && (larray[nid].session_id == INVAL)) || \
      ((type == SCHEDULED) && (larray[nid].session_id != INVAL))  )  )

void
clearOutlierScavengers(int type)
{
int i, nid, killit;
struct _JobIds *killJob;
pingPct_req req;

    for (i=minPct; i<=maxPct; i++){

        if (larray[i].status == STATUS_NO_STATUS) continue;

        if ( SCAVENGER_JOB(type, i)){
            addJob(larray[i].job_id, &scav, i);
        }
    }

    if (scav == NULL) return;

    req.nid1=minPct;
    req.nid2=maxPct;
    req.euid = -1;
    req.sessionID = INVAL;
    req.jobID = INVAL;

    killJob = scav;

    while (killJob){

         killit = 0;

         for (i = 0; i < killJob->nnids; i++){

	     nid = killJob->nidList[i];

	     if ( ((type == INTERACTIVE) && (nodeState[nid] == interactiveOutlier)) ||
	          ((type == SCHEDULED) && (nodeState[nid] == pbsOutlier)) ){

                 if (!(larray[nid].user_status & NICE_KILL_JOB_STARTED)){

		     killit = 1;

		     log_msg(
			"clearOutlierScavengers: low priority %s job %d is running outside %s partition\n",
			TYPE_STRING(type), killJob->id, TYPE_STRING(type));

		     break;
		 }
             }
         }

	 if (killit){

	     for (i=0; i < killJob->nnids; i++){

	 	 nid = killJob->nidList[i];

		 nice_kill_node(nid);
	     }
	     log_msg(
	     "clearOutlierScavengers: Nice-kill request sent to job %d",
				killJob->id);
	 }

	 killJob = killJob->next;
    }

    clearJobs(scav);
    scav = NULL;

    return;
}
/*
** start a subprocess to get information back from pbs **
*/

int
query_pbs(char **rstring, time_t timeout, char *cmd) {

int BUFSIZE=1028;
char buf[BUFSIZE];
char *buf2, *tmp, *tmp2;
char runqmgr[128];

int status, rc, n, bsize, size, fd[2];
pid_t pid;
time_t t1;
FILE *fp;

    status=1;

    sprintf(&runqmgr[0],"%s/bin/%s",pbs_prefix(),cmd);

    if (pipe(fd) < 0) {
        log_msg("can't open pipe for qmgr check");
        status = -1;
        return status;
    }

    pid = fork();

    if (pid == -1){
        log_msg("can't fork subprocess for qmgr command");
        status = -1;
        return status;
    }

    if (pid == 0 ) { /* forked process to do command */

        close(fd[0]); /* Close pipe for reading, we won't use it. */
        status = 0;

        fp = popen(cmd, "r");
        fp = popen(runqmgr,"r");


        while (fgets(buf, BUFSIZE, fp) !=NULL) {
            write(fd[1],buf,strlen(buf));
        }
        close(fd[1]); /* Done writing to pipe */
        pclose(fp);

        exit(0);
    }
    else { /* we're still parent process */

        t1 = time(NULL);

        bsize=BUFSIZE * sizeof(char);
        n=0;
        size=0;

        buf2 = malloc(bsize);
        tmp = buf2;

        close(fd[1]); /* We won't be writing */

        while (1) {

            rc = waitpid(pid,NULL,WNOHANG);
            if ( rc > 0 ) {
                status = 1;
                break;
            }
            if (rc < 0) {
                log_warning("waiting for completion of queue query");
                log_warning("waiting for completion of queue query");
                status = -1;
                break;
            }
            if ((time(NULL) -t1) > timeout){

                if (status == -1) break; /* already killed it */

                kill(pid,SIGKILL); /* timed out waiting */
                status = -1;
                t1 = time(NULL); /* now wait for killed process */
                timeout = 4;
            }
        }
        if (status) {
            while ((n = read(fd[0],buf,BUFSIZ)) != 0) {
                size += n;
                if (size < bsize) { /* If we don't have enough memory */
                                    /* allocated for what we've read */
                    bsize+=BUFSIZE*sizeof(char);
                    tmp2 = malloc(bsize);
                    memcpy(tmp2, buf2, size-n);
                    tmp=tmp2 + size - n;
                    free(buf2);
                    buf2=tmp2;
                }
                memcpy(tmp,buf,n);
                tmp+=n;
            }
            *rstring =  buf2;
        }
        status = size+1;
        close(fd[0]);
    }
    return status;
}


