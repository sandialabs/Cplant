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
$Id: DynAlloc.c,v 1.6 2001/11/24 23:34:43 lafisk Exp $
*/

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "pct_start.h"
#include "portal_assignments.h"
#include "ppid.h"
#include "srvr_comm.h"
#include "bebopd.h"
#include "cplant_host.h"
#include "srvr_err.h"
#include "proc_id.h"
#include "rpc_msgs.h"
#include "srvr_comm.h"
#include "srvr_coll.h"
#include "srvr_err.h"
#include "cplant.h"
#include "puma.h"

static int dnaInited=0;

int 
init_dyn_alloc()
{
    if (dnaInited) return 0;

    /*
    ** portal for bebopd queries
    */

    _my_dna_ptl = srvr_init_control_ptl(2);

    if (_my_dna_ptl == SRVR_INVAL_PTL) return -1;

    dnaInited = 1;

    return _my_dna_ptl;
}

int 
end_dyn_alloc() 
{
    srvr_release_control_ptl(_my_dna_ptl);

    dnaInited = 0;

    return 0;
}

/*
** Get bebopd nid/pid/ptl for informational requests.
**
**   Cplant apps will query the PCT for this info.  The PCT
**   stays up-to-date on changes in location of the bebopd.
**
**   Cplant servers will read the cplant-host file the first
**   time this is called, and then return that value each subsequent
**   time it is called unless "newLookUp == TRUE".  If newLookUp
**   is requested, Cplant servers will read the cplant-host
**   file again.
*/
extern server_id __pct_id;
static server_id bebopd_id = {-1,-1,-1};

int
get_bebopd_id(int *bnid, int *bpid, int *bptl, int newLookUp)
{
time_t t1;
int rc, list, count, ptl;
control_msg_handle mhandle;

    if (newLookUp || (___proc_type == APP_TYPE)){
        bebopd_id.nid = -1; bebopd_id.pid = -1; bebopd_id.ptl = -1;
    }

    if (___proc_type == SERV_TYPE){   /* read bebopd registration file */

        if (bebopd_id.nid == -1){
	    if ( cplantHost_getNid("bebopd", 1, &list, &count) != OK ){
		log_msg("Can not get nid from cplant-host file\n");
		return -1;
	    }

	    if (count == 1){
		bebopd_id.nid = list;
		bebopd_id.pid = PPID_BEBOPD;
		bebopd_id.ptl = (int) DNA_PTL;
	    }
        }

    }
    else{          /* ask PCT for bebopd ID - it keeps current on that */

        if (!dnaInited){
	    ptl = init_dyn_alloc();

	    if (ptl == -1){
	        return -1;
	    }
	}

	srvr_free_all_control_msgs(_my_dna_ptl);

        rc = srvr_send_to_control_ptl(__pct_id.nid, __pct_id.pid, __pct_id.ptl,
	            CHILD_BEBOPD_ID_REQUEST, (char *)&_my_dna_ptl, sizeof(int));

        t1 = time(NULL);

	SRVR_CLEAR_HANDLE(mhandle);

        while (1){

	    rc = srvr_get_next_control_msg(_my_dna_ptl, &mhandle,
	                    NULL, NULL, NULL);

            if (rc == 0){
		if ((time(NULL) - t1) > 10){
		    CPerrno = ERECVTIMEOUT;
		    break;
		}
		continue;
	    }

	    if (rc < 0) break;

            memcpy((void *)&bebopd_id, 
	           (void *)SRVR_HANDLE_USERDEF(mhandle), sizeof(server_id));
	    break;
	}

    }
    if (bebopd_id.nid == -1) return -1;

    *bnid = bebopd_id.nid;
    *bpid = bebopd_id.pid;
    *bptl = bebopd_id.ptl;

    return 0;
}
/*******************************************************************/
/*  Job creation, synchronization, and signalling functions        */
/*******************************************************************/

static int collectiveCallCheck(int nMembers, int *rankList, char *nm);

/*
** CplantSpawnJob sends a request to yod to start a new application.
** yod requests the nodes from bebopd, and initiates the load.  yod
** returns a job handle to the caller. The load may not be completed, 
** as cycle-stealers may need to be killed off to make way for the 
** new application.  The subsequent status function returns the current 
** status of the loading job.  Only one process should make the
** spawn request.
**
** I'm not sure whether callers want to do their own group management 
** and have a single process call this function, or whether they want 
** us to do the collective part, so we provide both types of calls.
**
** If you want to use collective calls (the *Grp versions), you must 
** call CplantInitCollective once beforehand.  It must be called
** by all processes in the application and it contains a barrier.
**
** Caller should free the jobFamilyInfo structure returned when done
** with it.
*/

