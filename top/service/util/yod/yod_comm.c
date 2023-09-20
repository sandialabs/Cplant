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
/* $Id: yod_comm.c,v 1.71.2.1 2002/03/21 18:42:17 jsotto Exp $ */

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <error.h>
#include "sys_limits.h"
#include "bebopd.h"
#include "srvr_comm.h"
#include "srvr_err.h"
#include "host_msg.h"
#include "yod_comm.h"
#include "util.h"
#include "appload_msg.h"
#include "config.h"
#include "pct_ports.h"
#include "yod_data.h"
#include "ppid.h"
#include "portal_assignments.h"
#include "sys/defines.h"

#include "cplant_host.h"
 
extern int sverrno;
extern int altBpid, altBnid;
extern char* _cplant_link_version;
extern int show_link_versions;

extern double dclock(void);

static int await_bebopd_get(char *buf, int len, int type);
static int await_bebopd_reply(int *job_id);
static void display_bebopd_error(int errorCode);

static int ptl_pct = SRVR_INVAL_PTL,
           ptl_bebopd = SRVR_INVAL_PTL,
           ptl_app_procs = SRVR_INVAL_PTL;

#define MAX_BT_SIZE   1024*1024

static int *thePcts;

static int *pctpid, *pctptl;

static int bebopd_nid;
static ppid_type bebopd_pid;
static int bebopd_ptl;

static yod_request *reqList;  /* members of a compound request     */
static char **nodeListString; /* node lists to be pulled by bebopd */

static void
display_node_spec(int type, nodeSpec *spec, int listnum)
{
    switch (type){
        case YOD_NODE_REQ_ANY:
            yodmsg(" anywhere you like\n");
            break;
        case YOD_NODE_REQ_LIST:
            yodmsg(" out of the %d nodes in %s\n",
                spec->list.listsize, nodeListString[listnum]);
                break;
        case YOD_NODE_REQ_RANGE:
            yodmsg(" from the range %d through %d\n",
            spec->range.from_node,spec->range.to_node);
                break;
            default:
            yodmsg(" ??????\n");
    }
}
static void
display_node_allocation_request(yod_request *req, int type)
{
int i, nreq;

    yodmsg("=============================================\n");
    yodmsg("Node allocation request: %d total nodes\n", req->nnodes);

    if (type == YOD_NODE_REQ_COMPOUND){
        nreq = req->spec.req.numRequests;

        for (i=0; i<nreq; i++){
            yodmsg("  %d nodes", reqList[i].nnodes);
            display_node_spec(reqList[i].specType, &(reqList[i].spec), i);
            yodmsg("\n");
        }
    }
    else{
        display_node_spec(type, &(req->spec), 0);
    }
    yodmsg("=============================================\n");
}

#define COMMA   44
#define DOT     46

