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
** $Id: job_service.c,v 1.2 2001/11/17 01:00:07 lafisk Exp $
**
*/

#include "srvr_comm.h"
#include "srvr_err.h"
#include "job_private.h"
#include "CMDhandlerTable.h"
#include "util.h"

static int _process_pct_done_message(yod_job *job, 
             control_msg_handle *mh);

static yod_job *currentIOjob;

/****************************************************
** PCT messages
**
** Check for a message from PCT.  If found, process
** it (library saves completion info and gathers
** back traces).
**
** Set message type, nid, rank and message handle
** in case caller wants all that.  
**
** Return 1 if a message was found, 0 otherwise.
**
** If caller did not provide mh, then free the message,
** else caller frees it later.
*/
int
job_get_pct_message(int jobHandle,
                     int *mtype, int *nid, int *rank,
                     control_msg_handle *mh)
{
int ptl, type, rc;
control_msg_handle mymh;
yod_job *job;

    _job_in("job_get_pct_message");

    job = _getJob(jobHandle);

    if (!job){
       _job_error_info("invalid job handle %d\n",jobHandle);
        JOB_RETURN_ERROR;
    }

    ptl = job->pctPtl;

    SRVR_CLEAR_HANDLE(mymh);

    rc = srvr_get_next_control_msg(ptl, &mymh, &type, NULL, NULL);

    if (rc == 0) JOB_RETURN_OK;

    if (rc == -1){
       _job_error_info("error awaiting pct message (%s)\n",
                               CPstrerror(CPerrno));
        JOB_RETURN_ERROR;
    }

    if (DBG_FLAGS(DBG_LOAD_1)){
       _jobMsg("Got message type (%s) from pct %d/%d\n",
        select_pct_to_yod_string(type), 
        SRVR_HANDLE_NID(mymh), SRVR_HANDLE_PID(mymh));
    }

    if (mtype) *mtype = type;

    if (nid)   *nid   = SRVR_HANDLE_NID(mymh);
    
    if (rank)  *rank  = _nid2rank(job, *nid);

    if (mh){
        memcpy((void *)mh, (void *)&mymh, sizeof(control_msg_handle));
    }

    if (type == PROC_DONE_MSG){
       _process_pct_done_message(job, &mymh);
    }
    else if (type == LAUNCH_FAILURE_MSG){
       /*
       ** there's a small window after we think job
       ** has successfully loaded where load can fail.
       */
       _job_launch_failed(job,
          (launch_failure *)SRVR_HANDLE_USERDEF(mymh));
    }

    if ((job->proc_done_count + job->failCount) == job->nnodes){

        job->jobStatus |= JOB_APP_FINISHED;

        _job_mkdate(time(NULL), &(job->endTime));

        _job_send_pct_control_message(job->pctNidMap[0],
           MSG_ALL_DONE, (char *)&(job->job_id), sizeof(int));

        _job_log_done(job);

        _job_remove_executables(job);
    }

    if (!mh){
       srvr_free_control_msg(ptl, &mymh);
    }

    JOB_RETURN_VAL(1);
}
int
job_free_pct_message(int jobHandle, control_msg_handle *mh)
{
yod_job *job;

    _job_in("job_free_pct_message");

    job = _getJob(jobHandle);

    if (!job){
       _job_error_info("bad job handle %d\n",jobHandle);
        JOB_RETURN_ERROR;
    }

    srvr_free_control_msg(job->pctPtl, mh);

    JOB_RETURN_OK;
}

