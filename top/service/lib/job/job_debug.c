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
** $Id: job_debug.c,v 1.2 2001/11/17 01:00:07 lafisk Exp $
**
** Functions to manage yod debugging output, to display
** internal structures, and to set/return library
** error string.
*/

#include <stdarg.h>
#include <string.h>

#include "pct_start.h"
#include "appload_msg.h"
#include "job_private.h"

char *depthmarker[10] =
{
"",         
"  ",
"    ", 
"      ",
"        ",
"          ",
"            ",
"              ",
"                ",
"                  "
};     

/*
** Status/debugging output  ****************************************
**
** Library functions should never just "printf", caller may not
** want library to output stuff.  Call _jobMsg or _jobErrorMsg
** instead.  (Exception - if a DBG_FLAG is turned on, the user
** obviously wants to see some output - printf is OK.)
*/
int Dbgflag = 0;
int JobQuiet = 0;
int prog_phase;    /* def'n required by yod codes we share */

void _jobMsg(char* format, ...)
{
va_list ap;

    if ( !jobOptions.quiet) {
      va_start(ap, format);
      if (jobOptions.myName) printf("%s: ", jobOptions.myName);
      vprintf(format, ap);
      va_end(ap);
    }
}

void _jobErrorMsg(char* format, ...)
{
va_list ap;

    if ( !jobOptions.quiet) {
      va_start(ap, format);
      if (jobOptions.myName) fprintf(stderr,"%s: ", jobOptions.myName);
      vfprintf(stderr, format, ap);
      va_end(ap);
    }
}

/*
** Job library's error string **************************************
**
** All library functions set the error string with
** _job_error_info() before returning -1.
*/

#define JOBSTACKDEPTH 1000
#define ERRSTRLEN 1024

static char _jobErrStr[ERRSTRLEN];

static char *job_stack[JOBSTACKDEPTH];
static int job_stack_ptr=JOBSTACKDEPTH;

void
_job_in(char *which)
{
    if ((job_stack_ptr <= 0)  || (job_stack_ptr > JOBSTACKDEPTH)){
        job_stack_ptr = JOBSTACKDEPTH;
    }

    /*
    ** error string starts afresh when library is entered
    */
    if (job_stack_ptr == JOBSTACKDEPTH) _jobErrStr[0] = 0;

    job_stack_ptr--;

    job_stack[job_stack_ptr] = which;
}
void
_job_out()
{
    job_stack_ptr++;
}

void
_job_error_info(const char *fmt, ...)
{
va_list ap;
int len, remaining;

    len = strlen(_jobErrStr);
    remaining = ERRSTRLEN - len - 1;

    if (remaining < 80){
        _jobErrStr[0] = 0;
        remaining = ERRSTRLEN - 1;
    }

    if (DBG_FLAGS(DBG_LOAD_1)){
        snprintf(_jobErrStr + len, remaining, "%s: ",job_stack_current());
        len = strlen(_jobErrStr);
        remaining = ERRSTRLEN - len - 1;
    }


    va_start(ap, fmt);
    vsnprintf(_jobErrStr+len, remaining, fmt, ap);
    va_end(ap);

}

