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
** $Id: job_comm.c,v 1.1 2001/11/04 20:22:39 lafisk Exp $
**
**  building/using portals to bebopd and pct
**    setting up portals for app communication
*/
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include "bebopd.h"
#include "config.h"
#include "ppid.h"
#include "portal_assignments.h"
#include "srvr_comm.h"
#include "srvr_err.h"
#include "cplant_host.h"
#include "job_private.h"
#include "pct_ports.h"

static int initCommDone=0;
extern char* _cplant_link_version;

int
_initComm()
{
#ifdef USE_DB
char *nid;
#else
int count;
#endif
int rc;

    _job_in("_initComm");

    if (initCommDone) JOB_RETURN_OK;

    _my_ppid = register_ppid(&_my_taskInfo, PPID_AUTO, GID_YOD, "yod");

    if ( _my_ppid == 0 )
    {
        _job_error_info( "Can not register myself with PPID=%d\n", PPID_AUTO);
        JOB_RETURN_ERROR; 
    }
    rc = server_library_init();

    CLEAR_ERR;

    /*
    ** We need a control portal for the bebopd get and put
    ** messages.
    */
    _myEnv.bebopdPtl = srvr_init_control_ptl(2);

    if (_myEnv.bebopdPtl == SRVR_INVAL_PTL){
        _job_error_info( "Can't create portal for incoming bebopd messages (%s)\n",
                            CPstrerror(CPerrno));
        JOB_RETURN_ERROR;
    }
#ifdef USE_DB
    nid = getdb("BEBOPD");

    if (!nid){
        _job_error_info( "Can't get bebopd nid from data base\n");
        JOB_RETURN_ERROR;
    }
    _myEnv.bnid = (int) strtol( nid, NULL, 10 );

    if ((_myEnv.bnid < 0) || (_myEnv.bnid > MAX_NODES)){
        _job_error_info( "bebopd nid from data base is invalid\n");
        JOB_RETURN_ERROR;
    }
#else
    if ( cplantHost_getNid("bebopd", 1, &(_myEnv.bnid), &count) != OK )
    {
        _job_error_info( "Can not get nid from cplant-host file\n");
        JOB_RETURN_ERROR;
    }

    if ( count != 1 ){
        _job_error_info( "expected one bebopd entry; got %d\n", count);
        JOB_RETURN_ERROR;
    }

#endif
    _myEnv.bptl = REQUEST_PTL;
    _myEnv.bpid = PPID_BEBOPD;

    rc = initialize_work_bufs();

    if (rc){
        _job_error_info("Can't make work buffers for IO (%s)\n",
                       CPstrerror(CPerrno));
        JOB_RETURN_ERROR;
    }

    initCommDone = 1;

    if (DBG_FLAGS(DBG_COMM)){

        _jobMsg("_my_ppid %d, bebopd control portal %d, bebopd ID %d/%d/%d\n",
	     _my_ppid,
	     _myEnv.bebopdPtl,
	     _myEnv.bnid, _myEnv.bpid, _myEnv.bptl);

    }


    JOB_RETURN_OK;
}