/*
** save a PCT's PROC_DONE_MSG for later reporting
*/
static int
_process_pct_done_message(yod_job *job, control_msg_handle *mh)
{
app_proc_done *dmsg;
int rank, term_sig;

    _job_in("_process_pct_done_message");

    dmsg = (app_proc_done *)SRVR_HANDLE_USERDEF(*mh);

    rank = _nid2rank(job, dmsg->nid);

    if (rank == -1){
       _job_error_info("invalid done message");
        JOB_RETURN_ERROR;
    }

    if (job->done_status == NULL){

        job->done_status = 
         (app_proc_done *)calloc(job->nnodes , sizeof(app_proc_done));

        if (!job->done_status){
           _job_error_info("out of memory");
            JOB_RETURN_ERROR;
        }
    }

    memcpy(job->done_status + rank, dmsg, sizeof(app_proc_done));

    job->proc_done_count++;

    if (DBG_FLAGS(DBG_PROGRESS)){
        _jobMsg("Done message from %d\n",dmsg->nid);
        _jobMsg("    job id %d, nid %d pid %d\n",
               dmsg->job_id, dmsg->nid, dmsg->pid);
        _jobMsg("    exit_code %d elapsed %d status %x bt_size %d\n",
               dmsg->final.exit_code, dmsg->elapsed,
               dmsg->status, dmsg->bt_size);
    }

    if (jobOptions.get_bt && (dmsg->bt_size > 0)){

        if (!job->backTraces){
            job->backTraces = (char **)calloc(sizeof(char *) , job->nnodes);

            if (!job->backTraces){
               _job_error_info("out of memory");
                JOB_RETURN_ERROR;
            }
        }
        job->backTraces[rank] =
          _job_get_stack_trace(job, rank, daemonWaitLimit);
    }

    term_sig = dmsg->final.term_sig;

    if (term_sig){
        job->terminator = dmsg->final.terminator;
    }


    JOB_RETURN_OK;
}

/***********************************************
** Application messages
**
** Check for a message from the application.  
**
** If it's an IO request, handle it.  (We created
** an option to let the caller handle IO instead.)
**
** If message was found, set message type, nid, 
** rank and message handle.  
**
** Return 1 if a message was found, 0 otherwise.
**
** If caller did not provide mh, then free the message,
** else caller frees it later.
*/
int
job_get_app_message(int jobHandle,
                     int *mtype, int *nid, int *rank,
                     control_msg_handle *mh)
{
int ptl, rc, srcnid, offset, type, msgtype;
control_msg_handle mymh;
hostCmd_t *rmsg;
yod_job *job;

    _job_in("job_get_app_message");

    job = _getJob(jobHandle);

    if (!job){
       _job_error_info("invalid job handle %d\n",jobHandle);
        JOB_RETURN_ERROR;
    }

    ptl = job->appPtl;

    SRVR_CLEAR_HANDLE(mymh);

    rc = srvr_get_next_control_msg(ptl, &mymh, &msgtype, NULL, NULL);

    if (rc == 0) JOB_RETURN_OK;

    if (rc == -1){
       _job_error_info("error awaiting app message (%s)\n",
                               CPstrerror(CPerrno));
        JOB_RETURN_ERROR;
    }

    srcnid = SRVR_HANDLE_NID(mymh);

    rmsg = (hostCmd_t *)SRVR_HANDLE_USERDEF(mymh);

    type = rmsg->type;

    if (mtype) *mtype = msgtype;

    if (nid)   *nid   = srcnid;
    
    if (rank)  *rank  = _nid2rank(job, srcnid);

    if (rank == -1){
       _job_error_info("invalid source node %d\n",srcnid);
        JOB_RETURN_ERROR;
    }

    if (mh){
        memcpy((void *)mh, (void *)&mymh, sizeof(control_msg_handle));
    }

    if ((msgtype == YO_ITS_IO) && (!jobOptions.IdoIO)){

        if ( (type >= FIRST_CMD_NUM) &&
             (type <= LAST_CMD_NUM)     ){

            offset = type - FIRST_CMD_NUM;

            if (DBG_FLAGS(DBG_IO_2)){

                _jobMsg(
                 "req type -0x%x (%s) from nid %d rank %d\n",
                  -(type), CMDstrings[offset],
                  srcnid, _nid2rank(job, srcnid));
             }
             ioErrno = 0;

             currentIOjob = job;
    
             host_cmd_handler_tbl[offset](&mymh);

             if (ioErrno){
                 if (job->ioWarnings <= 5){
                     _jobErrorMsg(
                         "IO Error - %s (node %d, rank %d job %d): %s\n",
                             CMDstrings[offset],
                             srcnid, _nid2rank(job, srcnid), job->job_id,
                             CPstrerror(ioErrno));
    
                     if (job->ioWarnings == 5){
                         _jobErrorMsg(
                      "Further IO error messages suppressed.\n");
                     }
                     job->ioWarnings++;
                 }
            }
        }
    }

    if (!mh){
       srvr_free_control_msg(ptl, &mymh);
    }

    JOB_RETURN_VAL(1);
}
int
job_free_app_message(int jobHandle, control_msg_handle *mh)
{
yod_job *job;

    _job_in("job_free_app_message");

    job = _getJob(jobHandle);

    if (!job){
       _job_error_info("bad job handle %d\n",jobHandle);
        JOB_RETURN_ERROR;
    }

    srvr_free_control_msg(job->appPtl, mh);

    JOB_RETURN_OK;
}