static jobFamilyInfo tempJob;

jobFamilyInfo *
CplantSpawnJobGrp(int nlines,    /* number of command lines */
             char **pnames, /* executable path names   */
             char ***argvs, /* argument list for each  */
             int *nnodes,   /* number of nodes for each */
             int *nprocs,   /* number of processes per node for each */
             int nMembers,
             int *rankList,
             int tmout)
{
int rc, iamRoot;
jobFamilyInfo *jobp;

    if (nMembers == 0) rankList = NULL;
    else if (rankList == NULL) nMembers = 0;

    iamRoot = collectiveCallCheck(nMembers, rankList, "CplantSpawnJobGrp");

    if (iamRoot == -1) return NULL;

    if (iamRoot){

        jobp = CplantSpawnJob(nlines, pnames, argvs, nnodes, nprocs);

        if (!jobp){
            jobp = &tempJob;
            jobp->error  = 2;
        }
    }
    else{
        jobp = (jobFamilyInfo *)malloc(sizeof(jobFamilyInfo));

        if (!jobp){
            CPerrno = ENOMEM;
            return NULL;
        }
    }

    rc = dsrvr_bcast((char *)jobp, sizeof(jobFamilyInfo), 
                  tmout, CMD_JOB_CREATE, rankList, nMembers);

    if ((rc != DSRVR_OK) || (jobp->error == 2)){
        if (jobp != &tempJob) free(jobp);
        return NULL;
    }
    
    return jobp;
}
jobFamilyInfo *
CplantSpawnJob(int nlines,    /* number of command lines */
             char **pnames, /* executable path names   */
             char ***argvs, /* argument list for each  */
             int *nnodes,   /* number of nodes for each */
             int *nprocs)   /* number of processes for each */
{
#undef CMD
#undef ACK
#define CMD     (cmd.info.jobCmd)
#define ACK     (ack.info.jobStatusAck)

int reqlen, rc;
char *req;
hostCmd_t cmd;
hostReply_t ack;
jobFamilyInfo *newJob;
    
    rc = packNodeRequest(nlines, pnames, argvs, nnodes, nprocs,
                       &req, &reqlen);

    if (rc){
        CPerrno = EINVAL;
        return NULL;
    }

    rc = _host_rpc(&cmd, &ack, CMD_JOB_CREATE, 
                   YOD_NID, YOD_PID, YOD_PTL,
                   req, reqlen, NULL, 0);

    free(req);

    if ((rc != 0) || (ack.hostErrno)){
        return NULL;
    }

    newJob = (jobFamilyInfo *)malloc(sizeof(jobFamilyInfo));

    if (!newJob){
        CPerrno = ENOMEM;
        return NULL;
    }

    newJob->job_id       = ACK.job_id;
    newJob->yodHandle    = ACK.yodHandle;
    newJob->status       = ACK.progress;
    newJob->nprocs       = ACK.nprocs;
    newJob->error        = ACK.retVal;
    newJob->callerHandle = NULL;
    
    return newJob;
}
/*
** returns status of any job managed by yod
*/
int
CplantFamilyStatusGrp(jobFamilyInfo *job,
                   int nMembers, int *rankList, int tmout)
{
int rc, iamRoot;

    if (nMembers == 0) rankList = NULL; 
    else if (rankList == NULL) nMembers = 0;

    iamRoot = 
      collectiveCallCheck(nMembers, rankList, "CplantFamilyStatusGrp");

    if (iamRoot == -1) return -1;

    if (iamRoot){
        rc = CplantFamilyStatus(job);

        if (rc){
           tempJob.error = 2;
        }
        else{
           memcpy((void *)&tempJob, (void *)job, sizeof(jobFamilyInfo));
        }
    }

    rc = dsrvr_bcast((void *)&tempJob, sizeof(jobFamilyInfo),
                       tmout, CMD_JOB_STATUS, rankList, nMembers);

    if ((rc != DSRVR_OK) || (tempJob.error == 2)){
        return -1;
    }

    if (!iamRoot){
        job->job_id = tempJob.job_id;
        job->status = tempJob.status;
        job->nprocs = tempJob.nprocs;
        job->error =  tempJob.error;
    }
   
    return 0;
}
int
CplantFamilyStatus(jobFamilyInfo *job)
{
#undef CMD
#undef ACK
#define CMD     (cmd.info.jobCmd)
#define ACK     (ack.info.jobStatusAck)

int rc;
hostCmd_t cmd;
hostReply_t ack;

    CMD.yodHandle = job->yodHandle;

    rc = _host_rpc(&cmd, &ack, CMD_JOB_STATUS, 
                   YOD_NID, YOD_PID, YOD_PTL,
                   NULL, 0, NULL, 0);

    if ((rc != 0) || (ack.hostErrno)){
        return -1;
    }

    job->job_id = ACK.job_id;
    job->status = ACK.progress;
    job->nprocs = ACK.nprocs;
    job->error  = ACK.retVal;
    
    return 0;
}
jobFamilyInfo *
CplantMyParentGrp(int nMembers, int *rankList, int tmout)
{
int rc, iamRoot;
jobFamilyInfo *jobp;

    if (nMembers == 0) rankList = NULL;
    else if (rankList == NULL) nMembers = 0;

    iamRoot = collectiveCallCheck(nMembers, rankList, "CplantMyParentGrp");

    if (iamRoot == -1) return NULL;

    if (iamRoot){

       jobp = CplantMyParent();

       if (!jobp){
          jobp = &tempJob;
          jobp->error  = 2;
       }
    }
    else{
        jobp = (jobFamilyInfo *)malloc(sizeof(jobFamilyInfo));

        if (!jobp){
            CPerrno = ENOMEM;
            return NULL;
        }
    }
    rc = dsrvr_bcast((char *)jobp, sizeof(jobFamilyInfo),
                  tmout, CMD_JOB_STATUS, rankList, nMembers);

    if ((rc != DSRVR_OK) || (jobp->error == 2)){
        if (jobp != &tempJob) free(jobp);
        return NULL;
    }

    return jobp;
}
jobFamilyInfo *
CplantMyParent()
{
jobFamilyInfo *job;
int rc;

    if (_my_parent_handle == INVAL) return NULL;

    job = (jobFamilyInfo *)malloc(sizeof(jobFamilyInfo));

    if (!job){
        CPerrno = ENOMEM;
        return NULL;
    }

    job->yodHandle = _my_parent_handle;

    rc = CplantFamilyStatus(job);

    if (rc){
        free(job);
        return NULL;
    } 
    return job; 
}
jobFamilyInfo *
CplantMySelfGrp(int nMembers, int *rankList, int tmout)
{
int rc, iamRoot;
jobFamilyInfo *jobp;

    if (nMembers == 0) rankList = NULL;
    else if (rankList == NULL) nMembers = 0;

    iamRoot = collectiveCallCheck(nMembers, rankList, "CplantMySelfGrp");

    if (iamRoot == -1) return NULL;

    if (iamRoot){

       jobp = CplantMySelf();

       if (!jobp){
          jobp = &tempJob;
          jobp->error  = 2;
       }
    }
    else{
        jobp = (jobFamilyInfo *)malloc(sizeof(jobFamilyInfo));

        if (!jobp){
            CPerrno = ENOMEM;
            return NULL;
        }
    }
    rc = dsrvr_bcast((char *)jobp, sizeof(jobFamilyInfo),
                  tmout, CMD_JOB_STATUS, rankList, nMembers);

    if ((rc != DSRVR_OK) || (jobp->error == 2)){
        if (jobp != &tempJob) free(jobp);
        return NULL;
    }

    return jobp;
}