/*
** Load progress reporting ****************************************
*/
int
_job_launch_failed(yod_job *yjob, launch_failure *lfail)
{
int signum, exitcode, rank, srcrank;

    _job_in("_job_launch_failed");

    if ((lfail->nid < 0) || 
        (lfail->nid >= MAX_NODES) ||
        ( (rank = _nid2rank(yjob, lfail->nid)) < 0)){

        _job_error_info("invalid node id in failure message");
        JOB_RETURN_ERROR;
    }

    yjob->failCount++;

    if (!yjob->fail_status){

        yjob->fail_status = 
        (launchErrors *)calloc(yjob->nnodes , sizeof(launchErrors));

        if (!yjob->fail_status){
           _job_error_info("out of memory");
            JOB_RETURN_ERROR;
        }
    }

    yjob->fail_status[rank].errType         = lfail->reason_code;
    yjob->fail_status[rank].reportingStatus = 0;

    if (lfail->reason_code == LAUNCH_ERR_PCT_GROUP){

        yjob->fail_status[rank].failedNid = lfail->type.peer.nid;
        yjob->fail_status[rank].failedPtl = lfail->type.peer.ptl;
        yjob->fail_status[rank].failedOp  = lfail->type.peer.op;
    }

    /*
    ** Normally report each type of error only once
    */
    if ((yjob->launch_err_types[lfail->reason_code] == 0) ||
         DBG_FLAGS(DBG_FAILURE) ||
        (lfail->reason_code == LAUNCH_ERR_CORRUPT_MSG) )
    {
        _jobErrorMsg(
         "Failure while hosting application process on node %d w/ rank %d.\n",
           lfail->nid, rank);

        _jobErrorMsg(
       "\tPCT report: %s\n",select_launch_err_string(lfail->reason_code));

        yjob->launch_err_types[lfail->reason_code]++;

        switch(lfail->reason_code){
            case LAUNCH_ERR_PCT_GROUP:
                if (yjob->fail_status[rank].failedNid >= 0){
                    _jobErrorMsg("\tTimed out waiting for node %d.\n",
                          yjob->fail_status[rank].failedNid);
                }
                break;

            case LAUNCH_ERR_EXEC:
                _jobErrorMsg("\tUnable to run your code on compute node.\n");
                _jobErrorMsg("\tPerhaps the executable file is corrupt?\n\n");
                break;

            case LAUNCH_ERR_CHILD_NACK:  /* child sent nack before main */
                if (lfail->type.child.Errno)
                  _jobErrorMsg("\tApp process low level error: %s\n",
                     CPstrerror(lfail->type.child.Errno));

                if (lfail->type.child.CPerrno)
                  _jobErrorMsg("\tApp process library level error: %s\n",
                     CPstrerror(lfail->type.child.CPerrno));

                _jobErrorMsg("\tApp startup routine status: %s\n",
                         start_error_string(lfail->type.child.reason_code));
                break;

            case LAUNCH_ERR_CHILD_EXIT: /* child exited before main */
                signum = lfail->type.child.reason_code >> 16;
                exitcode = lfail->type.child.reason_code  & 0x00ff;

                if (signum){
                    _jobErrorMsg("\tApp process terminating signal: %d, %s\n",
                                 signum, select_signal_name(signum));
                }
                _jobErrorMsg("\tApp process exit code: %d %s\n",
                         exitcode, start_error_string(exitcode));
                break;

            case LAUNCH_ERR_CORRUPT_MSG: /* bad hardware */

                srcrank = _nid2rank(yjob, lfail->type.peer.nid);

                _jobErrorMsg("\tReceived corrupt executable from node %d, rank %d\n",
                       lfail->type.peer.nid,  srcrank);

                if (yjob->launch_err_types[lfail->reason_code] == 1){
                    _jobErrorMsg(
"\t****************************************************************\n");
                    _jobErrorMsg(
"\tThis is a rare event.  It may mean a network card has bad memory\n");
                    _jobErrorMsg(
"\tor perhaps a network switch is malfunctioning.  Please notify\n");
                    _jobErrorMsg(
"\tsystem administration with the node ID of the problem nodes.\n");
                    _jobErrorMsg(
"\t****************************************************************\n");
                }

                break;

            default:
                break;
        }
    }
    JOB_RETURN_OK;
}


/*
** Display internal data structures ********************************
*/
static void displayNodeSpec(yod_request *req, char *str);
static void displayHostReply(hostReply_t *hr);
static void displayChars(unsigned char *c, int len, int width);
static void displayNidList(int *list, int size);
static void displaySpidList(spid_type *list, int size);
static void displayPpidList(ppid_type *list, int size);
static void displayNidRankList(nidOrder *list, int size);
static void displayYodRequest(yod_request *req);
static void displayMember(loadMbrs *mbr);

void
_displayNodeAllocationRequest(yod_job *yjob)
{
int i;
yod_request *req;

    _job_in("_displayNodeAllocationRequest");

    if (yjob->nrequests == 0){
        _jobMsg("Node allocation request: no request available\n");
        JOB_RETURN;
    }

    req = yjob->requests;

    _jobMsg("==================================================================\n");

    displayYodRequest(req);

    if (yjob->requests[0].specType == YOD_NODE_REQ_COMPOUND){

        _jobMsg(
	"==================================================================\n");

        for (i=1; i< yjob->nrequests; i++){

	    req = yjob->requests + i;

            displayYodRequest(req);
            displayNodeSpec(req, yjob->ndListStr[i]);
            _jobMsg("\n");
        }
    }
    else{
	displayNodeSpec(req, yjob->ndListStr[0]);
    }
    _jobMsg("==================================================================\n");

    JOB_RETURN;
}