static void
node_spec_type(char *nodelist, int listsize, int *spec_type, nodeSpec *spec)
{
char *c1, *c2;
char nodenum[10];

    *spec_type = YOD_NODE_REQ_ANY;

    if (listsize){

        if (strchr(nodelist, COMMA)){
            *spec_type = YOD_NODE_REQ_LIST;
            spec->list.listsize = listsize;
        }
        else{
            *spec_type = YOD_NODE_REQ_RANGE;
            if (strchr(nodelist, DOT)){

                c1 = nodelist; c2 = nodenum;
                while (isdigit(*c1)) *c2++ = *c1++;
                *c2 = 0;

                spec->range.from_node = atoi(nodenum);
                while (!isdigit(*c1)) c1++;
                spec->range.to_node   = atoi(c1);
            }
            else{
                spec->range.from_node = spec->range.to_node = atoi(nodelist);
            }
        }
    }
}
INT32
initialize_yod(int global_listsize, char *global_nodelist, 
	       int session_id, int session_limit,
	       int *pctList, uid_t euid)
{
int rc, i, compoundReq, job_id, nmembers;
int member, nnodes, next, listsize, nlistsmax;
yod_request req;
int *nodeList;
loadMembers *mbr, *mbr2;
nodeSpec global_spec;
int global_spec_type;
int list;
int count;

    CLEAR_ERR;

    job_id = INVAL;

    thePcts = pctList;

    nmembers = total_members();

    reqList = (yod_request *)malloc(nmembers * sizeof(yod_request));
    nodeListString = (char **)malloc(nmembers * sizeof(char *));
    nodeList = (int *)malloc(MAX_NODES * sizeof(int));

    if (!reqList || !nodeListString || !nodeList){
        CPerrno = ENOMEM;
	return job_id;
    }

    if (DBG_FLAGS(DBG_MEMORY)){
	yodmsg("memory: %p %u for bebopd requests\n",
                  reqList, nmembers * sizeof(yod_request));
	yodmsg("memory: %p %u for node list strings\n", 
              nodeListString, nmembers * sizeof(char *));
    }
   

    node_spec_type(global_nodelist, global_listsize, 
              &global_spec_type, &global_spec);
    
    for (i=0; i<nmembers; i++){
        nodeListString[i] = NULL;
    }
    /*
    ** yod/bebopd communication: we get pct IDs from bebopd.
    **
    ** We send one of these requests for nodes:
    **
    **   YOD_NODE_REQ_ANY - We request allocation of any collection
    **                      of [n] compute nodes.
    **
    **   YOD_NODE_REQ_RANGE - We request allocation of any collection
    **        of [n] compute nodes from the physical node number range 
    **        beginning with [from_node] and ending with [to_node].
    **
    **   YOD_NODE_REQ_LIST - We request allocation of any collection
    **        of [n] compute nodes from the [node_list] provided.
    **
    **   YOD_NODE_REQ_COMPOUND - We request simultaneous allocation of
    **        of [n1, n2, .., nk] compute nodes from a set of k
    **        specifications of type ANY, RANGE or LIST.  This is for
    **        the case of heterogeneous load, where the different
    **        executables may have different node type requirements.
    **
    **   The bebopd returns a list of PCTs that are reserved for us.
    **   The list retains the order of the specification passed in.
    **   (So if a node range of 100..98 was passed in, the pct list
    **   returned will list 100, 99, 98 in that order.)  Better
    **   hurry up because the PCTs won't wait forever.  They transition
    **   back to FREE if they don't hear from yod soon.
    */

    /*
    ** We need a control portal for the bebopd get and put
    ** messages.
    */
    ptl_bebopd = srvr_init_control_ptl(2);

    if (ptl_bebopd == SRVR_INVAL_PTL){
        return job_id;
    }
    if (DBG_FLAGS(DBG_COMM)){
	yodmsg("bebopd control portal: %d\n",ptl_bebopd);
    }
 
    req.nnodes       = NumProcs;
    req.session_id   = session_id;
    req.nnodes_limit = session_limit;
    req.priority     = priority;
    req.myptl        = ptl_bebopd;
    req.euid         = euid;

    /*
    ** Do we require a compound request?
    **
    ** If we have more than one member (executable), and at least one 
    ** has a node list and it does not agree with the node list of every 
    ** other member, then we need to send bebopd a compound request.
    */
    compoundReq = 0;

    if (nmembers > 1){
        for (i=0; i<nmembers; i++){
            mbr = member_data(i);
            if (!mbr){
                yodmsg("Internal error with member data\n");
                return job_id;
            }
            if (mbr->local_ndlist){
                if (i > 0){
                    compoundReq = 1;
                }
                else{
                    for (i=1; i<nmembers; i++){
                        mbr2 = member_data(i);

                        if (mbr2->local_ndlist == NULL){
                            compoundReq = 1;
                        }
                        else if (strcmp(mbr->local_ndlist, mbr2->local_ndlist)){
                            compoundReq = 1;
                        }
                        if (compoundReq) break;
                    }
                }
                break;
            }
        }
    }
    if (!compoundReq){ /* either there is no load file, or the load file */
                       /* has only one line, or all load file lines use  */
		       /* the same node specification                    */

        mbr = member_data(0);
        if (!mbr){
            yodmsg("Internal error with member data\n");
            return job_id;
        }

        if (mbr->local_ndlist){
            node_spec_type(mbr->local_ndlist, mbr->listsize,
                      &req.specType, &(req.spec));
                    
            if (req.specType == YOD_NODE_REQ_LIST)
                nodeListString[0] = mbr->local_ndlist;
        }
        else {
            req.specType = global_spec_type;

            if (req.specType != YOD_NODE_REQ_ANY){
            
                memcpy(&(req.spec), &(global_spec), sizeof(nodeSpec));

                if (req.specType == YOD_NODE_REQ_LIST)
                    nodeListString[0] = global_nodelist;
            }
        }
    }
    if (compoundReq){

        req.specType = YOD_NODE_REQ_COMPOUND;

        compoundReq = 0;

        for (member = 0; member < nmembers; member++){

            mbr = member_data(member);

            if (!mbr){
                yodmsg("Internal error with member data\n");
                return job_id;
            }
            nnodes = mbr->local_size;

            if (mbr->local_ndlist == NULL){

                /*
                ** Combine adjacent requests if their node specification
                ** is the same
                */
                for (next = member+1; next < nmembers; next++){

                    mbr = member_data(next);

                    if (mbr == NULL){
                        yodmsg("Internal error with member data\n");
                        return job_id;
                    }

                    if (mbr->local_ndlist == NULL){
                        nnodes += mbr->local_size;
                        member = next;
                    }
                    else{
                        break;
                    }
                }

                reqList[compoundReq].nnodes = nnodes;
                reqList[compoundReq].myptl = ptl_bebopd;

		reqList[compoundReq].specType = global_spec_type;

                if (global_spec_type != YOD_NODE_REQ_ANY){
                
                    memcpy(&(reqList[compoundReq].spec), &global_spec,
                                 sizeof(nodeSpec));

                    if (global_spec_type == YOD_NODE_REQ_LIST)
                        nodeListString[compoundReq] = global_nodelist;
                }
            }
            else{
                /*
                ** Combine adjacent requests if their node specification
                ** is the same
                */
                for (next = member+1; next < nmembers; next++){

                    mbr2 = member_data(next);

                    if (mbr2 == NULL){
                        yodmsg("Internal error with member data\n");
                        return job_id;
                    }

                    if (mbr2->local_ndlist &&
                        !strcmp(mbr->local_ndlist, mbr2->local_ndlist)) {

                        nnodes += mbr2->local_size;
                        member = next;
                    }
                    else{
                        break;
                    }
                }
                node_spec_type(mbr->local_ndlist, mbr->listsize, 
                   &(reqList[compoundReq].specType), &(reqList[compoundReq].spec));

                reqList[compoundReq].nnodes = nnodes;
                reqList[compoundReq].myptl = ptl_bebopd;

                if (reqList[compoundReq].specType == YOD_NODE_REQ_LIST){
                    nodeListString[compoundReq] = mbr->local_ndlist;
                }
            }
            compoundReq++;
        }

        req.spec.req.numRequests = compoundReq;
    }

    if (DBG_FLAGS(DBG_ALLOC)){
        display_node_allocation_request(&req, req.specType);
    }

    /*
    ** yod/pct communication: The pct sends user process
    ** pids and also sends failure/completion 
    ** messages so we need a control portal to check for these.
    */
    ptl_pct = srvr_init_control_ptl(NumProcs * 3);

    if (ptl_pct == SRVR_INVAL_PTL){
        return job_id;
    }
    if (DBG_FLAGS(DBG_COMM)){
	yodmsg("pct control portal: %d\n",ptl_pct);
    }	 

    /*
    ** yod/app communication: We receive IO commands from the
    ** application.  We need a control portal for these, and
    ** a data portal for bulk replies.  The data portal slots are
    ** opened over the work buffers.
    */
    ptl_app_procs = srvr_init_control_ptl(NumProcs * 3);

    if (ptl_app_procs == SRVR_INVAL_PTL){
        return job_id;
    }
    if (DBG_FLAGS(DBG_COMM)){
	yodmsg("application processes control portal: %d\n",ptl_app_procs);
    }	 
    rc = initialize_work_bufs();
 
    if (rc < 0){
        return job_id;
    }

    if (altBnid == SRVR_INVAL_NID){
	if ( cplantHost_getNid("bebopd", 1, &list, &count) != OK )
	{
	    yoderrmsg( "Can not get nid from cplant-host file\n");
	    return job_id;
	}
	
	if ( count != 1 )
	    yoderrmsg( "expected one bebopd entry; got %d\n", count);
    }

    bebopd_nid = ( (altBnid == SRVR_INVAL_NID) ? list : altBnid);
    bebopd_pid = ( (altBpid == SRVR_INVAL_PID) ? PPID_BEBOPD : altBpid);
    bebopd_ptl = REQUEST_PTL;

    /*
    ** OK - send request to bebopd, await PCT list
    */

    if (DBG_FLAGS(DBG_BEBOPD)){
	yodmsg("Send initial allocation request to bebopd %d/%d/%d\n",
		   bebopd_nid,bebopd_pid,bebopd_ptl);
    }
    rc = srvr_send_to_control_ptl(bebopd_nid, bebopd_pid, bebopd_ptl, 
		req.specType, (char *)&req, sizeof(yod_request));

    if (rc < 0){
        yodmsg("(%s) - sending request to bebopd\n",
                 CPstrerror(CPerrno));
        return job_id;
    }

    if (compoundReq > 0){
        /*
        ** bebopd will request the list of node allocation requests
        */
	
        rc = await_bebopd_get((char *)reqList, 
	    compoundReq * sizeof(yod_request), BEBOPD_GET_COMPOUND_REQ);

        if (rc < 0){                    /* internal error */
            return job_id;
        }
	else if (rc > 0){
	    display_bebopd_error(rc);   /* bebopd can't do it */
	    return job_id;
	}
    }

    if (compoundReq || (req.specType == YOD_NODE_REQ_LIST)){

        nlistsmax = (compoundReq ? compoundReq : 1);

        for (i=0; i<nlistsmax; i++){

	    if (nodeListString[i] == NULL) continue;

	    if (compoundReq){
		listsize = reqList[i].spec.list.listsize;
	    }
	    else{
		listsize = req.spec.list.listsize;
	    }

	    rc = parse_node_list(nodeListString[i], nodeList,
			      listsize, 0, MAX_NODES);

	    if (rc <= 0){
		yodmsg("Can't parse %s\n",nodeListString[i]);
		return job_id;
	    }

	    /*
	    ** bebopd will request the node list
	    */
	    if (DBG_FLAGS(DBG_ALLOC)){
		yodmsg("send node list %d size %d to bebopd %d/%d\n",
			  i,listsize,bebopd_nid,bebopd_pid);
	    }
	    if (sizeof(nid_type) != sizeof(int)){

	        nid_type *nlist;
		int nid;

                /*
		** code problem: someone invented "nid_type" for node
		** numbers and we weren't consistent about using it in the code.  
		** bebopd expects a list of nid_types, not a list of ints.
		*/
		nlist = (nid_type *)malloc(listsize * sizeof(nid_type));

		for (nid = 0; nid<listsize; nid++){
		     nlist[nid] = nodeList[nid];
		}

		rc = await_bebopd_get((char *)nlist,
			listsize * sizeof(nid_type), BEBOPD_GET_NODE_LIST);

                free(nlist);
	    }
	    else{

		rc = await_bebopd_get((char *)nodeList,
			listsize * sizeof(int), BEBOPD_GET_NODE_LIST);
	    }

	    if (rc < 0){
		return job_id;
	    }
	    else if (rc > 0){
		display_bebopd_error(rc);
		return job_id;
	    }
	}
    }

    rc = await_bebopd_reply(&job_id);

    if (rc < 0){     /* internal error */
	yodmsg("(%s) - awaiting pct list from bebopd\n",
		 CPstrerror(CPerrno));
	job_id = INVAL;
	return job_id;
    }
    else if (rc > 0){   /* bebopd error */

        display_bebopd_error(rc);
        job_id = INVAL;
	return job_id;
    }

    if (DBG_FLAGS(DBG_ALLOC)){
        yodmsg("Allocated pcts:\n");
        for (i=0; i<NumProcs; i++){
            yodmsg("pct (%d) %d\n",i,thePcts[i]);
        }
        yodmsg("\n");
    }

    for (i=0; i<MAX_NODES; i++){
        physnid2rank[i] = -1;
    }
    for (i=0; i<NumProcs; i++){
        physnid2rank[thePcts[i]] = i;
    }
    if (DBG_FLAGS(DBG_LOAD_2)){
        yodmsg("yod initialization and bebopd interchange completed\n");
    }

    pctpid = (int *)malloc(sizeof(int) * NumProcs);
    pctptl = (int *)malloc(sizeof(int) * NumProcs);

    if (!pctpid || !pctptl){
	yoderrmsg("Out of memory");
        job_id = INVAL;
    }
    else{
        for (i=0; i<NumProcs; i++){
            rank2physnid[i] = thePcts[i];
            pctpid[i] = PPID_PCT;
            pctptl[i] = PCT_LOAD_PORTAL;
        }
    }
    if (DBG_FLAGS(DBG_MEMORY)){
	yodmsg("memory: %p (%u) pid list\n",pctpid,sizeof(int)*NumProcs);
	yodmsg("memory: %p (%u) ptl list\n",pctptl,sizeof(int)*NumProcs);
    }

    free(reqList);
    free(nodeListString);
    free(nodeList);

    return job_id;
}
int
get_app_srvr_portal()
{
    return ptl_app_procs;
}
int
get_pct_portal()
{
    return ptl_pct;
}

