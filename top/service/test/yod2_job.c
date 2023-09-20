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
** $Id: yod2_job.c,v 1.3 2001/11/26 17:26:52 lafisk Exp $
**
** job service requests that can be sent by the application:
**
**
**CMD_JOB_CREATE          spawn a new application
**
**CMD_JOB_STATUS          return status of job in my family
**
**CMD_SYNCHRONIZE_JOBS    request to synchronize two jobs in family
**
**CMD_SYNCHRONIZE_STATUS  status of synchronization operation 
**
**CMD_JOB_NID_MAP         return the phys node ID map for a job
**
**CMD_JOB_PID_MAP         return the portal process ID map for a job
**
**CMD_JOB_SIGNAL          send a signal to a job in my family
**
**CMD_JOB_TERMINATION STATUS  get exit codes, terminating signals, and
**                               or terminator of each process in a job
**
**CMD_NODES_REMAINING     how many of my allocated nodes are not
**                          being used by jobs in my family
**
**
**  "my family" is all jobs started by the same yod2 that
**   started me.  So that's ancestors, children, siblings
**   and cousins.
**
*/

#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "rpc_msgs.h"
#include "cplant.h"
#include "job.h"
#include "yod2.h"
#include "srvr_comm.h"

static hostReply_t commandAck;
static char *replyBuf;
static int  replyLen;
static char freeReplyBuf;

static int send_ack(control_msg_handle *mh);
static int send_nack(control_msg_handle *mh);
static int send_to_job(control_msg_handle *mh);

static int statusSynch(jobTree *me, int otherHandle);
static int startSynch(jobTree *me, int otherHandle);

static int jobNodesRemaining(jobTree *job);
static int jobTerminationInfo(control_msg_handle *mh, jobTree *srcjob);
static int jobSignal(control_msg_handle *mh, jobTree *job);
static int jobSynchronize(control_msg_handle *mh, jobTree *job, int rtype);
static int jobMaps(control_msg_handle *mh, jobTree *job, int rtype);
static int jobStatus(control_msg_handle *mh, int srcrank, jobTree *job);
static int jobCreate(control_msg_handle *mh, int srcrank, jobTree *job);

#define SEND_ACK         1
#define SEND_NACK        2
#define SEND_DATA        3