jobFamilyInfo *
CplantMySelf()
{
jobFamilyInfo *job;
int rc;

    if (_my_own_handle == INVAL) return NULL;

    job = (jobFamilyInfo *)malloc(sizeof(jobFamilyInfo));

    if (!job){
        CPerrno = ENOMEM;
        return NULL;
    }

    job->yodHandle = _my_own_handle;

    rc = CplantFamilyStatus(job);

    if (rc){
        free(job);
        return NULL;
    } 
    return job; 
}
/*
** A synchronization point for two jobs in the same family.
** (Normally a parent in MPI_COMM_SPAWN and a child in 
** MPI_COMM_GET_PARENT.)  One process from each job lets
** yod know they want to synchronize.  yod replies that
** the barrier is done when it has received the request
** from each job.  When yod has replied to each job once
** that the barrier done, it forgets about the barrier.
**
** If the return value of the barrier request call indicates
** that the barrier is not yet done, the status function
** should be called to determine when the barrier has completed.
** Like this:
**
**   rc = CplantInterjobBarrierGrp(otherJob);
**
**   if (rc == 0){    ** synch operation is in progress **
**       while (1){
**           rc = CplantBarrierStatusGrp(otherJob);
**
**           if (rc == 1) break; ** parent/child are synch'ed **
**           if (rc == 0) continue;
**           if (rc == -1) errorHandler();
**       }
**   }
**   else if (rc == 1){ 
**       ** parent & child are synch'ed **
**   }
**   else if (rc == -1){
**       errorHandler();
**   }
**
** Returns 1 if synchronization is done,  (SYNC_COMPLETED)
**         0 if it is in progress,        (SYNC_IN_PROGRESS)
**        -1 if there is an error.        (SYNC_ERROR)
*/
int
CplantInterjobBarrierGrp(jobFamilyInfo *otherJob,
                     int nMembers, int *rankList, int tmout)
{
int rc, iamRoot, status;

    if (nMembers == 0) rankList = NULL;
    else if (rankList == NULL) nMembers = 0;

    iamRoot =
      collectiveCallCheck(nMembers, rankList, 
          "CplantInterjobBarrierGrp");

    if (iamRoot == -1) return SYNC_ERROR;

    if (iamRoot){
        status = CplantInterjobBarrier(otherJob);
    }

    rc = dsrvr_bcast((void *)&status, sizeof(int),
                       tmout, CMD_SYNCHRONIZE_JOBS, rankList, nMembers);

    if (rc != DSRVR_OK){
        status = SYNC_ERROR;
    }

    return status;
}