/*
** returns:   -1  internal error
**             0   OK
**            >0  bebopd error
*/
static int
await_bebopd_reply(int *job_id)
{
control_msg_handle mhandle;
int comm_rc, rc, msg_type;
time_t t1;
int await_status;
bebopd_status *status;

    await_status = BEBOPD_OK;

    if (DBG_FLAGS(DBG_BEBOPD)){
	yodmsg("Await bebopd request for our node list request\n");
    }

    t1 = time(NULL);

    while (1){
        SRVR_CLEAR_HANDLE(mhandle);

        comm_rc = srvr_get_next_control_msg(ptl_bebopd, &mhandle, &msg_type, NULL, 
				  (char **)&(status));

        if (comm_rc == 1) break;

        if (comm_rc < 0){
            yodmsg("(%s) - getting bebopd notice of pct list\n",
                         CPstrerror(CPerrno));
            await_status = -1;
	    break;
        }
 
        if ((time(NULL) - t1) > daemonWaitLimit){
            yodmsg("No reponse with pct list from bebopd, sorry\n");
            await_status = -1;
	    break;
        }
    }
    if (comm_rc == 1){
	if (status->rc != BEBOPD_OK){   /* bebopd can't fulfill the request */
	    await_status = status->rc;
	}
	else if (msg_type != BEBOPD_PUT_PCT_LIST){
	    yodmsg("unexpected message arrived on bebopd ptl\n");
	    await_status = -1;
	}
    }

    if (await_status == BEBOPD_OK){

	*job_id = status->job_id;

	if (DBG_FLAGS(DBG_BEBOPD)){
	     yodmsg("Await list of %d pcts from bebopd\n",NumProcs);
	     yodmsg("mhandle %p, thePcts %p size %d\n",
		 &mhandle, (void *)&thePcts,NumProcs * (int)sizeof(int));
	}
	rc = srvr_comm_put_reply(&mhandle, (void *)thePcts, 
				    NumProcs * sizeof(int));
 
	if (rc < 0){
	    yodmsg("(%s) - receiving pct list from bebopd\n", 
			 CPstrerror(CPerrno));
	    srvr_free_control_msg(ptl_bebopd, &mhandle);
	    return -1;
	}
    }
    if (comm_rc == 1){
        srvr_free_control_msg(ptl_bebopd, &mhandle);
    }

    return await_status;
}
static void
display_bebopd_error(int errorCode)
{
    switch (errorCode){
	case BEBOPD_ERR_INVALID:
	    yoderrmsg(
	    "yod sent an invalid request to the node allocator.  This\n");
	    yoderrmsg(
	    "should never happen.  Please notify system administration.\n");
	    break;

	case BEBOPD_ERR_INTERNAL:
	    yoderrmsg(
	    "bebopd internal error, This should never happen.\n");
	    yoderrmsg(
	    "Please notify system administration.\n");
	    break;

	/*
	** bebopd running with PBS support
	*/
	case BEBOPD_ERR_SESSION_LIMIT:

	    yoderrmsg(
  "Unable to allocate these nodes to your job.  Allocating them to you would\n");
	    yoderrmsg(
	      "give you more total nodes than the limit allocated you by PBS.\n");
	    break;

	case BEBOPD_ERR_NO_INT_SUPPORT:

	    yoderrmsg(
	      "Unable to allocate nodes to your interactive job.  The compute\n");
	    yoderrmsg(
	      "partition is dedicated completely to PBS jobs at this point in time.\n");
	    break;

	case BEBOPD_ERR_INSUFF_INT_NODES:

	    yoderrmsg(
	      "Unable to allocate nodes to your job.  The compute partition\n");
	    yoderrmsg(
	      "is running PBS (scheduled) jobs and has insufficient nodes\n");
	    yoderrmsg(
	      "reserved for interactive (non-PBS) jobs at this time.\n");
	    yoderrmsg(
	      "Run \"pingd\" to learn about nodes reserved for interactive use.\n");
	    break;

	/*
	** bebopd is NOT running in PBS support mode
	*/
#ifdef STRIPDOWN
        case BEBOPD_ERR_CONTACT_NODES:
	    yoderrmsg("Unable to contact the requested nodes.\n");
            break;

        case BEBOPD_ERR_UNEXPECTED_NODE_STATE:
	    yoderrmsg("Node(s) in unexpected state(s).\n");
            break;
#endif
	case BEBOPD_ERR_FREE_NODES:

	    yoderrmsg("Unable to allocate the requested nodes.\n");
	    yoderrmsg("Insufficient number of free compute nodes.\n");

            /*
	    ** This situtation should not occur with a PBS job,
	    ** maybe need to try again.
	    */
	    if (pbs_job != NO_PBS_JOB) retryLoad = 1;
	    break;

	case BEBOPD_ERR_NO_PBS_SUPPORT:

	    yoderrmsg("Unable to allocate nodes to your PBS batch job.\n");
	    yoderrmsg("The node allocator is not supporting PBS scheduling at this time.\n");
	    yoderrmsg("Please contact the system adminstrators.\n");
	    break;

	default:
	    yoderrmsg("Unrecognized node allocator return code %d.\n",errorCode);
	    yoderrmsg(
	    "This should never happen.  Please notify system administration.\n");
	    break;
    }
}
/*
** Returns:   0 - OK
**           -1 - error occured in await_bebopd_get
**           >0 - bebopd can't allocate the nodes
**
** We have a buffer to send to bebopd.  We sent bebopd a control 
** message telling him to come get it at his leisure.  We want yod 
** waiting around for bebopd, not the other way around.  So this
** routine awaits the bebopd's get request and then replies with 
** the buffer.
*/
int
await_bebopd_get(char *buf, int len, int mtype)
{
int rc, comm_rc, msg_type, send_status;
control_msg_handle mhandle;
bebopd_status *status;
time_t t1;
 
    send_status = BEBOPD_OK;

    if (DBG_FLAGS(DBG_BEBOPD)){
        yodmsg("Await request from bebopd for buffer size (%x/%d\n", 
            mtype, len);
    }

    t1 = time(NULL);
 
    while (1){

        SRVR_CLEAR_HANDLE(mhandle);

        comm_rc = srvr_get_next_control_msg(ptl_bebopd, &mhandle, &msg_type, NULL, 
			          (char **)&(status));
 
	if (comm_rc == 1) break;   /* got a message */
 
        if (comm_rc < 0){
            yodmsg("(%s) - getting bebopd request for node list\n",
                         CPstrerror(CPerrno));
            send_status = -1;
	    break;
        }
 
        if ((time(NULL) - t1) > daemonWaitLimit){
            yodmsg("No reponse from bebopd, sorry\n");
            send_status = -1;
	    break;
        }
    }
    if (comm_rc == 1){
	if (status->rc |= BEBOPD_OK){
            send_status = status->rc;  /* bebopd can't allocate nodes */
	}
	else if (msg_type != mtype){
	    yodmsg("Unexpected message on bebopd portal\n");
	    send_status = -1;
	}
    }
    
    if (send_status == BEBOPD_OK){
	if (DBG_FLAGS(DBG_BEBOPD)){
	    yodmsg("Send to bebopd message type %x\n",mtype);
	}
	rc = srvr_comm_get_reply(&mhandle, (void *)buf, len);
     
	if (rc < 0){
	    yodmsg("(%s) - error sending to bebopd\n",
			     CPstrerror(CPerrno));
	    send_status = -1;
	}
    }
 
    if (comm_rc == 1){
        srvr_free_control_msg(ptl_bebopd, &mhandle);
    }
 
    return send_status;
}
/*
** send control message to bebopd initiating bebopd get request,
**  await the get request and send off the data
*/
int
send_to_bebopd(char *buf, int len,   /* buffer for bebopd to come get */
	       int sendtype,         /* msg type for send to bebopd   */
	       int gettype)          /* msg type for bebopd get request */
{
yod_request req;
int rc;

    if (bebopd_ptl == SRVR_INVAL_PTL){
	return -1;
    }

    memset(&req, 0, sizeof(yod_request));

    req.myptl = ptl_bebopd;
    req.nnodes = len;

    rc = srvr_send_to_control_ptl(bebopd_nid, bebopd_pid, bebopd_ptl,
		 sendtype, (char *)&req, sizeof(yod_request));

    if (rc < 0){
	yodmsg("(%s) - sending %d message to bebopd\n",
			CPstrerror(CPerrno), sendtype);
        return -1;
    }

    rc = await_bebopd_get(buf, len, gettype);

    if (rc < 0){
        return -1;	
    }

    return 0;
}