/********************************************************************************/
/*   bebopd communication                                                       */
/********************************************************************************/
static int
physnidOrder(const void *n1, const void *n2)
{
int rc;
nidOrder *no1, *no2;

    no1 = (nidOrder *)n1;
    no2 = (nidOrder *)n2;

    if (no1->nid < no2->nid) rc = -1;
    else if (no1->nid > no2->nid) rc = 1;
    else rc = 0;

    return rc;
}
int
_await_bebopd_reply(yod_job *yjob)
{
control_msg_handle mhandle;
int comm_rc, rc, msg_type;
time_t t1;
int await_status, i;
char *reply;

    _job_in("await_bebopd_reply");

    await_status = BEBOPD_OK;

    t1 = time(NULL);

    while (1){
        SRVR_CLEAR_HANDLE(mhandle);

        comm_rc = srvr_get_next_control_msg(_myEnv.bebopdPtl, 
                  &mhandle, &msg_type, NULL, &reply);

        if (comm_rc == 1) break;

        if (comm_rc < 0){
            _job_error_info("(%s) - getting bebopd notice of pct list\n",
                         CPstrerror(CPerrno));
            await_status = -1;
            break;
        }

        if ((time(NULL) - t1) > _myEnv.daemonWaitLimit){
            _job_error_info("No reponse with pct list from bebopd, sorry\n");
            await_status = -1;
            break;
        }
    }
    memcpy((void *)&(yjob->reply), (void *)reply, sizeof(bebopd_status));

    if (comm_rc == 1){
        if (yjob->reply.rc != BEBOPD_OK){   /* bebopd can't fulfill the request */
            await_status = ((bebopd_status *)reply)->rc;
        }
        else if (msg_type != BEBOPD_PUT_PCT_LIST){
            _job_error_info("unexpected message arrived on bebopd ptl 0x%x\n",
                             msg_type);
            await_status = -1;
        }
    }

    if (await_status != BEBOPD_OK){
        srvr_free_control_msg(_myEnv.bebopdPtl, &mhandle);
	JOB_RETURN_VAL(await_status);
    }

    yjob->job_id = yjob->reply.job_id;

    yjob->nnodes = SRVR_HANDLE_TRANSFER_LEN(mhandle) / sizeof(int);

    yjob->pctNidMap   = (int *)malloc(yjob->nnodes * sizeof(int));
    yjob->nidOrderMap = (nidOrder *)malloc(yjob->nnodes * sizeof(nidOrder));

    if (!yjob->pctNidMap || !yjob->nidOrderMap){
	_job_error_info("out of memory\n");
	JOB_RETURN_ERROR;
    }

    rc = srvr_comm_put_reply(&mhandle, (void *)(yjob->pctNidMap),
				SRVR_HANDLE_TRANSFER_LEN(mhandle));

    if (rc < 0){
	_job_error_info("(%s) - receiving pct list from bebopd\n",
		     CPstrerror(CPerrno));


	free(yjob->pctNidMap);
	free(yjob->nidOrderMap);

	yjob->pctNidMap = NULL;
        yjob->nidOrderMap = NULL;

	srvr_free_control_msg(_myEnv.bebopdPtl, &mhandle);

	JOB_RETURN_ERROR;
    }

    /*
    ** we also need a list of nid/rank in phys nid order so we
    ** can do a nid to rank mapping
    */
    for (i=0; i<yjob->nnodes; i++){
	yjob->nidOrderMap[i].nid = yjob->pctNidMap[i];
	yjob->nidOrderMap[i].rank = i;
    }
    if (yjob->nnodes > 1){
	qsort((void *)(yjob->nidOrderMap), yjob->nnodes, sizeof(nidOrder), 
		physnidOrder);
    }

    srvr_free_control_msg(_myEnv.bebopdPtl, &mhandle);

    JOB_RETURN_VAL(await_status);
}
/*
** Returns:   0 - OK
**           -1 - error occured in await_bebopd_get
**           >0 - reason the bebopd can't allocate the nodes
**
** We have a buffer to send to bebopd.  We sent bebopd a control
** message telling him to come get it at his leisure.  We want yod
** waiting around for bebopd, not the other way around.  So this
** routine awaits the bebopd's get request and then replies with
** the buffer.
*/
int
_await_bebopd_get(char *buf, int len, int mtype)
{
int rc, comm_rc, msg_type, send_status;
control_msg_handle mhandle;
bebopd_status *status;
time_t t1;

    _job_in("_await_bebopd_get");

    send_status = BEBOPD_OK;

    t1 = time(NULL);

    while (1){

        SRVR_CLEAR_HANDLE(mhandle);

        comm_rc = srvr_get_next_control_msg(_myEnv.bebopdPtl, &mhandle, &msg_type, NULL,
                                  (char **)&(status));

        if (comm_rc == 1) break;   /* got a message */

        if (comm_rc < 0){
            _job_error_info("(%s) - getting bebopd request for node list\n",
                         CPstrerror(CPerrno));
            send_status = -1;
            break;
        }

        if ((time(NULL) - t1) > _myEnv.daemonWaitLimit){
            _job_error_info("No reponse from bebopd\n");
            send_status = -1;
            break;
        }
    }
    if (comm_rc == 1){
        if (status->rc |= BEBOPD_OK){
            send_status = status->rc;  /* bebopd can't allocate nodes */
        }
        else if (msg_type != mtype){
            _job_error_info("Unexpected message on bebopd portal\n");
            send_status = -1;
        }
    }

    if (send_status == BEBOPD_OK){

        rc = srvr_comm_get_reply(&mhandle, (void *)buf, len);

        if (rc < 0){
            _job_error_info("(%s) - error sending to bebopd\n",
                             CPstrerror(CPerrno));
            send_status = -1;
        }
    }

    if (comm_rc == 1){
        srvr_free_control_msg(_myEnv.bebopdPtl, &mhandle);
    }

    JOB_RETURN_VAL(send_status);
}
/*
** send control message to bebopd initiating bebopd get request,
**  await the get request and send off the data
*/
int
_send_to_bebopd(char *buf, int len,   /* buffer for bebopd to come get */
               int sendtype,         /* msg type for send to bebopd   */
               int gettype)          /* msg type for bebopd get request */
{
yod_request req;
int rc;

    _job_in("_send_to_bebopd");

    if (_myEnv.bptl == SRVR_INVAL_PTL){
        JOB_RETURN_ERROR;
    }

    memset(&req, 0, sizeof(yod_request));

    req.myptl = _myEnv.bebopdPtl;
    req.nnodes = len;

    rc = srvr_send_to_control_ptl(_myEnv.bnid, _myEnv.bpid, _myEnv.bptl,
                 sendtype, (char *)&req, sizeof(yod_request));

    if (rc < 0){
        _job_error_info("(%s) - sending %d message to bebopd\n",
                        CPstrerror(CPerrno), sendtype);
        JOB_RETURN_ERROR;
    }

    rc = _await_bebopd_get(buf, len, gettype);

    if (rc < 0){
        JOB_RETURN_ERROR;
    }

    JOB_RETURN_OK;
}