void
_displayNodeList(yod_job *yjob)
{
int i;

    for (i=0; i<yjob->nnodes; i++){

        if (i && (i%10 == 0)) _jobMsg("\n");
        _jobMsg("%04d ",yjob->pctNidMap[i]);
    }
    _jobMsg("\n");
    
}
void
_displayJob(yod_job *job)
{
int i;

    _jobMsg("\n");

    _jobMsg("Global size %d,  list size %d,  list: %s\n",
          job->globalSize, job->globalListSize, job->globalListStr);

    if (job->globalList){
	displayNidList(job->globalList, job->globalListSize);
    }

    _jobMsg("\n");
    _jobMsg("Initial load message to go to PCTs\n");

    _displayInitialMsg(&(job->msg1));

    _jobMsg("\n");

    _jobMsg("Message fanned out to PCTs\n");

    _displayLoadMsg(&(job->msg2));

    if (job->straceMsg){
        _jobMsg("strace info: job id %d, dirlen %d, optlen %d, listlen %d\n",
	     job->straceMsg->job_ID, job->straceMsg->dirlen, job->straceMsg->optlen,
	     job->straceMsg->listlen);
    }
    else{
        _jobMsg("no strace info\n");
    }

    if (job->pidmap){
        _jobMsg("Application system pid map:\n");
	displaySpidList(job->pidmap, job->globalSize);
    }
    else{
        _jobMsg("no application system pid map\n");
    }

    if (job->ppidmap){
        _jobMsg("Application portal pid map:\n");
	displayPpidList(job->ppidmap, job->globalSize);
    }
    else{
        _jobMsg("no application portal pid map\n");
    }

    _jobMsg("\n");
    _jobMsg("Job ID: %d\n",job->job_id);
    _jobMsg("Number of spawned applications: %d\n",job->nkids);
    _jobMsg("My parent: %p\n",job->parent);

    _jobMsg("job status: 0x%04x\n",job->jobStatus);

    _jobMsg("Number of members: %d\n",job->nMembers);

    for (i=0; i<job->nMembers; i++){
         _jobMsg("\nMember %d\n",i);
         displayMember(job->Members + i);
    }

    _jobMsg("\n");
    _jobMsg("Number of bebopd load requests for this job: %d\n",job->nrequests);

    _displayNodeAllocationRequest(job);

    _jobMsg("bebopd reply: job id %d, rc %d\n",
               job->reply.job_id, job->reply.rc);

    _jobMsg("Nodes allocated: %d\n",job->nnodes);

    if (job->nnodes){
	displayNidList(job->pctNidMap, job->nnodes);

	_jobMsg("Phys nid order map with rank:\n");
	displayNidRankList(job->nidOrderMap, job->nnodes);
    }

}
void
_displayAllInfo()
{
int i; 

    _jobMsg("\n");
    _jobMsg("yod's environment:\n");

    _jobMsg("\n");
    if (_myEnv.env){
        _jobMsg("env length %d\n",_myEnv.envlen);
        displayChars(_myEnv.env, _myEnv.envlen, 60);
    }
    else{
        _jobMsg("env not set\n");
    }

    _jobMsg("\n");
    if (_myEnv.cwd){
        _jobMsg("cwd: %s\n",_myEnv.cwd);
    }
    else{
        _jobMsg("cwd not set\n");
    }

    _jobMsg("\n");
    _jobMsg("pbs job (NO_PBS_JOB 0, PBS_BATCH 1, PBS_INTERACTIVE 2): %d\n",_myEnv.pbs_job);
    _jobMsg("session id %d\n",_myEnv.session_id);
    _jobMsg("nodes allocated %d\n",_myEnv.session_nodes);
    _jobMsg("session priority %s\n",
      (_myEnv.session_priority == SCAVENGER ? "SCAVENGER" : "REGULAR JOB"));

    _jobMsg("\n");
    _jobMsg("uid %d, euid %d, gid %d, egid %d\n", _myEnv.uid, _myEnv.euid, _myEnv.gid, _myEnv.egid);
    _jobMsg("groups: %d\n",_myEnv.ngroups);
    for (i=0; i<_myEnv.ngroups; i++){
        _jobMsg("\t%d\n",_myEnv.groups[i]);
    }

    _jobMsg("\n");
    _jobMsg("My bebopd ptl %d, bebopd nid %d, bebopd pid %d, bebopd ptl %d\n",
        _myEnv.bebopdPtl, _myEnv.bnid, _myEnv.bpid, _myEnv.bptl);

    for (i=0; i<_jobArraySize; i++){

        if (_jobList[i].used){
             _jobMsg("Job handle %d\n",i);
             _displayJob(_jobList + i);
        }
        else{
             _jobMsg("(Job handle %d unused)\n",i);
        }
    }
}