void
takedown_yod_portals()
{
    if (ptl_bebopd != SRVR_INVAL_PTL){
        srvr_release_control_ptl(ptl_bebopd);
        ptl_bebopd = SRVR_INVAL_PTL;
    }
    if (ptl_pct != SRVR_INVAL_PTL){
        srvr_release_control_ptl(ptl_pct);
        ptl_pct = SRVR_INVAL_PTL;
    }
    if (ptl_app_procs != SRVR_INVAL_PTL){
        srvr_release_control_ptl(ptl_app_procs);
        ptl_app_procs = SRVR_INVAL_PTL;
    }
    takedown_work_bufs();
}
/***********************************************************************
**  pct communications
***********************************************************************/
VOID
display_pct_list(void)
{
int ii, i, nmembers;
loadMembers *mbr;

    if (NumProcs == 0) return;

    yodmsg("\nAllocated nodes:\n");

    nmembers = total_members();

    phys2name(0);

    for (i=0; i<nmembers; i++){

       mbr = member_data(i);

       yodmsg("%s:\n",mbr->pname);

       for (ii=mbr->data.fromRank; ii<=mbr->data.toRank; ii++){
           yodmsg("  node ID %d, rank %d, ",
            thePcts[ii], ii);
           yodmsg("node name: "); 
           print_node_name(thePcts[ii], stdout);
           yodmsg("\n");
       }
    }
    yodmsg("\n");
}
/*
** Return number of messages sucessfully sent
*/
INT32
send_pcts_control_message(int msg_type, char *buf, int len, volatile int *count)
{
int i, rc;

    CLEAR_ERR;

    if (count){
        *count = 0;
    }

    for (i=0; i<NumProcs; i++){

	if (DBG_FLAGS(DBG_LOAD_1)){
	    yodmsg("    send control message type %x to PCT %d/%d/%d\n",
		      msg_type, thePcts[i], PPID_PCT, PCT_LOAD_PORTAL);
	}

        rc = srvr_send_to_control_ptl(thePcts[i], PPID_PCT, PCT_LOAD_PORTAL,
             msg_type, buf, len);

        if (rc){
            return -1;
        }
        if (count){
            (*count)++;
        }
    }
    return 0;
}
char *
get_stack_trace(int pctRank, int bt_size, int job_id, int tmout)
{
int rc, len;
char *buf;

    CLEAR_ERR;

    if (DBG_FLAGS(DBG_DBG)){
        yodmsg("get stack trace of %d bytes from pct rank %d\n",
		    bt_size, pctRank);
    }

    if (bt_size < 1) {
        CPerrno = EINVAL;
	return NULL;
    }
    if (bt_size > MAX_BT_SIZE){
	bt_size = MAX_BT_SIZE - 1;
    }
    len = bt_size + 1;

    buf = (char *)malloc(len);

    if (!buf){
        CPerrno = ENOMEM;
	return NULL;
    }
    if (DBG_FLAGS(DBG_MEMORY)){
	yodmsg("memory: %p (%u) for stack trace\n", buf, len);
    }

    rc = srvr_comm_get_req(buf, bt_size,
               MSG_GET_BT, (char *)&job_id, sizeof(int),
               thePcts[pctRank], PPID_PCT, PCT_LOAD_PORTAL,
               1,      /* blocking call */
               tmout);

    if (rc){
	free(buf);
	if (DBG_FLAGS(DBG_MEMORY)){
	    yodmsg("memory: %p stack trace buffer FREED\n",buf);
	}
	buf = NULL;
    }
    else{
	buf[bt_size] = 0;
    }
    return buf;
}
INT32
send_root_pct_get_message(INT32 msg_type, CHAR *user_data, INT32 user_data_len,
            CHAR *get_data, INT32 get_data_len, INT32 tmout)
{
int rc;

    CLEAR_ERR;

    if (DBG_FLAGS(DBG_LOAD_1)){
	yodmsg("    send get request type %x for %d bytes to PCT %d/%d/%d\n",
		  msg_type, get_data_len, 
		  thePcts[0], PPID_PCT, PCT_LOAD_PORTAL);
    }

    rc = srvr_comm_get_req(get_data, get_data_len, 
               msg_type, user_data, user_data_len,
               thePcts[0], PPID_PCT, PCT_LOAD_PORTAL,
               1,      /* blocking call */
               tmout);

    return rc;
}
/*
** We wait for the pcts to access the read memory buffer, 
** subject to a timeout in seconds.  If tmout is zero, wait forever.
** Return 0 if OK, -1 on error.
**
** Some messages are sent to all pcts, some are only sent to the root
** pct which fans out the data to the other pcts.  In heterogeneous
** load, the pcts form subgroups, each hosting a different executable.  
** Each subgroup has a root pct which can fan out data to it's subgroup.  
** So:
**
** member: which subgroup (member) we are sending to, or -1 for all
** rootPctOnly: if TRUE, send only to root pct of given member or
**              to root pct of all members
**              if FALSE, send to all pcts hosting given member or
**              all pcts of all members
*/
INT32
send_pcts_put_message(INT32 msg_type, CHAR *user_data, INT32 user_data_len,
            CHAR *put_data, INT32 put_data_len, INT32 tmout,
        BOOLEAN rootPctOnly, INT32 member)
{
int bufnum, myrc, mbr, nmembers, rank;
loadMembers *mbrData;
int ntargets;

    CLEAR_ERR;

    nmembers = total_members();  /* virtually always 1 */

    if (DBG_FLAGS(DBG_LOAD_1)){
        yodmsg("    send %s put request type %x, buffer %p, size %d\n",
          rootPctOnly ? "root pct" : "all pcts", msg_type,
                   put_data, put_data_len);
        if (nmembers > 1){
            if (member > -1) { yodmsg("    application member # %d\n",member);
            } else yodmsg("    all members of application\n");
        }
    }

    for (mbr = 0; mbr < nmembers; mbr++){ 

        if ( (member >= 0) && (member != mbr)){
            continue;
        }
        mbrData = member_data(mbr);

        if (mbrData == NULL){
            yoderrmsg("Error in internal member data structure\n");
            return -1;
        }
        rank = mbrData->data.fromRank;

        if (rootPctOnly){
            ntargets = 1;
        }
        else{
            ntargets = mbrData->local_size;
        }

        bufnum = srvr_comm_put_req(put_data, put_data_len, 
                             msg_type, 
                             user_data, user_data_len,
                             ntargets, 
                             rank2physnid+rank, pctpid+rank, pctptl+rank);
                               
	if (bufnum < 0){
	    myrc = -1;
            break;
	}

	/*
	** Test the pct_ptl here.  The pct's may not be ready to
	** load, and will send a message to that effect. (NOT_READY_MSG)
	**
	** So cancel the put and try again later.
	*/

	myrc = wait_pcts_put_msg(bufnum, tmout, ntargets);

	srvr_delete_buf(bufnum);

        if (myrc) break;
    }

    return myrc;
}