/*
** bebopd logs user activity for us
*/
/*
** required for Tflops style logging
*/
static char log_record[USERLOG_MAX];
static char nlist[USERLOG_MAX];
/*
**
*/

void
_job_log_start(yod_job *yjob)
{
char *args;
struct passwd *pw;

    _job_in("_job_log_start");

    pw = getpwuid(_myEnv.euid);

    if (pw){
        yjob->userName = strdup(pw->pw_name);
    }
    else{
        yjob->userName = strdup("unknown");
    }

    /*
    ** argstr contains null-separted args, and is terminated
    ** with two null bytes.
    */
    args = yjob->Members[0].argstr;
    log_record[0] = 0;

    while (*args){

        strcat(log_record, args); strcat(log_record, " ");
        while (*args++); 

        if (*args && 
            ((strlen(log_record) + strlen(args) + 1) >= USERLOG_MAX)){
            break;
        }
    }

    yjob->cmdLine = strdup(log_record);

    write_node_list(yjob->pctNidMap, yjob->nnodes, nlist, USERLOG_MAX);

    _job_mkdate(time(NULL), &(yjob->startTime));

    snprintf(log_record, USERLOG_MAX-1,
        ">> Starting Cplant user %s | base %d | start %s | "
        "cmdLine %s | nnodes %d | Nodes %s",
           yjob->userName, yjob->pctNidMap[0], yjob->startTime,
           yjob->cmdLine, yjob->nnodes, nlist);

    _send_to_bebopd(log_record, strlen(log_record)+1,
            YOD_USERLOG_RECORD, BEBOPD_GET_USERLOG_RECORD);

    yjob->start_logged = 1;

    JOB_RETURN;
}
void
_job_log_done(yod_job *yjob)
{
    _job_in("_job_log_done");

    if (!yjob->start_logged) JOB_RETURN;

    if (yjob->terminator == PCT_NO_TERMINATOR){

        snprintf(log_record,USERLOG_MAX-1,
		">> Finished Cplant user %s | base %d | start %s | "
		"end %s | cmdLine %s | nnodes %d | Nodes",
               yjob->userName, yjob->pctNidMap[0], yjob->startTime,
	   yjob->endTime, yjob->cmdLine, yjob->nnodes);
    }
    else if (yjob->terminator == PCT_JOB_OWNER){

	    snprintf(log_record,USERLOG_MAX-1,
	       ">> Killed Cplant user %s | base %d | start %s | "
	       "end %s | cmdLine %s | nnodes %d | Nodes",
               yjob->userName, yjob->pctNidMap[0], yjob->startTime,
	       yjob->endTime, yjob->cmdLine, yjob->nnodes);
    }
    else if (yjob->terminator == PCT_ADMINISTRATOR){

	   snprintf(log_record,USERLOG_MAX-1,
	       ">> Aborted Cplant user %s | base %d | time %s | cmdLine %s",
               yjob->userName, yjob->pctNidMap[0], yjob->endTime,
	       yjob->cmdLine);
    }
    _send_to_bebopd(log_record, strlen(log_record)+1,
	    YOD_USERLOG_RECORD, BEBOPD_GET_USERLOG_RECORD);

    yjob->end_logged = 1;

    JOB_RETURN;
}
static char *months[12]={"Jan", "Feb", "Mar",
                         "Apr", "May", "Jun",
                         "Jul", "Aug", "Sep",
                         "Oct", "Nov", "Dec"};