#define SENDTIMEOUT   10
int
perform_service_job(jobTree *job, int srcrank, control_msg_handle *mh)
{
int rtype, rc, status;

    rtype = ((hostCmd_t *)(SRVR_HANDLE_USERDEF(*mh)))->type;

    if ( (rtype < FIRST_JOB_CMD_NUM) || (rtype > LAST_JOB_CMD_NUM)){

        yoderrmsg("perform_service_job - invalid type %d\n",rtype);
        return -1;
    }

    switch (rtype) {

        case CMD_JOB_CREATE:

            rc = jobCreate(mh, srcrank, job);

            break;
            
        case CMD_JOB_STATUS:

            rc = jobStatus(mh, srcrank, job);

            break;
            
        case CMD_SYNCHRONIZE_JOBS:
        case CMD_SYNCHRONIZE_STATUS:

            rc = jobSynchronize(mh, job, rtype);

            break;

        case CMD_JOB_NID_MAP:
        case CMD_JOB_PID_MAP:

            rc = jobMaps(mh, job, rtype);

            break;


        case CMD_JOB_SIGNAL:

            rc = jobSignal(mh, job);

            break;

        case CMD_JOB_TERMINATION_STATUS:

            rc = jobTerminationInfo(mh, job);

            break;

        case CMD_NODES_REMAINING:

            rc = jobNodesRemaining(job);

            break;

    }

    if (rc == SEND_NACK){
        send_nack(mh);
        status = -1;
    }
    else if (rc == SEND_ACK){
        send_ack(mh);
        status = 0;
    }
    else if (rc == SEND_DATA){
        send_to_job(mh);
        status = 0;
    }

    return status;
}
static int
jobTerminationInfo(control_msg_handle *mh, jobTree *srcjob)
{
#undef ACK
#define ACK     ( ack.info.jobStatusAck)
#undef CMD
#define CMD     ( cmd->info.jobCmd)

hostReply_t ack;
hostCmd_t *cmd;
jobTree *job;
int  infolen, handle;

    cmd    = (hostCmd_t *)SRVR_HANDLE_USERDEF(*mh);
    handle = CMD.yodHandle;

    job = findJob(handle);

    if (!job){
        return SEND_NACK;
    }

    if ( (job->nfail) ||                     /* the load failed */
         (job->status == LOAD_FAILED_JOB) ){

        ACK.job_id =    job->job_id;
        ACK.yodHandle = job->handle;
        ACK.progress  = jobInfo_jobState(job->handle);
        ACK.nprocs    = job->nprocs;
        ACK.retVal    = -1;

        return SEND_ACK;
    }

    if (!job->endCode){  /* no processes have terminated */

        ACK.job_id =    job->job_id;
        ACK.yodHandle = job->handle;
        ACK.progress  = jobInfo_jobState(job->handle);
        ACK.nprocs    = job->nprocs;
        ACK.retVal    = 0;

        return SEND_ACK;
    }

    infolen = job->nprocs * sizeof(final_status);

    if (infolen > IOBUFSIZE){
        yoderrmsg("jobTerminationInfo: info length (%d) exceeds buffer size\n",
                    infolen);
        yoderrmsg("Rewrite send_to_job to send IOBUFSIZE packets instead.\n");
        return SEND_NACK;
    }

    if (DBG_FLAGS(DBG_JOBS)){
        yodmsg("Job termination info request, from job %d, for job %d\n",
                  srcjob->job_id, job->job_id);
    }
    
    replyBuf = (char *)(job->endCode);
    replyLen = infolen;
    freeReplyBuf = 0;

    ACK.job_id =    job->job_id;
    ACK.yodHandle = job->handle;
    ACK.progress  = jobInfo_jobState(job->handle);
    ACK.nprocs    = job->nprocs;
    ACK.retVal    = job->ndone; /* number of procs that have terminated */

    return SEND_DATA;
}
static int
jobMaps(control_msg_handle *mh, jobTree *srcjob, int rtype)
{
#undef ACK
#define ACK     ( ack.info.jobStatusAck)
#undef CMD
#define CMD     ( cmd->info.jobCmd)

int *list, listlen, handle;
hostReply_t ack;
hostCmd_t *cmd;
jobTree *job;

    cmd    = (hostCmd_t *)SRVR_HANDLE_USERDEF(*mh);
    handle = CMD.yodHandle;

    job = findJob(handle);

    if (!job){
        return SEND_NACK;
    }

    listlen = job->nprocs * sizeof(int);

    if (listlen > IOBUFSIZE){
        yoderrmsg("jobMaps: map length exceeds buffer size\n");
        yoderrmsg("Rewrite send_to_job to send IOBUFSIZE packets instead.\n");
        return SEND_NACK;
    }

    list = NULL;

    if (rtype == CMD_JOB_NID_MAP){

        if (DBG_FLAGS(DBG_JOBS)){
	    yodmsg("Job nid map request, from job %d, for job %d\n",
                  srcjob->job_id, job->job_id);
        }
        list = jobInfo_nodeList(handle);
    }
    else if (rtype == CMD_JOB_PID_MAP){

        if (DBG_FLAGS(DBG_JOBS)){
	    yodmsg("Job pid map request, from job %d, for job %d\n",
                  srcjob->job_id, job->job_id);
        }
        list = jobInfo_appPpidList(handle);
    }
    
    if (!list){
        return SEND_NACK;
    }

    replyBuf = (char *)list;
    replyLen = listlen;
    freeReplyBuf = 1;

    ACK.job_id =    job->job_id;
    ACK.yodHandle = job->handle;
    ACK.progress  = jobInfo_jobState(job->handle);
    ACK.nprocs    = job->nprocs;
    ACK.retVal    = 0;

    return SEND_DATA;
}
static int
jobNodesRemaining(jobTree *job)
{
#undef ACK
#define ACK     ( commandAck.info.jobStatusAck)

    ACK.job_id =    job->job_id;   /* info about requester */
    ACK.yodHandle = job->handle;   /* for what it's worth  */
    ACK.nprocs    = job->nprocs;
    ACK.progress  = jobInfo_jobState(job->handle);

    ACK.retVal = numRemainingNodes();  /* for whole job family */

    if (DBG_FLAGS(DBG_JOBS)){
	yodmsg(
   "Nodes remaining request from job %d, answer is %d\n",
                  job->job_id, ACK.retVal);
    }

    return SEND_ACK;
}
static int
jobSignal(control_msg_handle *mh, jobTree *job)
{
#undef CMD
#undef ACK
#define CMD     ( cmd->info.jobCmd)
#define ACK     ( commandAck.info.jobStatusAck)

hostCmd_t    *cmd;
int        targetHandle, sig, rc;
jobTree   *targetJob;

    cmd    = (hostCmd_t *)SRVR_HANDLE_USERDEF(*mh);

    targetHandle = CMD.yodHandle;
    sig          = CMD.sigNum;

    targetJob = findJob(targetHandle);

    if (!targetJob){
        return SEND_NACK;
    }

    if (DBG_FLAGS(DBG_JOBS)){
	yodmsg(
   "Job signal request, from job %d, for job %d, signal %d (%s)\n",
                  job->job_id, targetJob->job_id,
                  sig, select_signal_name(sig));
    }
    
    rc = job_send_signal(targetHandle, sig);

    if (rc){
        return SEND_NACK;
    }

    ACK.job_id =    targetJob->job_id;  /* info about signalled job */
    ACK.yodHandle = targetHandle;
    ACK.progress  = jobInfo_jobState(targetHandle);
    ACK.nprocs    = targetJob->nprocs;

    ACK.retVal    = 0;

    return SEND_ACK;
}
static int
jobSynchronize(control_msg_handle *mh, jobTree *job, int rtype)
{
#undef CMD
#undef ACK
#define CMD     ( cmd->info.jobCmd)
#define ACK     ( commandAck.info.jobSynchAck)

hostCmd_t    *cmd;

    cmd    = (hostCmd_t *)SRVR_HANDLE_USERDEF(*mh);

    if (rtype == CMD_SYNCHRONIZE_JOBS){

        ACK.status = startSynch(job, CMD.yodHandle);

        if (DBG_FLAGS(DBG_JOBS)){
	    yodmsg(
	   "Job synchronize request from job %d, for handle %d (%d)\n",
		     job->job_id, CMD.yodHandle, ACK.status);
        }

    }
    else if (rtype == CMD_SYNCHRONIZE_STATUS){

        ACK.status = statusSynch(job, CMD.yodHandle);

        if (DBG_FLAGS(DBG_JOBS)){
	    yodmsg(
	   "Job synchronize status request from job %d, for handle %d (%d)\n",
		     job->job_id, CMD.yodHandle, ACK.status);
        }
    }

    return SEND_ACK;
}
static int
jobStatus(control_msg_handle *mh, int srcrank, jobTree *job)
{
#undef CMD
#undef ACK
#define CMD     ( cmd->info.jobCmd)
#define ACK     ( commandAck.info.jobStatusAck)

hostCmd_t    *cmd;
jobTree      *queryJob;

    cmd    = (hostCmd_t *)SRVR_HANDLE_USERDEF(*mh);

    queryJob = findJob(CMD.yodHandle);

    if (!queryJob){
        return SEND_NACK;
    }

    if (DBG_FLAGS(DBG_JOBS)){
	yodmsg("Job status request, from job %d, rank %d, for job %d\n",
                  job->job_id, srcrank, queryJob->job_id);
    }

    ACK.job_id    = queryJob->job_id;
    ACK.yodHandle = CMD.yodHandle;
    ACK.progress  = jobInfo_jobState(CMD.yodHandle);
    ACK.nprocs    = queryJob->nprocs;

    if (queryJob->status == LOAD_FAILED_JOB){
        ACK.retVal = -1;
    }
    else{
        ACK.retVal = 0;
    }

    return SEND_ACK;
}
static int
jobCreate(control_msg_handle *mh, int srcrank, jobTree *job)
{
#undef CMD
#undef ACK
#define CMD     ( cmd->info.jobCmd)
#define ACK     ( commandAck.info.jobStatusAck)

hostCmd_t    *cmd;
char         *jobReq, *parentName;
ndrequest    *ndreq;
jobTree      *newJob;
int          buf, rc, len;

    if (DBG_FLAGS(DBG_JOBS)){
	yodmsg("Job create request received from job %d, rank %d\n",
                  job->job_id, srcrank);
    }

    cmd    = (hostCmd_t *)SRVR_HANDLE_USERDEF(*mh);
    len    = SRVR_HANDLE_TRANSFER_LEN(*mh);

    parentName = yodJobPname(job, srcrank);

    if (!parentName){
        yoderrmsg("jobCreate: Can't find parent executable name.\n");
        return SEND_NACK;
    }

    /*
    ** Pull down the job description from the parent.
    */

    buf = get_work_buf();

    if (buf < 0){
        yoderrmsg("jobCreate: Unable to obtain local work buffer.\n");
        return SEND_NACK;
    }

    if (len > IOBUFSIZE){
        yoderrmsg("jobCreate: Are you serious?  Request is length %d\n",
                        len);
        yoderrmsg(
        "Rewrite jobCreate to pull in packets like host_write2().\n");
        free_work_buf(buf);
        return SEND_NACK;
    }

    rc = srvr_comm_put_reply(mh, (jobReq = workBufData(buf)), len);

    if (rc){
        yoderrmsg("jobCreate: put reply problem (%s)\n",
                    CPstrerror(CPerrno));
        free_work_buf(buf);
        return SEND_NACK;
    }

    /*
    ** Create the node request, request the nodes and
    ** notify the PCTs of intent to load job.  new_job()
    ** adds the new job to the list of loading or running
    ** jobs.
    */

    ndreq = unpackNodeRequest(jobReq, len, parentName);

    if (!ndreq){
        yoderrmsg("jobCreate: error in job request message.\n");
        free_work_buf(buf);
        return SEND_NACK;
    }

    free_work_buf(buf);

    newJob = new_job(ndreq, job->handle);

    if (!newJob){
        yoderrmsg("jobCreate: error creating new job.\n");
        /* new_job calls free(ndreq) if it fails */
        return SEND_NACK;
    }

    if (DBG_FLAGS(DBG_JOBS)){
	yodmsg("New job created, job id %d, %d nodes, status 0x%x (%s)\n",
                 newJob->job_id, newJob->nprocs, newJob->status,
                displayJobStatus(newJob->status));
    }

    /*
    ** Reply with new job status to parent job.  Include
    ** yod handle for new job for subsequent requests.
    */
    
    ACK.job_id    = newJob->job_id;
    ACK.yodHandle = newJob->handle;
    ACK.progress  = jobInfo_jobState(newJob->handle);
    ACK.nprocs    = newJob->nprocs;

    ACK.retVal    = 0;

    return SEND_ACK;
}
static int
send_ack(control_msg_handle *mh)
{
int nid,pid,ptl,rc,seqno;

    seqno = ((hostCmd_t *)SRVR_HANDLE_USERDEF(*mh))->my_seq_no;
    nid = SRVR_HANDLE_NID(*mh);
    pid = SRVR_HANDLE_PID(*mh);
    ptl = ((hostCmd_t *)(SRVR_HANDLE_USERDEF(*mh)))->ctl_ptl;

    commandAck.your_seq_no = seqno;
    commandAck.retVal      = 0;
    commandAck.hostErrno   = 0;

    rc = srvr_send_to_control_ptl(nid, pid, ptl,
          JOB_MANAGEMENT_ACK, (char *)&commandAck, 
          sizeof(hostReply_t));

    if (DBG_FLAGS(DBG_JOBS)){
	yodmsg("Job management request SUCCESS\n");
    }

    return rc;
}
static int
send_nack(control_msg_handle *mh)
{
int nid,pid,ptl,rc,seqno;
hostReply_t nack;

    seqno = ((hostCmd_t *)(SRVR_HANDLE_USERDEF(*mh)))->my_seq_no;
    nid   = SRVR_HANDLE_NID(*mh);
    pid   = SRVR_HANDLE_PID(*mh);
    ptl   = ((hostCmd_t *)(SRVR_HANDLE_USERDEF(*mh)))->ctl_ptl;

    nack.your_seq_no = seqno;
    nack.retVal      = -1;
    nack.hostErrno   = 1;

    rc = srvr_send_to_control_ptl(nid, pid, ptl,
          JOB_MANAGEMENT_ACK, (char *)&nack, sizeof(hostReply_t));

    if (DBG_FLAGS(DBG_JOBS)){
	yodmsg("Job management request FAILURE\n");
    }

    return rc;
}
static int
send_to_job(control_msg_handle *mh)
{
time_t start;
int slot, myrc, status, seqno;
int nid[2], pid[2], ptl[2];

    myrc = 0;

    nid[0] = SRVR_HANDLE_NID(*mh);
    pid[0] = SRVR_HANDLE_PID(*mh);
    ptl[0] = ((hostCmd_t *)(SRVR_HANDLE_USERDEF(*mh)))->ctl_ptl;
    seqno = ((hostCmd_t *)SRVR_HANDLE_USERDEF(*mh))->my_seq_no;

    commandAck.your_seq_no = seqno;
    commandAck.retVal      = 0;
    commandAck.hostErrno   = 0;

    slot = srvr_comm_put_req(replyBuf, replyLen, JOB_MANAGEMENT_ACK,
                    (char *)&commandAck, sizeof(hostReply_t),
                    1, nid, pid, ptl);

    if (slot < 0){
        return -1;
    }

    start = time(NULL);

    while (1){
        status = srvr_test_read_buf(slot, 1);

        if (status == 1){  
            myrc = 0;
            break;
        }
        else if (status == -1){
            myrc = -1;
            break;
        }
        else if ( (time(NULL) - start) > SENDTIMEOUT){
                myrc = -1;
                break;
        }
        sleep(1);
    }

    srvr_delete_buf(slot);

    if (freeReplyBuf) free(replyBuf);

    return myrc;
}
/*******************************************************************
 job synchronization

o  One process from each job requests the jobs to synchronize.
o  Once both jobs have made the request, yod considers them synchronized.
o  One process from each job learns that both jobs have requested
      to synchronize.  
o  Once both jobs have been informed that synchronization is complete,
      yod forgets about the operation.

 A job synchronization operation is uniquely identified by the
 two jobs participating in the operation.  So you can not
 have different groups in the two jobs synchronizing at the same
 time.  The motiviation for this synchronization is that the
 spawning processes (in MPI_COMM_SPAWN) will want to synchronize
 with the child processes (in MPI_GET_PARENT).

 This is an operation that should occur infrequently - hence the
 lack of a fancy data structure.
********************************************************************/