#include <sys/time.h>
#include <unistd.h>

INT32
wait_pcts_put_msg(INT32 bufnum, INT32 tmout, INT32 howmany)
{
int rc;
struct timeval tv;
long t1;

    if (tmout){
        gettimeofday(&tv, NULL);
        t1 = tv.tv_sec;
    }

    while (1){
        rc = srvr_test_read_buf(bufnum, howmany);
        if (rc < 0){
            return -1;
        }

        if (rc == 1) break;  /* all pcts have pulled data */

        if (tmout){ 

             gettimeofday(&tv, NULL);

             if ((tv.tv_sec - t1) > tmout){
                 yodmsg("wait_pcts_put_msg: yod timed out waiting for pcts (%d secs)\n", tmout);
                 return -1;
             }
        }
    }

    return 0;
}
INT32
await_pct_msg(INT32 *mtype, CHAR **user_data, INT32 *pctRank, INT32 tmout)
{
int rc, rank;
struct timeval tv;
long t1;

  if (tmout){
    gettimeofday(&tv, NULL);
    t1 = tv.tv_sec;
  }

  while (1){
    rc = get_pct_control_message(mtype, user_data, &rank);

    if (rc < 0){
      return -1;
    }

    if (rc == 1) break;  /* got it */ 

    if (tmout){ 

      gettimeofday(&tv, NULL);

      if ((tv.tv_sec - t1) > tmout){
        yodmsg("await_pct_msg: yod timed out (%d secs) waiting for pct control msg\n", tmout);
        return -1;
      }
    }
  }

  if (pctRank) {
    *pctRank = rank;
  }
  return 0;
}