/***********************************************
** 
**  requests to PCT
*/
int
job_send_signal(int handle, int sig)
{
yod_job *job;
send_sig sigMsg;

    _job_in("job_send_signal");

    job = _getJob(handle);

    if (!job){
       _job_error_info("bad job handle %d\n",handle);
        JOB_RETURN_ERROR;
    }

    switch (sig){

       case SIGUSR1:
       case SIGUSR2:

           sigMsg.job_ID = job->job_id;
           sigMsg.type   = sig;

           _job_send_pct_control_message(job->pctNidMap[0],
                  MSG_SEND_SIGUSR, (char *)&sigMsg, sizeof(send_sig));

           break;

       case SIGTERM:

           _job_send_pct_control_message(job->pctNidMap[0],
                  MSG_ABORT_LOAD_1, (char *)&(job->job_id), sizeof(int));

           break;

       case SIGKILL:

           _job_send_pct_control_message(job->pctNidMap[0],
                  MSG_ABORT_LOAD_2, (char *)&(job->job_id), sizeof(int));

           break;

       default:

          _job_error_info("PCT does not accept signal %s\n",
                     select_signal_name(sig));
           JOB_RETURN_ERROR;

           break;
    }
    JOB_RETURN_OK;
}
int
job_nodes_reset(int handle)
{
yod_job *job;
int rc;

    _job_in("job_nodes_reset");

    job = _getJob(handle);

    if (!job){
       _job_error_info("bad job handle %d\n",handle);
        JOB_RETURN_ERROR;
    }

    rc = _job_send_all_pcts_control_message(job, 
                MSG_ABORT_RESET, (char *)&job->job_id, sizeof(int));

    if (!job->endTime){
        _job_mkdate(time(NULL), &(job->endTime));
        job->jobStatus |= JOB_APP_FINISHED;

        _job_log_done(job);
    }

    JOB_RETURN_VAL(rc);
}
void
CMDhandler_mass_murder(control_msg_handle *mh)
{
int srcnid, rank;
yod_job *job;

      job = currentIOjob;

      if ((job->jobStatus & JOB_APP_STARTED) &&
          !(job->jobStatus & JOB_APP_FINISHED) && 
          !(job->jobStatus & JOB_APP_MASS_MURDER)  ){

          srcnid = SRVR_HANDLE_NID(*mh);
          rank = _nid2rank(job, srcnid);

          _job_send_pct_control_message(job->pctNidMap[0],
               MSG_ABORT_LOAD_2, (char *)&(job->job_id), sizeof(int));

          _jobMsg(
          "Job %d, rank %d on node %d requested that application be killed.\n",
                 job->job_id, rank, srcnid); 
          _jobMsg(
          "Here we go.  Waiting for completion messages to come in.\n\n");

          job->jobStatus |= JOB_APP_MASS_MURDER;
      }
      else if (DBG_FLAGS(DBG_IO_2)){
           _jobMsg("mass murder request ignored\n");
      }
}
void
CMDhandler_heartbeat(control_msg_handle *mh)
{
       return;
}
/*
** This gets called by CMDhandler_open()
*/
int
get_app_srvr_portal()
{
   return currentIOjob->appPtl;
}