void
_job_mkdate(time_t t1, char **buf)
{
struct tm *t2;
char dstr[32];

    _job_in("_job_mkdate");

    t2 = localtime(&t1);

    sprintf(dstr, "%s %d, %d, %02d:%02d:%02d",
      months[t2->tm_mon], t2->tm_mday, 1900+t2->tm_year,
      t2->tm_hour, t2->tm_min, t2->tm_sec);

    *buf = strdup(dstr);

    JOB_RETURN;
}



/********************************************************************************/
/*   pct communication                                                          */
/********************************************************************************/

static int wait_pcts_put_msg(int bufnum, int tmout, int howmany);

/*
** Send control message to each pct.
*/
int
_job_send_pct_control_message(int pctNid, int msg_type, 
                          char *buf, int len)
{
int rc;

    _job_in("_job_send_pct_control_message"); 

    if (DBG_FLAGS(DBG_LOAD_1)){
        _jobMsg("    send control message type %x to PCT %d/%d/%d\n",
	      msg_type, pctNid, PPID_PCT, PCT_LOAD_PORTAL);
    }

    rc = srvr_send_to_control_ptl(pctNid, PPID_PCT, PCT_LOAD_PORTAL,
             msg_type, buf, len);

    if (rc){
        _job_error_info("sending control message to physical node %d (%s)\n",
              pctNid, CPstrerror(CPerrno));
        JOB_RETURN_ERROR;
    }

    JOB_RETURN_OK;
}
int
_job_send_all_pcts_control_message(yod_job *yjob, int msg_type, 
                          char *buf, int len)
{
int i, rc;
int *thePcts;

    _job_in("_job_send_all_pcts_control_message"); 

    if (yjob->nnodes < 1){
        _job_error_info("no pcts allocated yet\n");
        JOB_RETURN_ERROR;
    }

    thePcts = yjob->pctNidMap;

    for (i=0; i < yjob->nnodes ; i++){

        rc = _job_send_pct_control_message(thePcts[i], msg_type, buf, len);

        if (rc){
            JOB_RETURN_ERROR;
        }
    }

    JOB_RETURN_OK;
}
/*
**  Request a stack trace from one pct, return pointer to the stack
**  trace, which should be freed by caller
*/
char *
_job_get_stack_trace(yod_job *yjob, int pctRank, int tmout)
{
int rc, len;
int bt_size;
char *buf;
int *thePcts;

    _job_in("_job_get_stack_trace");

    bt_size = yjob->done_status[pctRank].bt_size;

    if (DBG_FLAGS(DBG_DBG)){
        _jobMsg("get stack trace of %d bytes from pct rank %d, job %d\n",
		    bt_size, pctRank, yjob->job_id);
    }

    if (bt_size < 1) {
        _job_error_info("invalid parameter to call\n");
	JOB_RETURN_VAL(NULL);
    }
    if (bt_size > MAX_BT_SIZE){
	bt_size = MAX_BT_SIZE - 1;
    }
    len = bt_size + 1;

    buf = (char *)malloc(len);

    if (!buf){
        _job_error_info("can't allocate memory\n");
	JOB_RETURN_VAL(NULL);
    }
    thePcts = yjob->pctNidMap;

    rc = srvr_comm_get_req(buf, bt_size,
               MSG_GET_BT, (char *)&(yjob->job_id), sizeof(int),
               thePcts[pctRank], PPID_PCT, PCT_LOAD_PORTAL,
               1,      /* blocking call */
               tmout);

    if (rc){
        _job_error_info("get request error physical node %d (%s)\n",
            thePcts[pctRank], CPstrerror(CPerrno));
	free(buf);
	buf = NULL;
    }
    else{
	buf[bt_size] = 0;
    }
    JOB_RETURN_VAL(buf);
}
/*
**  Send a get request to the rank 0 pct, place the data in get_data,
**  return -1 on error or 0 on success
*/
int
_job_send_root_pct_get_message(yod_job *yjob, int msg_type, 
                               char *user_data, int user_data_len,
                               char *get_data, int get_data_len, int tmout)
{
int rc;
int *thePcts;

    _job_in("_job_send_root_pct_get_message");

    if (yjob->nnodes < 1){
        _job_error_info("no pcts allocated to job\n");
        JOB_RETURN_ERROR;
    }

    thePcts = yjob->pctNidMap;

    if (DBG_FLAGS(DBG_LOAD_1)){
	_jobMsg("    send get request type %x for %d bytes to PCT %d/%d/%d\n",
		  msg_type, get_data_len, 
		  thePcts[0], PPID_PCT, PCT_LOAD_PORTAL);
    }

    rc = srvr_comm_get_req(get_data, get_data_len, 
               msg_type, user_data, user_data_len,
               thePcts[0], PPID_PCT, PCT_LOAD_PORTAL,
               1,      /* blocking call */
               tmout);

    if (rc){
        _job_error_info(
            "error sending get request to physical node %d (%s)\n",
            thePcts[0], CPstrerror(CPerrno));
    }

    JOB_RETURN_VAL(rc);
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
int
_job_send_pcts_put_message(yod_job *yjob, int msg_type, char *user_data, 
                           int user_data_len, char *put_data, int put_data_len, 
			   int tmout, BOOLEAN rootPctOnly, int member)
{
int bufnum, myrc, mbr, nmembers, rank;
loadMbrs *mbrData;
int ntargets, pids[2], ptls[2];

    _job_in("_job_send_pcts_put_message");

    nmembers = yjob->nMembers;

    if (DBG_FLAGS(DBG_LOAD_1)){
        _jobMsg("    send %s put request type %x, buffer %p, size %d\n",
          rootPctOnly ? "root pct" : "all pcts", msg_type,
                   put_data, put_data_len);
        if (nmembers > 1){
            if (member > -1) { 
	        _jobMsg("    application member # %d\n",member);
            } 
	    else _jobMsg("    all members of application\n");
        }
    }

    pids[0] = PPID_PCT;         pids[1] = -1;
    ptls[0] = PCT_LOAD_PORTAL ; ptls[1] = -1;

    for (mbr = 0; mbr < nmembers; mbr++){ 

        if ( (member >= 0) && (member != mbr)){
            continue;
        }
        mbrData = yjob->Members + mbr;

        rank = mbrData->data.fromRank;

        if (rootPctOnly){
            ntargets = 1;
        }
        else{
            ntargets = mbrData->data.toRank - mbrData->data.fromRank + 1;
        }

        bufnum = srvr_comm_put_req(put_data, put_data_len, 
                             msg_type, 
                             user_data, user_data_len,
                             ntargets, 
			     yjob->pctNidMap + rank, pids, ptls);
                               
	if (bufnum < 0){
	    _job_error_info(
	        "error sending put request to physical node %d (%s)\n",
	        yjob->pctNidMap[rank], CPstrerror(CPerrno));
    
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

    JOB_RETURN_VAL(myrc);
}

#include <sys/time.h>
#include <unistd.h>

static int
wait_pcts_put_msg(int bufnum, int tmout, int howmany)
{
int rc;
struct timeval tv;
long t1;

    _job_in("wait_pcts_put_msg");

    if (tmout){
        gettimeofday(&tv, NULL);
        t1 = tv.tv_sec;
    }

    while (1){
        rc = srvr_test_read_buf(bufnum, howmany);
        if (rc < 0){
	     _job_error_info("waiting for PCTs to pull data (%s)\n",
                                CPstrerror(CPerrno));
            JOB_RETURN_ERROR;
        }

        if (rc == 1) break;  /* all pcts have pulled data */

        if (tmout){ 

             gettimeofday(&tv, NULL);

             if ((tv.tv_sec - t1) > tmout){
                 _job_error_info("timed out waiting for pcts (%d secs)\n", tmout);
		JOB_RETURN_ERROR;
             }
        }
    }

    JOB_RETURN_OK;
}
/*
**   Call _job_get_pct_control_message but wait around for tmout seconds.
*/
int
_job_await_pct_msg(yod_job *yjob, int *mtype, char **user_data, int *pctRank, int tmout)
{
int rc, rank;
struct timeval tv;
long t1;

  _job_in("_job_await_pct_msg");

  if (tmout){
    gettimeofday(&tv, NULL);
    t1 = tv.tv_sec;
  }

  while (1){
    rc = _job_get_pct_control_message(yjob, mtype, user_data, &rank);

    if (rc < 0){
	 _job_error_info("problem waiting for PCT control message (%s)\n",
                     CPstrerror(CPerrno));
	JOB_RETURN_ERROR;
    }

    if (rc == 1) break;  /* got it */ 

    if (tmout){ 

      gettimeofday(&tv, NULL);

      if ((tv.tv_sec - t1) > tmout){
        _job_error_info("timed out (%d secs) waiting for pct control msg\n", 
                          tmout);
	JOB_RETURN_ERROR;
      }
    }
  }

  if (pctRank) {
    *pctRank = rank;
  }
  JOB_RETURN_OK;
}
/*
**   Check for a control message from a pct, save message type in mtype
**   and pointer to user data in user_data, and rank of pct in pctRank.
**   Return 1 if there's a message, 0 if not, -1 on error.  The previous
**   control message is freed before the next is read in, so be certain
**   you're not saving pointers to user data from previous calls.
*/
static control_msg_handle pct_mh;

int
_job_get_pct_control_message(yod_job *yjob, int *mtype, char **user_data, int *pctRank)
{
int rc, src_nid, src_pid, rank;

    _job_in("_job_get_pct_control_message");

    if (SRVR_IS_VALID_HANDLE(pct_mh)){

        srvr_free_control_msg(yjob->pctPtl, &pct_mh); /* free the previous one */
    }

    SRVR_CLEAR_HANDLE(pct_mh);

    rc = srvr_get_next_control_msg(yjob->pctPtl, &pct_mh, mtype, NULL, user_data);

    if (rc == 1){

        src_nid = SRVR_HANDLE_NID(pct_mh);
        src_pid = SRVR_HANDLE_PID(pct_mh);

        if (DBG_FLAGS(DBG_LOAD_1)){
            _jobMsg("Got message type (%s) from pct %d/%d\n",
             select_pct_to_yod_string(*mtype), src_nid, src_pid);
        }
	rank = _nid2rank(yjob, src_nid);

        if (rank < 0){

            _job_error_info("Unexpected message type (%s) from %d/% \n",
		 select_pct_to_yod_string(*mtype), src_nid, src_pid);

            JOB_RETURN_ERROR;
        }
        if (pctRank) *pctRank = rank;
    }

    JOB_RETURN_VAL(rc);    /* 1 - there's a message, 0 - no message */
}
/*
** Wait for a control message for each pct, copy the messages types to
** mtypes array and the user data the the udataBufs
*/

static char checkRank[MAX_PROC_PER_GROUP];

int
_job_all_get_pct_control_message(yod_job *yjob, int *mtypes, 
			    char *udataBufs, int udataBufLen, 
			    int timeout)
{
int i, rc, mtype, rank, ii;
time_t tstart;
char *udata;

    _job_in("_job_all_get_pct_control_message");

    if (yjob->nnodes < 1) JOB_RETURN_OK;

    tstart = time(NULL);
    memset(checkRank, 0, yjob->nnodes);
    i = 0;

    while (i < yjob->nnodes){

        rc = _job_get_pct_control_message(yjob, &mtype, &udata, &rank);

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
            _job_error_info("Failure awaiting messages from the PCTs (%s)\n",
                       CPstrerror(CPerrno));
	    JOB_RETURN_ERROR;
        }

        if (rc == 0){
            if ( (time(NULL) - tstart) > timeout){

                for (rank = 0, ii= 0; rank < yjob->nnodes; rank++){
                    if (checkRank[rank] == 0){

                        if (ii > 5){
                            _job_error_info(
			       "Remaining failure messages suppressed.\n");
                            break;
                        }
                        else{
                            _job_error_info( 
                            "awaiting messages from PCTs, no word from node %d, rank %d\n",
                            yjob->pctNidMap[rank],rank);
                        }

                        ii++;
                    }
                }
                JOB_RETURN_ERROR;
            }
        }
    }
    JOB_RETURN_OK;
}

int
_job_check_link_version(yod_job *yjob, int member)
{
int i, status;
unsigned char* nextchar;
char my_link_no[100], exec_link_no[100];
BOOLEAN found_exec_link_version = FALSE;
loadMbrs *mbr;
 
    _job_in("_job_check_link_version");

    status = 0;

    if ((member < 0) || (member >= yjob->nMembers)){
        _job_error_info("invalid member argument %d\n",member);
	JOB_RETURN_ERROR;
    }
 
    mbr = yjob->Members + member;

    /* find my link version */
    nextchar = (unsigned char *)_cplant_link_version; 
    while ( *nextchar != ':' ) {
      nextchar++;
    }
    nextchar++;
    i = 0;
    while ( *nextchar != ':' ) {
      my_link_no[i++] = *nextchar++; 
    }
    my_link_no[i] = '\0';
    if (jobOptions.show_link_versions) {
      _jobMsg("current link no. is %s\n", my_link_no);
    }

    /* locate executable's link version */
    nextchar = (unsigned char *)_cplant_link_version; 
    for (i=0; i<mbr->data.execlen; i++) {
      if (mbr->exec[i] == *nextchar) {
        nextchar++;
        if (*nextchar == ':') {
          found_exec_link_version = TRUE;
          break;
        }
      }
      else {
        nextchar = (unsigned char *)_cplant_link_version;
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
      if (jobOptions.show_link_versions) {
        _jobMsg("executable link no. is %s\n", exec_link_no);
      }

      /* compare link numbers */
      if ( strcmp(my_link_no,exec_link_no) != 0 ) {
        _jobMsg("--------------------------------------------------------\n");
        _jobMsg("Link version numbers on %s do not match:\n", mbr->pname);
        _jobMsg("current link version is %s\n", my_link_no);
        _jobMsg("link version of executable is %s\n", exec_link_no);
        _jobMsg("--------------------------------------------------------\n");
        status = -1;
      }
    }
    else {
      _jobMsg("--------------------------------------------------------\n");
      _jobMsg("Could not find link version no. in %s:\n", mbr->pname);
      _jobMsg("you may need to relink your application.\n");
      _jobMsg("--------------------------------------------------------\n");
      status = -1;
    }
    JOB_RETURN_VAL(status);
}
int
_job_read_executable(yod_job *yjob, int member)
{
double td;
int fd, rc, i;
loadMbrs *mbr, *mbr2, *sentinel;
 
    _job_in("_job_read_executable");

    if ((member < 0) || (member >= yjob->nMembers)){
        _job_error_info("invalid member argument %d\n",member);
        JOB_RETURN_ERROR;
    }

    mbr = yjob->Members + member;

    if (mbr->pnameSameAs){
	mbr = mbr->pnameSameAs;
    }

    if (mbr->exec != NULL){
        JOB_RETURN_OK;    /* must have read it in already */
    }
 
    mbr->exec = (unsigned char *)malloc(mbr->data.execlen);

    if (mbr->exec == NULL){
        _job_error_info("out of memory for executable size %d\n",mbr->data.execlen);
        JOB_RETURN_ERROR;
    }

    if (jobOptions.timing_data){
        td = dclock();
    }
 
    fd = open(mbr->pname, O_RDONLY);
 
    if (fd < 0){
        _job_error_info("Can't open %s to read\n",mbr->pname);
        JOB_RETURN_ERROR;
    }
    else{
 
        /* read executable into yod memory */
        rc = read(fd, mbr->exec, mbr->data.execlen);
 
        if (rc < mbr->data.execlen){
            _job_error_info("Can't read %s, rc %d\n",mbr->pname,rc);
	    close(fd);
            JOB_RETURN_ERROR;
        }
        else{
            if (jobOptions.timing_data){
                _jobMsg("YOD TIMING: Read in %s, %f\n",mbr->pname, dclock()-td);
            }
        }
        close(fd);
    }

    /* compute a check sum for PCT's verification */
    mbr->exec_check = (unsigned char)0;

    for (i=0; i<mbr->data.execlen; i++){
	 mbr->exec_check ^= mbr->exec[i];
    }

    if (mbr->pnameCount > 1){

         sentinel = yjob->Members + yjob->nMembers;

         for (mbr2 = mbr + 1 ; mbr2 < sentinel; mbr2++){

	     if (mbr2->pnameSameAs == mbr){
	         mbr2->exec       = mbr->exec;
	         mbr2->exec_check = mbr->exec_check;
	     }
	 }
    }

    JOB_RETURN_OK;
}
int
_job_send_executable(yod_job *yjob, int member)
{
int rc;
loadMbrs *mbr, *sameAs;
sendExec msg;
 
    _job_in("_job_send_executable");

    if ((member < 0) || (member >= yjob->nMembers)){
        _job_error_info("invalid member argument %d\n",member);
        JOB_RETURN_ERROR;
    }

    mbr = yjob->Members + member;

    if (mbr->pnameSameAs){
        sameAs = mbr->pnameSameAs;
    }
    else{
        sameAs = mbr;
    }

    sameAs->pnameCount--;

    if (DBG_FLAGS(DBG_LOAD_1)){
      _jobMsg("    send %s, length %d to root PCT\n",
			  mbr->pname, mbr->data.execlen);
    }

    msg.job_ID = yjob->job_id;
    msg.cksum  = mbr->exec_check;
 
    rc = _job_send_pcts_put_message(yjob, MSG_PUT_EXEC, 
                  (char *)&msg, sizeof(sendExec),
                  mbr->exec, mbr->data.execlen, _myEnv.daemonWaitLimit,
                  PUT_ROOT_PCT_ONLY, member);
 
    if (rc){
      _job_error_info("Error sending %s to pcts\n",mbr->pname);
      JOB_RETURN_ERROR;
    }

    if (sameAs->pnameCount == 0){
        free(mbr->exec);
	mbr->exec = NULL;
    }

    JOB_RETURN_OK;
}
/*
** copy executable to a parallel file system
*/
int
_job_move_executable(yod_job *yjob, loadMbrs *mbr)
{
int i, ii, len, rc;
loadMbrs *mbr2;
const char *pfsPath, *vfile;
char *c;
struct stat sbuf;
char vmname[128];
FILE *fp;
double td1;

    _job_in("move_executable");

    mbr2 = mbr->pnameSameAs;

    if (mbr2){

        if (mbr2->execPath){   /* already moved this one */

            mbr->execPath = mbr2->execPath;

            JOB_RETURN_OK;
        }

    }

    /*
    ** Get VM name, which we need to make name of executable unique.
    ** Don't worry if there's no file containing VM name, we'll assume
    ** this is a small Cplant without multiple VMs.
    */
    vfile = vm_name_file();
    vmname[0] = 0;

    if (strcmp(vfile, NO_VM) && !stat(vfile, &sbuf)){

        len = sbuf.st_size;
        if (len >= 128) len = 127;

        fp = fopen(vfile, "r");

        if (fp){
            /*
            ** first word found in this file must be the VM name
            */
            rc = fread(_tmpBuf, len, 1, fp);

            if (rc == 1){

                for (c=_tmpBuf, ii=0, i=0; ii<len; ii++, c++){
                    if (isspace(*c)){
                       if (i) break;
                       else   continue;
                    }
                    if (i==79){   /* corrupt vm name file */
                        i = 0;
                        break;
                    }
                    vmname[i++] = *c;
                }
                vmname[i] = 0;
            }
            fclose(fp);
        }
    }
    if (vmname[0] == 0){
        strcpy(vmname,"Cplant");
    }
    
    pfsPath = pfs();    /* path to global parallel file system */
    
    if (pfsPath == NULL){
        _job_error_info("Insufficient RAM disk on a node and no parallel file system to copy to");
        JOB_RETURN_ERROR;
    }
    /*
    ** Create unique executable name.  
    */
    if (yjob->nMembers == 1){
        sprintf(_tmpBuf, "%s/%s%s-job-%d-%s", pfsPath, NAME_PREFIX, vmname, 
                yjob->job_id, basename(mbr->pname));
    }
    else{
        sprintf(_tmpBuf, "%s/%s%s-job-%d-member-%d", pfsPath, NAME_PREFIX, vmname, 
                yjob->job_id, (int)(mbr - yjob->Members));
    }

    mbr->execPath = strdup(_tmpBuf);

    if (!mbr->execPath){
        _job_error_info("can't malloc");
        JOB_RETURN_ERROR;
    }

    if (mbr2){
        mbr2->execPath = mbr->execPath;
    }

    /*
    ** Copy the executable to parallel global file system.
    */

    snprintf(_tmpBuf, MAXPATHLEN, "cp %s %s", mbr->pname, mbr->execPath);

    if (jobOptions.timing_data) td1 = dclock();

    rc = system(_tmpBuf);

    _jobMsg("%s ...\n",_tmpBuf);

    if (rc) {
        _job_error_info("Copy failed.  Talk to system administration please.\n");
        JOB_RETURN_ERROR;
    }

    if (jobOptions.timing_data){
           _jobMsg("YOD TIMING:  Copy executable to global storage %f\n",
                    dclock()-td1);
    }

    JOB_RETURN_OK;
}
void
_job_remove_executables(yod_job *yjob)
{
int i;
loadMbrs *mbr;

    _job_in("_job_remove_executables");

    for (i=0; i<yjob->nMembers; i++){
        mbr = yjob->Members + i;

        if (!mbr->pnameSameAs){
            if (mbr->execPath) unlink(mbr->execPath);
        }
    }
    JOB_RETURN;
}

#if READY_TO_DO_APP_STUFF

/***********************************************************************
**  app communications
***********************************************************************/

int
get_app_control_message(control_msg_handle *mh)
{
int rc;

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
    
    if (what){
        sprintf(cmd,"/usr/sbin/sendmail -s \"%s\" %s",what,tostring);
    }
    else{
        sprintf(cmd,"/usr/sbin/sendmail -s yodNews %s",tostring);
    }
    notify_fp = popen(cmd, "w");

    free(cmd);

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
int fyod_read_configFile(int *nid)
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
#endif