static control_msg_handle pct_mh;

INT32
get_pct_control_message(INT32 *mtype, CHAR **user_data, INT32 *pctRank)
{
INT32 rc, src_nid, src_pid;

    if (SRVR_IS_VALID_HANDLE(pct_mh)){

        srvr_free_control_msg(ptl_pct, &pct_mh); /* free the previous one */
    }

    SRVR_CLEAR_HANDLE(pct_mh);

    rc = srvr_get_next_control_msg(ptl_pct, &pct_mh, mtype, NULL, user_data);

    if (rc == 1){

        src_nid = SRVR_HANDLE_NID(pct_mh);
        src_pid = SRVR_HANDLE_PID(pct_mh);

        if (DBG_FLAGS(DBG_LOAD_1)){
            yodmsg("Got message type (%s) from pct %d/%d\n",
             select_pct_to_yod_string(*mtype), src_nid, src_pid);
        }

        if ((src_nid < 0) || (src_nid >= MAX_NODES) ||
            (physnid2rank[src_nid] < 0)){

            yodmsg("Unexpected message type (%s) from %d/%d ",
             select_pct_to_yod_string(*mtype), src_nid, src_pid);
            yodmsg("on pct control portal\n");

            return -1;
        }
        if (pctRank) *pctRank = physnid2rank[src_nid];
    }

    return rc;    /* 1 - there's a message, 0 - no message */
}
/*
** Await a control message from each PCT, save the message type
** and user data.
*/