static void
displayNodeSpec(yod_request *req, char *str)
{
    switch (req->specType){
        case YOD_NODE_REQ_ANY:
            _jobMsg(" anywhere you like\n");
            break;
        case YOD_NODE_REQ_LIST:
            _jobMsg(" out of the %d nodes in list %s\n",
                    req->spec.list.listsize, str);
                      
	    break;
        case YOD_NODE_REQ_RANGE:
            _jobMsg(" from the range %d through %d\n",
            req->spec.range.from_node,req->spec.range.to_node);
                break;
            default:
            _jobMsg(" ??????\n");
    }
}

static void
displayHostReply(hostReply_t *hr)
{
    _jobMsg("fh 0x%08x, curPos %u, isatty %u, srvr nid/pid/ptl %u/%u/%u\n",
      hr->retVal, hr->info.openAck.curPos, hr->info.openAck.isattyFlag,
      hr->info.openAck.srvrNid, hr->info.openAck.srvrPid,
      hr->info.openAck.srvrPtl);
}

static void
displayChars(unsigned char *c, int len, int width)
{
int i;

    _jobMsg("\n");
    for (i=0; i<len; i++){
        if (i && (i%width == 0)){
           _jobMsg("\n");
        }
        if ((*c > 31) && (*c < 127)) {
            _jobMsg("%c",*c);
        } else {
            _jobMsg("<%d>",(int)(*c));
        }

        c++;
    }
    _jobMsg("\n");
}
static void
displayNidList(int *list, int size)
{
int i;

    _jobMsg("\n");
    for (i=0; i<size; i++){
	if (i && (i%20==0)) _jobMsg("\n");
	_jobMsg("%d,",list[i]);
    }
    _jobMsg("\n");
}
static void
displaySpidList(spid_type *list, int size)
{
int i;

    _jobMsg("\n");
    for (i=0; i<size; i++){
	if (i && (i%20==0)) _jobMsg("\n");
	_jobMsg("%d,",list[i]);
    }
    _jobMsg("\n");
}
static void
displayPpidList(ppid_type *list, int size)
{
int i;

    _jobMsg("\n");
    for (i=0; i<size; i++){
	if (i && (i%20==0)) _jobMsg("\n");
	_jobMsg("%d,",list[i]);
    }
    _jobMsg("\n");
}
static void
displayNidRankList(nidOrder *list, int size)
{
int i;

    _jobMsg("\n");
    for (i=0; i<size; i++){
	if (i && (i%20==0)) _jobMsg("\n");
	_jobMsg("%d (%d),",list[i].nid,list[i].rank);
    }
    _jobMsg("\n");
}
static void
displayYodRequest(yod_request *req)
{
    _jobMsg(
      "nodes %d, session %d, node limit %d, priority %s, myptl %d, euid %d\n",
        req->nnodes,
        req->session_id, req->nnodes_limit, 
	(req->priority == SCAVENGER ? "scavenger" : "regular"),
	req->myptl, req->euid);

}
void
_displayInitialMsg(load_msg1 *msg1)
{
   _jobMsg("job ID %d, session ID %d, parent handle %d, my handle %d, nprocs %d, n_members %d \n",
             msg1->job_id, msg1->session_id, 
             msg1->parent_handle, msg1->my_handle,
             msg1->nprocs, msg1->n_members);
    _jobMsg("yod ID  %d/%d/%d\n", msg1->yod_id.nid, msg1->yod_id.pid, msg1->yod_id.ptl);
}