int
CplantInterjobBarrier(jobFamilyInfo *otherJob) 
{
#undef CMD
#undef ACK
#define CMD     (cmd.info.jobCmd)
#define ACK     (ack.info.jobSynchAck)

int rc;
hostCmd_t cmd;
hostReply_t ack;
int status;

    status = 0;

    CMD.yodHandle = otherJob->yodHandle;

    rc = _host_rpc(&cmd, &ack, CMD_SYNCHRONIZE_JOBS, 
                   YOD_NID, YOD_PID, YOD_PTL,
                   NULL, 0, NULL, 0);

    if ((rc != 0) || (ack.hostErrno)){
        status = SYNC_ERROR;
    }
    else{
        status = ACK.status;
    }
    
    return status;
}
/*
** Returns 1 if synchronization is done,  (SYNC_COMPLETED)
**         0 if it is in progress,        (SYNC_IN_PROGRESS)
**        -1 if there is an error.        (SYNC_ERROR)
*/
int
CplantBarrierStatusGrp(jobFamilyInfo *otherJob,
                          int nMembers, int *rankList, int tmout)
{
int rc, iamRoot, status;

    if (nMembers == 0) rankList = NULL;
    else if (rankList == NULL) nMembers = 0;

    iamRoot =
      collectiveCallCheck(nMembers, rankList, 
          "CplantInterjobBarrierStatusGrp");

    if (iamRoot == -1) return SYNC_ERROR;

    if (iamRoot){
        status = CplantBarrierStatus(otherJob);
    }

    rc = dsrvr_bcast((void *)&status, sizeof(int),
                       tmout, CMD_SYNCHRONIZE_STATUS, rankList, nMembers);

    if (rc != DSRVR_OK){
        status = SYNC_ERROR;
    }

    return status;
}