static char checkRank[MAX_PROC_PER_GROUP];

int
all_get_pct_control_message(int *mtypes, 
			    char *udataBufs, int udataBufLen, 
			    int timeout)
{
int i, rc, mtype, rank, ii;
time_t tstart;
char *udata;

    if (NumProcs < 1) return 0;

    tstart = time(NULL);
    memset(checkRank, 0, NumProcs);
    i = 0;

    while (i < NumProcs){

        rc = get_pct_control_message(&mtype, &udata, &rank);

        if (rc == 1){

            checkRank[rank] = 1;

            if (mtypes) mtypes[rank] = mtype;

            if (udataBufs){
                memcpy(udataBufs + (rank* udataBufLen), 
		         udata, udataBufLen);
            }

            i++;
            continue;
        }
        if (rc < 0){                      /* there's an error */
            yoderrmsg("Failure awaiting messages from the PCTs (%s)",CPstrerror(CPerrno));
	    return -1;
        }

        if (rc == 0){
            if ( (time(NULL) - tstart) > timeout){

                for (rank = 0, ii= 0; rank < NumProcs; rank++){
                    if (checkRank[rank] == 0){

                        if (ii == 0){
                            start_notify("yod load failure");
                        }

                        if (ii > 5){
                            sprintf(msgbuf,
                            "Remaining failure messages suppressed.\n");
                        }
                        else{
                            sprintf(msgbuf,
                            "awaiting messages from PCTs, no word from node %d, rank %d\n",
                            rank2physnid[rank],rank);
                        }

                        yoderrmsg("%s",msgbuf);
                        log_msg(msgbuf);
                        notify(msgbuf);

                        if (ii > 5){
                            break;
                        }
                        ii++;
                    }
                }
		if (ii) end_notify();
                retryLoad = 1;
                return -1;
            }
        }
    }
    return 0;
}

int
check_link_version(int member)
{
int i, status;
unsigned char* nextchar;
char my_link_no[100], exec_link_no[100];
BOOLEAN found_exec_link_version = FALSE;
loadMembers *mbr;
 
    status = 0;
 
    mbr = member_data(member);

    /* find my link version */
    nextchar = _cplant_link_version; 
    while ( *nextchar != ':' ) {
      nextchar++;
    }
    nextchar++;
    i = 0;
    while ( *nextchar != ':' ) {
      my_link_no[i++] = *nextchar++; 
    }
    my_link_no[i] = '\0';
    if (show_link_versions) {
      yodmsg("current link no. is %s\n", my_link_no);
    }

    /* locate executable's link version */
    nextchar = _cplant_link_version; 
    for (i=0; i<mbr->data.execlen; i++) {
      if (mbr->exec[i] == *nextchar) {
        nextchar++;
        if (*nextchar == ':') {
          found_exec_link_version = TRUE;
          break;
        }
      }
      else {
        nextchar = _cplant_link_version;
      }
    }

    /* get exec's link number */
    if ( found_exec_link_version == TRUE ) {

      /* first char after ":" */
      nextchar = &(mbr->exec[i+2]);
      i = 0;
      while ( *nextchar != ':' ) {
        exec_link_no[i++] = *nextchar++; 
      }
      exec_link_no[i] = '\0';
      if (show_link_versions) {
        yodmsg("executable link no. is %s\n", exec_link_no);
      }

      /* compare link numbers */
      if ( strcmp(my_link_no,exec_link_no) != 0 ) {
        yodmsg("--------------------------------------------------------\n");
        yodmsg("Link version numbers on %s do not match:\n", mbr->pname);
        yodmsg("current link version is %s\n", my_link_no);
        yodmsg("link version of executable is %s\n", exec_link_no);
        yodmsg("--------------------------------------------------------\n");
        status = -1;
      }
    }
    else {
      yodmsg("--------------------------------------------------------\n");
      yodmsg("Could not find link version no. in %s:\n", mbr->pname);
      yodmsg("you may need to relink your application.\n");
      yodmsg("--------------------------------------------------------\n");
      status = -1;
    }
    return status;
}
int
read_executable(int member, int timing_data)
{
double td;
int fd, rc, status, i, sameAs;
loadMembers *mbr, *mbr2;
 
    status = 0;
    sameAs = member;
 
    mbr = member_data(member);

    if (mbr->pnameSameAs > -1){

	sameAs = mbr->pnameSameAs;
        mbr = member_data(sameAs);
    }

    if (mbr->exec != NULL){
        return 0;    /* must have read it in already */
    }
 
    mbr->exec = (unsigned char *)malloc(mbr->data.execlen);

    if (mbr->exec == NULL){
        CPerrno = ENOMEM;
        return -1; 
    }

    if (DBG_FLAGS(DBG_MEMORY)){
	yodmsg("memory: 0x%p  (%u)  executable %s\n",
                   mbr->exec, mbr->data.execlen, mbr->pname);
    }

 
    if (timing_data){
        td = dclock();
    }
 
    fd = open(mbr->pname, O_RDONLY);
 
    if (fd < 0){
        yoderrmsg("Can't open %s to read\n",mbr->pname);
        status = -1;
    }
    else{
 
        /* read executable into yod memory */
        rc = read(fd, mbr->exec, mbr->data.execlen);
 
        if (rc < mbr->data.execlen){
            yoderrmsg("Can't read %s, rc %d\n",mbr->pname,rc);
            status = -1;
        }
        else{
            if (timing_data){
                yodmsg("YOD TIMING: Read in %s, %f\n",mbr->pname, dclock()-td);
            }
        }
        close(fd);
    }

    if (status == 0){   /* compute a check sum for PCT's verification */
        mbr->exec_check = (unsigned char)0;

	for (i=0; i<mbr->data.execlen; i++){
	     mbr->exec_check ^= mbr->exec[i];
	}
    }

    if (mbr->pnameCount > 1){
         for (i=sameAs+1; i<total_members(); i++){

	     mbr2 = member_data(i);

	     if (mbr2->pnameSameAs == sameAs){
	         mbr2->exec       = mbr->exec;
	         mbr2->exec_check = mbr->exec_check;
	     }
	 }
    }

    return status;
}
int
send_executable(int member, int job_ID)
{
int rc, status;
loadMembers *mbr, *sameAs;
sendExec msg;
 
    status = 0;

    mbr = member_data(member);

    if (mbr->pnameSameAs > -1){
        sameAs = member_data(mbr->pnameSameAs);
    }
    else{
        sameAs = mbr;
    }
    sameAs->pnameCount--;

    if (DBG_FLAGS(DBG_LOAD_1)){
      yodmsg("    send %s, length %d to root PCT\n",
			  mbr->pname, mbr->data.execlen);
    }

    msg.job_ID = job_ID;
    msg.cksum = mbr->exec_check;
 
    rc = send_pcts_put_message(MSG_PUT_EXEC, (CHAR *)&msg, sizeof(sendExec),
                  mbr->exec, mbr->data.execlen, daemonWaitLimit,
                  PUT_ROOT_PCT_ONLY, member);
 
    if (rc){
      yoderrmsg("Error sending %s to pcts\n",mbr->pname);
      status = -1;
    }

    if (sameAs->pnameCount == 0){
        free(mbr->exec);

        if (DBG_FLAGS(DBG_MEMORY)){
            yodmsg("memory: 0x%p (%s) FREED\n",mbr->exec,mbr->pname);
        }

    }

    return status;
}
/***********************************************************************
**  app communications
***********************************************************************/