typedef struct _synchMember{
  jobTree *job;
  char in;
  char out;
} synchMember;

typedef struct _synch{
  synchMember s1;
  synchMember s2;
  int used;
} synch;

static synch *synchArray=NULL;
static int SAsize = 0;
static int SAincrement = 10;

static int
growSynchArray()
{
char *cstart;
int len, sizeadded;

    SAsize += SAincrement;

    len       = sizeof(synch) * SAsize;
    sizeadded = sizeof(synch) * SAincrement;

    synchArray = (synch *)realloc(synchArray, len);

    if (!synchArray) return -1;

    cstart = (char *)synchArray + len - sizeadded;

    memset(cstart, 0, sizeadded);

    return 0;
}
static synch *
findSyncRecord(jobTree *j1, jobTree *j2)
{
int i;
synch *s;

   for (i=0; i<SAsize; i++){
       if (!synchArray[i].used) continue;

       s = synchArray + i;

       if ( ( (s->s1.job == j1) && (s->s2.job == j2)) ||
            ( (s->s2.job == j1) && (s->s1.job == j2))   ){

           return s;

       }
   }
   return NULL;
}
static void
clearSyncRecord(synch *s)
{
    memset((void *)s, 0, sizeof(synch));
}
static synch *
unusedSyncRecord()
{
int i,rc,oldsize;

   for (i=0; i<SAsize; i++){
       if (!synchArray[i].used) return (synchArray+i);
   }

   oldsize = SAsize;

   rc = growSynchArray();
   if (rc) return NULL;

   return (synchArray + oldsize);
}
static int
newSyncRecord(jobTree *me, jobTree *other)
{
synch *new;

   new = unusedSyncRecord();

   if (!new) return -1;

   new->s1.job = me;
   new->s1.in  = 1;

   new->s2.job = other;

   new->used   = 1;
   
   return 0;
}
static int
statusSyncRecord(synch *srec, jobTree *me)
{
int status;    
synchMember *mine;
synchMember *theirs;

   if (srec->s1.job == me){
      mine   = &(srec->s1);
      theirs = &(srec->s2);
   }
   else if (srec->s2.job == me){
      mine   = &(srec->s2);
      theirs = &(srec->s1);
   }
   else{
      return SYNC_ERROR;
   }

   mine->in = 1;

   if (theirs->in == 1){
       mine->out = 1;
       status = SYNC_COMPLETED; 
   }
   else{
      status = SYNC_IN_PROGRESS;
   }

   if (mine->out && theirs->out){
       clearSyncRecord(srec);
   }
   return status;
}