int
CplantBarrierStatus(jobFamilyInfo *otherJob) 
{
#undef CMD
#undef ACK
#define CMD     (cmd.info.jobCmd)
#define ACK     (ack.info.jobSynchAck)

int rc;
hostCmd_t cmd;
hostReply_t ack;
int status;

    status = 0;

    CMD.yodHandle = otherJob->yodHandle;

    rc = _host_rpc(&cmd, &ack, CMD_SYNCHRONIZE_STATUS, 
                   YOD_NID, YOD_PID, YOD_PTL,
                   NULL, 0, NULL, 0);

    if ((rc != 0) || (ack.hostErrno)){
        status = SYNC_ERROR;
    }
    else{
        status = ACK.status;
    }
    
    return status;
}
/*
** Send a signal to a Cplant job in your family
*/
int
CplantSignalJob(int sig, jobFamilyInfo *job)
{
#undef CMD
#undef ACK
#define CMD     (cmd.info.jobCmd)
#define ACK     (ack.info.jobStatusAck)

int rc;
hostCmd_t cmd;
hostReply_t ack;

    CMD.yodHandle = job->yodHandle;
    CMD.sigNum    = sig;

    rc = _host_rpc(&cmd, &ack, CMD_JOB_SIGNAL, 
                   YOD_NID, YOD_PID, YOD_PTL,
                   NULL, 0, NULL, 0);

    if (rc != 0){
       return -1;
    }

    return ACK.retVal;
}
/*
** If you're a PBS job, yod knows how many nodes you have allocated 
** but are unused by this yod session.  Note your PBS script could
** have started other jobs - yod doesn't know about those.
*/
int
CplantNodesRemaining()
{
#undef CMD
#undef ACK
#define CMD     (cmd.info.jobCmd)
#define ACK     (ack.info.jobStatusAck)

int rc;
hostCmd_t cmd;
hostReply_t ack;
int nnodes;

    if (_my_PBS_ID == INVAL) return -1;

    rc = _host_rpc(&cmd, &ack, CMD_NODES_REMAINING, 
                   YOD_NID, YOD_PID, YOD_PTL,
                   NULL, 0, NULL, 0);

    if ((rc != 0) || (ack.hostErrno)){
        nnodes = -1;
    }
    else{
        nnodes = ACK.retVal;
    }
    
    return nnodes;
}
/*
** Get the termination info for a job where some or all of the
** processes have terminated.  Request any of the following:
**
**   o  list of exit codes 
**
**   o  list of terminating signals
**
**   o  list of terminators if app was killed
**            PCT_NO_TERMINATOR
**            PCT_JOB_OWNER
**            PCT_ADMINISTRATOR
**
**   o  char array telling which processes have terminated
**
** Return value on success is the number of processes in the
** job that have terminated.  Return on error is -1.
*/
int 
CplantFamilyTerminationGrp(jobFamilyInfo *job,
                     int *exitCode, int *termSig, int *terminator, int *done,
                     int len,
                     int nMembers, int *rankList, int tmout)
{
int rc1, rc2, rc3, rc4, ndone, iamRoot, maplen;

    if (nMembers == 0) rankList = NULL;
    else if (rankList == NULL) nMembers = 0;

    iamRoot = collectiveCallCheck(nMembers, rankList, 
                         "CplantJobTermination");

    if (iamRoot == -1) return -1;

    if (iamRoot){

        ndone = CplantFamilyTermination(job, exitCode, termSig, terminator, done, len);

    }

    rc1 = dsrvr_bcast((char *)&ndone, sizeof(int), tmout,
                           CMD_JOB_STATUS, rankList, nMembers);

    if (rc1 != DSRVR_OK){
        return -1; 
    }

    if (ndone < 0){
        return ndone;
    }

    maplen = job->nprocs * sizeof(int);

    if (exitCode){
        rc1 = dsrvr_bcast((char *)exitCode, maplen,
                  tmout, CMD_JOB_STATUS, rankList, nMembers);
    }
    else{
        rc1 = DSRVR_OK;
    }

    if (termSig){
        rc2 = dsrvr_bcast((char *)termSig, maplen,
                  tmout, CMD_JOB_STATUS, rankList, nMembers);
    }
    else{
        rc2 = DSRVR_OK;
    }

    if (terminator){
        rc3 = dsrvr_bcast((char *)terminator, maplen,
                  tmout, CMD_JOB_STATUS, rankList, nMembers);
    }
    else{
        rc3 = DSRVR_OK;
    }

    if (done){
        rc4 = dsrvr_bcast((char *)done, maplen,
                  tmout, CMD_JOB_STATUS, rankList, nMembers);
    }
    else{
        rc4 = DSRVR_OK;
    }

    if ((rc1 != DSRVR_OK) || (rc2 != DSRVR_OK) || 
        (rc3 != DSRVR_OK) || (rc4 != DSRVR_OK)    ){

        return -1;
    }
    
    return ndone;
}
int 
CplantFamilyTermination(jobFamilyInfo *job, 
                     int *exitCode, int *termSig, int *terminator, int *done,
                     int len)
{
#undef CMD
#undef ACK
#define CMD     (cmd.info.jobCmd)
#define ACK     (ack.info.jobStatusAck)

int buflen, rc, ndone, itsdone, i;
hostCmd_t cmd;
hostReply_t ack;
final_status *buf;

    if ((job->nprocs < 1) || (len < job->nprocs)) return -1;

    buflen = sizeof(final_status) * job->nprocs;

    buf = (final_status *)malloc(buflen);
    
    if (!buf){
       return -1;
    }

    CMD.yodHandle = job->yodHandle;

    rc = _host_rpc(&cmd, &ack, CMD_JOB_TERMINATION_STATUS, 
                   YOD_NID, YOD_PID, YOD_PTL,
                   NULL, 0, (char *)buf, buflen);

    if ((rc != 0) || (ack.hostErrno)){
        return -1;
    }

    ndone = 0;

    for (i=0; i<job->nprocs; i++){

        itsdone = (buf[i].terminator != PCT_TERMINATOR_UNSET);

        if (exitCode) exitCode[i]     = buf[i].exit_code;
        if (termSig) termSig[i]       = buf[i].term_sig;
        if (terminator) terminator[i] = buf[i].terminator;

        if (done){
            if (itsdone) done[i] = 1;
            else         done[i] = 0;
        }

        if (itsdone) ndone++;
    } 
    
    return ndone;
}
/*
**  Set the nid and pid map for a job in my family.
**    Gets the info from yod, and it's in process
**    rank order.
*/
int 
CplantFamilyMapGrp(jobFamilyInfo *job,
                     int *nmap, int *pmap, int len,
                     int nMembers, int *rankList, int tmout)
{
int rc1, rc2, iamRoot, maplen;

    if (nMembers == 0) rankList = NULL;
    else if (rankList == NULL) nMembers = 0;

    iamRoot = collectiveCallCheck(nMembers, rankList, 
                         "CplantFamilyMapGrp");

    if (iamRoot == -1) return -1;

    if (iamRoot){

        rc1 = CplantFamilyMap(job, nmap, pmap, len);

        if (rc1){
            nmap[0] = INVAL;
            pmap[0] = INVAL;
        }
    }
    maplen = job->nprocs * sizeof(int);

    rc1 = dsrvr_bcast((char *)nmap, maplen,
                  tmout, CMD_JOB_STATUS, rankList, nMembers);

    rc2 = dsrvr_bcast((char *)pmap, maplen,
                  tmout, CMD_JOB_STATUS, rankList, nMembers);

    if ((rc1 != DSRVR_OK) || (nmap[0] == INVAL) ||
        (rc2 != DSRVR_OK) || (pmap[0] == INVAL)    ){

        return -1;
    }
    
    return 0;
}
int
CplantFamilyMap(jobFamilyInfo *job,
                     int *nmap, int *pmap, int len)
{
#undef CMD
#undef ACK
#define CMD     (cmd.info.jobCmd)
#define ACK     (ack.info.jobStatusAck)

int maplen, rc;
hostCmd_t cmd;
hostReply_t ack;

    if ((job->nprocs < 1) || (len < job->nprocs)) return -1;

    maplen = len * sizeof(int);

    CMD.yodHandle = job->yodHandle;

    rc = _host_rpc(&cmd, &ack, CMD_JOB_NID_MAP, 
                   YOD_NID, YOD_PID, YOD_PTL,
                   NULL, 0, (char *)nmap, maplen);

    if ((rc != 0) || (ack.hostErrno)){
        return -1;
    }

    rc = _host_rpc(&cmd, &ack, CMD_JOB_PID_MAP, 
                   YOD_NID, YOD_PID, YOD_PTL,
                   NULL, 0, (char *)pmap, maplen);

    if ((rc != 0) || (ack.hostErrno)){
        return -1;
    }

    job->status       = ACK.progress;
    
    return 0;
}

static int
collectiveCallCheck(int nMembers, int *rankList, char *nm)
{
int iamRoot;

    if ((nMembers && (rankList[0] == _my_rank)) ||
        (_my_rank == 0)){

       iamRoot = 1;
    }
    else{
       iamRoot = 0;
    }

    if (!dsrvrMembersInited){

        if (iamRoot){
            fprintf(stderr,
            "%s failure:\n",nm);
            fprintf(stderr,
            "CplantInitCollective must be called by all processes initially.\n");
        }
        return -1;
    }

    return iamRoot;

}