INT32
get_app_control_message(control_msg_handle *mh)
{
INT32 rc;

    SRVR_CLEAR_HANDLE(*mh);

    rc = srvr_get_next_control_msg(ptl_app_procs, mh, NULL, NULL, NULL);

    return rc;    /* 1 - there's a message, 0 - no message */
}
void
free_app_control_message(control_msg_handle *mh)
{
    srvr_free_control_msg(ptl_app_procs, mh);
}
/***********************************************************************
**  notification to system administration
**
**   start_notify(what)    open pipe to mail program, subject is "what"
**   notify(line)          line to write
**   end_notify()          finish up the mail and send it off
***********************************************************************/
static FILE *notify_fp = NULL;

int
start_notify(const char *what)
{
const char *tostring;
char *cmd;
int len;

    tostring = notify_list();

    if (tostring == NULL){   /* we notify no one, not an error */
	return 0;
    }

    len = strlen(tostring) + strlen(what) + 30;

    cmd = malloc(len);

    if (!cmd){
	log_warning("can't malloc %d bytes in start_notify",len);
	return -1;
    }
    if (DBG_FLAGS(DBG_MEMORY)){
	yodmsg("memory: %p (%u) for start-notify command\n", cmd,len);
    }
    
    if (what){
        sprintf(cmd,"/bin/mail -s \"%s\" %s",what,tostring);
    }
    else{
        sprintf(cmd,"/bin/mail -s yodNews %s",tostring);
    }
    notify_fp = popen(cmd, "w");

    free(cmd);
    if (DBG_FLAGS(DBG_MEMORY)){
	yodmsg("memory: %p start notify cmd buffer FREED\n",cmd);
    }

    if (notify_fp == NULL){
	log_warning("can't popen command in start_notify");
	return -1;
    }

    return 0;
}
int
notify(char *line)
{
int rc;

    if (!notify_fp || !line){
	return 0;
    }

    rc = fprintf(notify_fp, "%s", line);

    if (rc < 0){
	log_warning("can't write to mail pipe in notify");
	if (notify_fp) pclose(notify_fp);
	notify_fp = NULL;
	return -1;
    }

    return 0;
}
int
end_notify()
{
char machineName[80];

    if (!notify_fp){
	return 0;
    }
    fprintf(notify_fp,
    "\n-------------------------------------------------------------\n");

    fprintf(notify_fp,
     "This notification was sent from a yod process.\n");
     
    cplantHost_hostname(machineName, 80);

    if (machineName[0]){
       fprintf(notify_fp,
       "To be removed from this list, contact the adminstrators of\n");
       fprintf(notify_fp, "%s.\n",machineName);
    }
    fprintf(notify_fp,
    "-------------------------------------------------------------\n");

    fprintf(notify_fp, "%c", 4);   /* End Of Text */

    pclose(notify_fp);
    notify_fp = NULL;

    return 0;
}

/*   The following Routine might really belong in some other
**  directory, but for now put it here rather than add another
**  library to YOD's searches.
*/

 
/*
 * Read FYOD config file.
 * No longer reads a fyod config file.  Instead it gets the nid
 * from the cplant-host file. The pid and portal are now hard-coded.
 * This function exists for parallel with SFYOD.
 *     This routine reads nids for up to 10 FYODs but takes the
 *     first.   Future enhancements might include:
 **        -Randomly picking from the list of 10.
 **        -Would actually have liked to give different compute
 **           nodes in the one job different FYOD addresses.
 **           That does not go easily through YOD --> PCT, etc.
 **        -Could do a Probe or Heartbeat check to see if the
 **           FYOD is up and running.
 */
INT32 fyod_read_configFile(int *nid)
{
#define EWARNING (2)
/*                   EWARNING is a wmd used parameter  */
    static char fname[] = "fyod_read_configFile";
    int list[10];
    int count;
    int rc;
 
    rc = cplantHost_getNid("fyod", 10, list, &count); 
    if ( rc != 0 ) {
        *nid = -1;
    	/* error(fname, EWARNING, "cplantHost_getNid() failed"); */
	error(1, 0, "%s: cplantHost_getNid() failed", fname); 
    }
    *nid = list[0];
 
    return 0;
}