/*
** Return 1 if synchronization is in progress
** Return 2 if synchronization is completed
** Return -1 on error
*/
static int
startSynch(jobTree *me, int otherHandle)
{
jobTree *other;
synch *rec;
int status, rc;

    other = findJob(otherHandle);

    if (!other){
        return SYNC_ERROR;
    }

    rec = findSyncRecord(me, other);

    if (rec){
        status = statusSyncRecord(rec, me); 
    }
    else{
        rc = newSyncRecord(me, other);

        if (rc){
            status = SYNC_ERROR;    
        }
        else{
            status = SYNC_IN_PROGRESS;    
        }
    }
    return status;
}
static int
statusSynch(jobTree *me, int otherHandle)
{
jobTree *other;
synch *rec;
int status;

    other = findJob(otherHandle);

    if (!other){
        return SYNC_ERROR;
    }

    rec = findSyncRecord(me, other);

    if (rec){
        status = statusSyncRecord(rec, me); 
    }
    else{
        status = SYNC_ERROR;    
    }
    return status;
}
/*******************************************************************
** INTRA_JOB_BARRIER - all processes in the job send this message 
** type, when yod has heard from all of them, it sends a message
** back to them.  
*******************************************************************/

static void
clear_sync_request(jobTree *job)
{
    memset(job->syncWithYod, 0, job->nprocs);
    job->nsynced = 0;
}
int
register_sync_request(jobTree *job, int srcrank, control_msg_handle *mh)
{
int i, *appinfo;

    if (!job->syncWithYod){

        job->syncWithYod = (char *)calloc(job->nprocs , sizeof(char));
        job->syncPtl     = (short *)malloc(job->nprocs * sizeof(short));
        job->syncPid     = (short *)malloc(job->nprocs * sizeof(short));

        if (!job->syncWithYod || !job->syncPtl || !job->syncPid){

            yoderrmsg("allocate syncWithYod arrays (%s)\n",strerror(errno));

            if (job->syncWithYod) free(job->syncWithYod);
            if (job->syncPtl)     free(job->syncPtl);
            if (job->syncPid)     free(job->syncPid);

            job->syncWithYod = NULL;
            job->syncPtl     = NULL;
            job->syncPid     = NULL;
      
            return -1;
        }
        job->nsynced = 0;
    }

    if (job->syncWithYod[srcrank]) return -1;

    if (DBG_FLAGS(DBG_JOBS)){
        yodmsg("Intra job synchronization, job %d, rank %d\n",
                  job->job_id, srcrank);
    }

    appinfo = (int *)SRVR_HANDLE_USERDEF(*mh);

    job->syncPtl[srcrank] = (short)appinfo[0];
    job->syncPid[srcrank] = (short)appinfo[1];

    job->syncWithYod[srcrank] = 1;
    job->nsynced++;

    if (job->nsynced == job->nprocs){

        for (i=0; i<job->nprocs; i++){

             srvr_send_to_control_ptl(
                   job->nodeList[i], job->syncPid[i], job->syncPtl[i],
                    INTRA_JOB_BARRIER, NULL, 0);
        }

        if (DBG_FLAGS(DBG_JOBS)){
            yodmsg("Intra job synchronization, job %d, COMPLETE\n",
                      job->job_id);
        }
        clear_sync_request(job);
    }
    return 0;
}