void
_displayLoadMsg(load_msg2 *msg)
{
int i;

    _jobMsg("stdin/stdout/stderr:\n");
    displayHostReply(&(msg->fstdio[0]));
    displayHostReply(&(msg->fstdio[1]));
    displayHostReply(&(msg->fstdio[2]));

    _jobMsg("uid %d, euid %d, gid %d, egid %d, umask 0x%08x\n",
          msg->uid, msg->euid,
          msg->gid, msg->egid, msg->u_mask);

    _jobMsg("env buffer length %d\n",msg->envbuflen);
    _jobMsg("option bits 0x%08x\n",msg->option_bits);
    _jobMsg("fyod node %d\n",msg->fyod_nid);
    _jobMsg("yod server portal %d\n",msg->app_serv_ptl);

    _jobMsg("from rank %d, to rank %d, arg buffer length %d, executable length %d\n",
      msg->data.fromRank, msg->data.toRank, msg->data.argbuflen, msg->data.execlen);

    _jobMsg("Number of groups %d\n\t",msg->ngroups);

    if (msg->ngroups < FEW_GROUPS){
        for (i=0; i<msg->ngroups; i++){
            _jobMsg("%d,",msg->groups[i]);
        }
        _jobMsg("\n");
    }
    else{
        _jobMsg("Too many groups to pack into load message.\n");
    }
    _jobMsg("Strace info length: %d\n",msg->straceMsgLen);

}


static void
displayMember(loadMbrs *mbr)
{

    _jobMsg("\n");
    _jobMsg("Member %p\n",mbr);

    _jobMsg("\tFrom rank %d to rank %d, arglen %d, execlen %d\n",
	mbr->data.fromRank, mbr->data.toRank, mbr->data.argbuflen, mbr->data.execlen);

    _jobMsg("\n");
    if (mbr->pname){
	_jobMsg("\tprogram name %s\n",mbr->pname);
    }
    else{
	_jobMsg("\tno program name\n");
    }

    _jobMsg("\n");
    _jobMsg("\tMembers using same executable: %d\n",mbr->pnameCount);
    _jobMsg("\tI'm using same as %p\n",mbr->pnameSameAs);

    _jobMsg("\tLocation of executable in memory: %p\n",mbr->exec);
    _jobMsg("\tCheck sum: %u\n",mbr->exec_check);

    _jobMsg("\n");
    if (mbr->execPath){
	_jobMsg("\texecutable copied to %s\n",mbr->execPath);
    }
    else{
	_jobMsg("\texecutable not copied to a file\n");
    }

    _jobMsg("\n");
    if (mbr->argstr){
	_jobMsg("\tArguments\n");
	displayChars(mbr->argstr, mbr->data.argbuflen, 60);
    }
    else{
	_jobMsg("\tno program arguments\n");
    }

    _jobMsg("\n");
    if (mbr->localListSize){
	_jobMsg("\tnode list string: %s\n",mbr->localListStr);
        if (mbr->localList){
	    displayNidList(mbr->localList, mbr->localListSize);
        } else{
            _jobMsg("\tnode list not built\n");
        }
    }
    else{
	_jobMsg("\tno node list specified\n");
    }

    _jobMsg("\n");
    _jobMsg("\tSize: %d\n",mbr->localSize);
    _jobMsg("\tSend or copy: %d\n",mbr->send_or_copy);
}
/*
** libjob users may call these *****************************************
*/
char *
job_stack_current()
{
    return job_stack[job_stack_ptr];
}
void
job_stack_display()
{
int i,ii,sp,from;
char *indent;

   if (job_stack_ptr < 970)
       from = job_stack_ptr + 30;
   else
       from = 999;

   for (i=from,sp=0; i>=job_stack_ptr; i--,sp++){

      for (ii=0; ii<sp; ii++) _jobMsg(" ");

      if (sp < MAXDEPTH) indent = depthmarker[sp];

      _jobMsg("%s%s\n",indent,job_stack[i]);
   }
}
char *
job_strerror()
{
    return _jobErrStr;
}
#define LAST_STATUS_BIT  5

static char *jobStatuses[LAST_STATUS_BIT+1]={
"node request built",
"pct list allocated",
"sent request to load to pcts",
"all pcts say OK to load",
"application started on compute nodes",
"all application processes have terminated"
};

static char *noStatus = "no status yet";

char *
displayJobStatus(int status)
{
int i;

    for (i = LAST_STATUS_BIT; i >= 0; i--){

         if (status & (1 << i)){
             return jobStatuses[i];
         }
    }
    return noStatus;
}

