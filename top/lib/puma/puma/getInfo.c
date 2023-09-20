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
$Id: getInfo.c,v 1.9 2001/12/11 16:49:13 pumatst Exp $
*/


#ifndef GET_INFO

#include "puma.h"
#include "portal_assignments.h"
#include "bebopd.h"
#include "config.h"

#include "getInfo.h"

#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "pct_start.h"
#include "ppid.h"
#include "srvr_comm.h"
#include "srvr_err.h"
#include "cplant_host.h"
#include "sys_limits.h"

static time_t t1;

static int bebopd_timeout=-1;

static void
update_timeout()
{
char *c;

    if ((c = daemon_timeout())) bebopd_timeout = atoi(c);
}

/*
** These are a list of simple functions that return information to a running
** job about it's own status. They're functions instead of just allowing the
** user access to the variables so they can't mess up anything but their
** own code.
*/

static pct_rec pctRecs[MAX_NODES];

/* Returns the node ID */
int
CplantMyNid() {
  return (int)_my_pnid;
}

/* Returns portal pid */
int
CplantMyPPid() {
  return (int)_my_ppid;
}

/* Returns rank */
int
CplantMyRank() {
  return (int)_my_rank;
}

/* Returns pid */
int
CplantMyPid() {
  return (int)_my_pid;
}

/* Returns number of nodes for job */
int
CplantMySize() {
  return (int)_my_nnodes;
}

/* Return job ID */
int
CplantMyJobId() {
  return (int)_my_gid;
}

/* Return nid map */
int *
CplantMyNidMap() 
{
int *nmap;
int len, i;

  len = _my_nnodes * sizeof(int);
  nmap = (int *)malloc(len);

  if (nmap){

      if (sizeof(int) == sizeof(_my_nid_map[0])){
          memcpy((void *)nmap, (void *)_my_nid_map, len);
      }
      else{
          for (i=0; i<_my_nnodes; i++){
              nmap[i] = _my_nid_map[i];
          }
      }
  }
  return nmap;
}

/* Return pid map */
int *
CplantMyPidMap() 
{
int *pmap;
int len, i;

  len = _my_nnodes * sizeof(int);
  pmap = (int *)malloc(len);

  if (pmap){

      if (sizeof(int) == sizeof(_my_pid_map[0])){
          memcpy((void *)pmap, (void *)_my_pid_map, len);
      }
      else{
          for (i=0; i<_my_nnodes; i++){
              pmap[i] = _my_pid_map[i];
          }
      }
  }
  return pmap;
}
/* Return PBS ID */
int 
CplantMyPBSid() {
  return _my_PBS_ID;
}

/*
** These next functions request information from the bebopd and talk to the
** dynamic node allocation (DNA) interface. Currently only the job size
** checks to make sure the owner and job being checked are the same. The
** rest just trust the user.
*/
int CplantJobSize(int jobID) {

int rc, msgtype, xfer_len, *data;
int bnid, bpid, bptl;
control_msg_handle mhandle;
ping_req req;


/* I may move this into the init_dyn_alloc function and keep the bnid, bpid
** etc. like _my_pid etc. 
*/
#ifdef USE_DB
int bebopd_nid;
#endif

int list, count;


    if (!_my_dna_ptl) {
	fprintf(stderr,"Reply portal for query not initialized\n");
	return -1;
    }

    if (bebopd_timeout < 0) update_timeout();

    memset((char *)&req, 0, sizeof(ping_req));

    req.args.jobID = jobID;
    req.args.euid = geteuid();

    req.pingPtl = _my_dna_ptl;

#ifdef USE_DB

    bebopd_nid = getdb("BEBOPD");
    if (bebopd_nid == NULL ) {
        bnid=SRVR_INVAL_NID;
    }
    else {
        bnid = (int) strtol( bebopd_nid, &eptr, 10);
        if ( eptr== bebopd_nid ) {
            bnid = altBnid;
        }
    }

#else
    if ( cplantHost_getNid("bebopd", 1, &list, &count) != OK ){
        fprintf(stderr, "Can not get nid from cplant-host file\n");
        return -1;
    }
    if (count != 1){
        fprintf(stderr, "Expected one bebopd entry; got %d\n", count);
    }

    bnid = list;
#endif

    bpid = PPID_BEBOPD;
    bptl = (int) DNA_PTL;

    srvr_free_all_control_msgs(_my_dna_ptl);

    rc = srvr_send_to_control_ptl(bnid, bpid, bptl, BEBOPD_INFO_GET_JOB_SIZE,
	 (char *)&req, sizeof(ping_req));

    if ( rc < 0 ) {
	log_warning("CplantJobSize - send request");
	return -1;
    }

    t1 = time(NULL);

    while (1) {

	SRVR_CLEAR_HANDLE(mhandle);

	rc = srvr_get_next_control_msg(_my_dna_ptl, &mhandle, &msgtype,
		&xfer_len, (char**)&data);
	if ( rc==1 ) {
	    srvr_free_control_msg(_my_dna_ptl, &mhandle);
	    return *data;
	}
	if ( rc < 0 ) {
	    log_warning("CplantJobSize - await reply");
	    return -1;
	}

	if ((time(NULL) - t1) > bebopd_timeout){
	        CPerrno = ERECVTIMEOUT;
		return -1;
	}
    }
    return -1;
}

/* 
** These next functions return information from the pingd interface. I may
** rewrite them because a lot of the code is duplicated. They currently
** do not check whether the person asking is the owner of the job being
** queried on or not. Still debating the best way to do that check.
** Also the error codes are not defined. Different numbers are returned for
** different errors, but I have not yet created a map explaining them.
**
** Each function sets an array argument passed to it and returns the size of 
** that array or an arbitrary error code.
*/

/*
** WARNING: these nids are not necessarily in process rank order
*/
int
CplantJobNidMap(int jobID, int ** nid_map) {

int rc, bnid, bpid, bptl, size=0;
int list, count, i, rsize;
ping_req req;
control_msg_handle mhandle;
int pcts_recd;
time_t t1;
pingd_summary *ps;

#ifdef USE_DB
int bebopd_nid;
#endif

    if (bebopd_timeout < 0) update_timeout();

    memset((char *)&req, 0, sizeof(ping_req));

    *nid_map = malloc(MAX_NODES * sizeof(int));
    memset((char *)*nid_map, 0, MAX_NODES * sizeof(int));

    req.args.nid1 = 0;
    req.args.nid2 = MAX_NODES-1;

    req.args.jobID = jobID;

    req.args.euid = geteuid();

    req.pingPtl = _my_dna_ptl;

#ifdef USE_DB

    bebopd_nid = getdb("BEBOPD");
    if (bebopd_nid == NULL ) {
        bnid=SRVR_INVAL_NID;
    }
    else {
        bnid = (int) strtol( bebopd_nid, &eptr, 10);
        if ( eptr== bebopd_nid ) {
            bnid = altBnid;
        }
    }

#else
    if ( cplantHost_getNid("bebopd", 1, &list, &count) != OK ){
        fprintf(stderr, "Can not get nid from cplant-host file\n");
        return -1;
    }
    if (count != 1){
        fprintf(stderr, "Expected one bebopd entry; got %d\n", count);
    }

    bnid = list;
#endif

    bpid = PPID_BEBOPD;

    bptl = (int) PING_PTL;

    rc = srvr_send_to_control_ptl(bnid, bpid, bptl, PCT_STATUS_REQUEST,
	 (char *)&req, sizeof(ping_req));

    if (rc < 0 ) {
	printf("Error sending to bebopd\n");
	return -1;
    }

    t1 = time(NULL);

    while (1){
	SRVR_CLEAR_HANDLE(mhandle);

	rc = srvr_get_next_control_msg(_my_dna_ptl, &mhandle, NULL, NULL,
		(char **)&ps);

	if (rc == 1) break;

	if (rc < 0) {
	    printf("(%s) - getting bebopd notice of data arrival\n",
			CPstrerror(CPerrno));
	    return -1;
	}

	if ((time(NULL) - t1) > bebopd_timeout) {
	    printf("Timeout to bebopd\n");
	    return -2;
	}

    }

    if (ps->nodes == 0) {
	srvr_free_control_msg(_my_dna_ptl, &mhandle);
	printf("No active compute nodes at this time.\n");
	return -3;	
    }


    rsize = sizeof(pct_rec) * MAX_NODES; /* upper bound on msg from bebopd */

    rc = srvr_comm_put_reply(&mhandle, (void *)pctRecs, rsize);

    if (rc < 0){
	printf("(%s) - receiving data from bebopd\n", CPstrerror(CPerrno));
	srvr_free_control_msg(_my_dna_ptl, &mhandle);
	return -4;
    }
    pcts_recd = (SRVR_HANDLE_TRANSFER_LEN(mhandle)) / sizeof(pct_rec);

    for (i=0; i < pcts_recd; i++) {
	
	if (pctRecs[i].status.job_id==jobID) {
	    (*nid_map)[size] = pctRecs[i].nid;
	    size++;
	}     

    }
    srvr_free_control_msg(_my_dna_ptl, &mhandle);
 
    return size;
}


/*
** WARNING: these pids are not necessarily in process rank order
*/
int
CplantJobPidMap(int jobID, int ** pid_map) {

int rc, bnid, bpid, bptl, size=0;
int list, count, i, rsize;
ping_req req;
control_msg_handle mhandle;
int pcts_recd;
time_t t1;
pingd_summary *ps;

#ifdef USE_DB
int bebopd_nid;
#endif

    memset((char *)&req, 0, sizeof(ping_req));

    if (bebopd_timeout < 0) update_timeout();

    *pid_map = malloc(MAX_NODES * sizeof(int));
    memset((char *)*pid_map, 0, MAX_NODES * sizeof(int));

    req.args.nid1 = 0;
    req.args.nid2 = MAX_NODES-1;

    req.args.jobID = jobID;

    req.args.euid = geteuid();

    req.pingPtl = _my_dna_ptl;

#ifdef USE_DB

    bebopd_nid = getdb("BEBOPD");
    if (bebopd_nid == NULL ) {
        bnid=SRVR_INVAL_NID;
    }
    else {
        bnid = (int) strtol( bebopd_nid, &eptr, 10);
        if ( eptr== bebopd_nid ) {
            bnid = altBnid;
        }
    }

#else
    if ( cplantHost_getNid("bebopd", 1, &list, &count) != OK ){
        fprintf(stderr, "Can not get nid from cplant-host file\n");
        return -1;
    }
    if (count != 1){
        fprintf(stderr, "Expected one bebopd entry; got %d\n", count);
    }

    bnid = list;
#endif

    bpid = PPID_BEBOPD;

    bptl = (int) PING_PTL;

    rc = srvr_send_to_control_ptl(bnid, bpid, bptl, PCT_STATUS_REQUEST,
         (char *)&req, sizeof(ping_req));

    if (rc < 0 ) {
        printf("Error sending to bebopd\n");
        return -1;
    }

    t1 = time(NULL);

    while (1){
        SRVR_CLEAR_HANDLE(mhandle);

        rc = srvr_get_next_control_msg(_my_dna_ptl, &mhandle, NULL, NULL,
                (char **)&ps);

        if (rc == 1) break;

        if (rc < 0) {
            printf("(%s) - getting bebopd notice of data arrival\n",
                        CPstrerror(CPerrno));
            return -1;
        }

        if ((time(NULL) - t1) > bebopd_timeout) {
            printf("Timeout to bebopd\n");
            return -2;
        }

    }

    if (ps->nodes == 0) {
        srvr_free_control_msg(_my_dna_ptl, &mhandle);
        printf("No active compute nodes at this time.\n");
        return -3;
    }


    rsize = sizeof(pct_rec) * MAX_NODES; /* upper bound on msg from bebopd */


    rc = srvr_comm_put_reply(&mhandle, (void *)pctRecs, rsize);

    if (rc < 0){
        printf("(%s) - receiving data from bebopd\n", CPstrerror(CPerrno));
        srvr_free_control_msg(_my_dna_ptl, &mhandle);
        return -4;
    }
    pcts_recd = (SRVR_HANDLE_TRANSFER_LEN(mhandle)) / sizeof(pct_rec);

    for (i=0; i < pcts_recd; i++) {

        if (pctRecs[i].status.job_id==jobID) {
            (*pid_map)[size] = pctRecs[i].pid;
            size++;
        }

    }
    srvr_free_control_msg(_my_dna_ptl, &mhandle);

    return size;

}

int
CplantJobStatus(int jobID, job_info ** status) {

int rc, bnid, bpid, bptl, size=0;
int list, count, i, rsize;
ping_req req;
control_msg_handle mhandle;
int pcts_recd;
time_t t1;
job_info * s;
pingd_summary *ps;

#ifdef USE_DB
int bebopd_nid;
#endif

    if (bebopd_timeout < 0) update_timeout();

    memset((char *)&req, 0, sizeof(ping_req));

    req.args.nid1 = 0;
    req.args.nid2 = MAX_NODES-1;

    req.args.jobID = jobID;

    req.args.euid = geteuid();

    req.pingPtl = _my_dna_ptl;

#ifdef USE_DB

    bebopd_nid = getdb("BEBOPD");
    if (bebopd_nid == NULL ) {
        bnid=SRVR_INVAL_NID;
    }
    else {
        bnid = (int) strtol( bebopd_nid, &eptr, 10);
        if ( eptr== bebopd_nid ) {
            bnid = altBnid;
        }
    }

#else
    if ( cplantHost_getNid("bebopd", 1, &list, &count) != OK ){
        fprintf(stderr, "Can not get nid from cplant-host file\n");
        return -1;
    }
    if (count != 1){
        fprintf(stderr, "Expected one bebopd entry; got %d\n", count);
    }

    bnid = list;
#endif

    bpid = PPID_BEBOPD;

    bptl = (int) PING_PTL;

    rc = srvr_send_to_control_ptl(bnid, bpid, bptl, PCT_STATUS_REQUEST,
         (char *)&req, sizeof(ping_req));

    if (rc < 0 ) {
        printf("Error sending to bebopd\n");
        return -1;
    }

    t1 = time(NULL);

    while (1){
        SRVR_CLEAR_HANDLE(mhandle);

        rc = srvr_get_next_control_msg(_my_dna_ptl, &mhandle, NULL, NULL,
                (char **)&ps);

        if (rc == 1) break;

        if (rc < 0) {
            printf("(%s) - getting bebopd notice of data arrival\n",
                        CPstrerror(CPerrno));
            return -1;
        }

        if ((time(NULL) - t1) > bebopd_timeout) {
            printf("Timeout to bebopd\n");
            return -2;
        }

    }

    if (ps->nodes == 0) {
        srvr_free_control_msg(_my_dna_ptl, &mhandle);
        printf("No active compute nodes at this time.\n");
        return -3;
    }


    rsize = sizeof(pct_rec) * MAX_NODES; /* upper bound on msg from bebopd */


    rc = srvr_comm_put_reply(&mhandle, (void *)pctRecs, rsize);

    if (rc < 0){
        printf("(%s) - receiving data from bebopd\n", CPstrerror(CPerrno));
        srvr_free_control_msg(_my_dna_ptl, &mhandle);
        return -4;
    }
    pcts_recd = (SRVR_HANDLE_TRANSFER_LEN(mhandle)) / sizeof(pct_rec);

    s = (job_info *)malloc(pcts_recd * sizeof(job_info));

    for (i=0; i < pcts_recd; i++) {

        if (pctRecs[i].status.job_id==jobID) {

          s[size].rank   = pctRecs[i].status.rank;
          s[size].job_id = pctRecs[i].status.job_id;
          s[size].u_stat = pctRecs[i].status.user_status;
          s[size].session_id  = pctRecs[i].status.session_id;
          s[size].parent_id   = pctRecs[i].status.parent_id;
          s[size].job_pid     = pctRecs[i].status.job_pid;
          s[size].elapsed     = pctRecs[i].status.elapsed;
          s[size].niceKillCountdown = pctRecs[i].status.niceKillCountdown;
          s[size].priority          = pctRecs[i].status.priority;

            s[size].nid      = pctRecs[i].nid;

            size++;
        }

    }
    srvr_free_control_msg(_my_dna_ptl, &mhandle);

    s = (job_info *)realloc(s, (size * sizeof(job_info)));

    *status = s;
    return size;
}

/*
** These next functions get information about PBS from bebopd	    **
*/

int
CplantPBSQueue(char * queue_name, char ** queue_info) {

int rc, bnid, bpid, bptl;
int list, count;
control_msg_handle mhandle;
time_t t1;
int msg_size;
int * msg;

int xfer_len;
char * q_info;

#ifdef USE_DB
int bebopd_nid;
#endif

    if (bebopd_timeout < 0) update_timeout();

    msg_size = (strlen(queue_name)+1) * sizeof(char) + 2*sizeof(int);
    msg = malloc(msg_size);

    msg[0] = _my_dna_ptl;

    msg[1] = geteuid();

    strcpy((char *)(msg+2),queue_name);


#ifdef USE_DB

    bebopd_nid = getdb("BEBOPD");
    if (bebopd_nid == NULL ) {
        bnid=SRVR_INVAL_NID;
    }
    else {
        bnid = (int) strtol( bebopd_nid, &eptr, 10);
        if ( eptr== bebopd_nid ) {
            bnid = altBnid;
        }
    }

#else
    if ( cplantHost_getNid("bebopd", 1, &list, &count) != OK ){
        fprintf(stderr, "Can not get nid from cplant-host file\n");
        return -1;
    }
    if (count != 1){
        fprintf(stderr, "Expected one bebopd entry; got %d\n", count);
    }

    bnid = list;
#endif

    bpid = PPID_BEBOPD;

    bptl = (int) DNA_PTL;

    rc = srvr_send_to_control_ptl(bnid, bpid, bptl, BEBOPD_QUERY_QMGR_Q,
	(char *)msg, msg_size);
    if ( rc < 0 ) {
	printf("Errr sending to bebopd\n");
	return -1;
    }

    t1 = time(NULL);
    while(1) {

        SRVR_CLEAR_HANDLE(mhandle);

        rc = srvr_get_next_control_msg(_my_dna_ptl, &mhandle, NULL, &xfer_len,
                (char **)&q_info);

	if (rc == 1) break;
    
	if (rc < 0 ) {
	    printf("(%s) - getting queue information from bebopd\n",
	    CPstrerror(CPerrno));
	    return -1;
	}

	if ((time(NULL) - t1) > bebopd_timeout) {
	    printf("Timed out waiting for bebopd - %d\n",bebopd_timeout);
	    return -2;
	}
    }

    if (!msg_size) {
	srvr_free_control_msg(_my_dna_ptl, &mhandle);
	printf("Error connecting to queue manager\n");
	return -3;
    }
    
    q_info = (char *)malloc(xfer_len);

    if (!q_info) {
	printf("Error allocating memory for pbs information\n");
	srvr_free_control_msg(_my_dna_ptl, &mhandle);
	return -4;
    }

    rc = srvr_comm_put_reply(&mhandle, q_info, xfer_len);

    if (rc == -1) {
	printf("(%s) - error receiving pbs information.\n",
		CPstrerror(CPerrno));
    }


    srvr_free_control_msg(_my_dna_ptl, &mhandle);

    *queue_info=q_info;

    return 1;
}

int
CplantPBSqstat(qstat_entry ** rval) {

int rc, bnid, bpid, bptl, mtype;
int list, count, i, j, k;
control_msg_handle mhandle;
time_t t1;
int msg_size;
int msg[2];
qstat_entry * qstat_info;

int xfer_len;
char * q_info, *tmp;

#ifdef USE_DB
int bebopd_nid;
#endif


    if (bebopd_timeout < 0) update_timeout();

    msg[0] = _my_dna_ptl;

    msg[1] = geteuid();
    
    msg_size=2*sizeof(int);


#ifdef USE_DB

    bebopd_nid = getdb("BEBOPD");
    if (bebopd_nid == NULL ) {
        bnid=SRVR_INVAL_NID;
    }
    else {
        bnid = (int) strtol( bebopd_nid, &eptr, 10);
        if ( eptr== bebopd_nid ) {
            bnid = altBnid;
        }
    }

#else
    if ( cplantHost_getNid("bebopd", 1, &list, &count) != OK ){
        fprintf(stderr, "Can not get nid from cplant-host file\n");
        return -1;
    }
    if (count != 1){
        fprintf(stderr, "Expected one bebopd entry; got %d\n", count);
    }

    bnid = list;
#endif

    bpid = PPID_BEBOPD;

    bptl = (int) DNA_PTL;

    rc = srvr_send_to_control_ptl(bnid, bpid, bptl, BEBOPD_DO_QSTAT,
        (char *)msg, msg_size);
    if ( rc < 0 ) {
        printf("Errr sending to bebopd\n");
        return -1;
    }


    t1 = time(NULL);
    while(1) {

        SRVR_CLEAR_HANDLE(mhandle);

        rc = srvr_get_next_control_msg(_my_dna_ptl, &mhandle, &mtype, &xfer_len,
                (char **)&q_info);

	if (rc == 1) break;

        if (rc < 0 ) {
            printf("(%s) - getting queue information from bebopd\n",
            CPstrerror(CPerrno));
            return -1;
        }

        if ((time(NULL) - t1) > bebopd_timeout) {
            printf("Timed out waiting for bebopd - %d\n",bebopd_timeout);
            return -2;
        }
    }

    if (mtype == REPLY_NO_DATA) {
	srvr_free_control_msg(_my_dna_ptl, &mhandle);
	return 0;
    }
    
    q_info = (char *)malloc(xfer_len);

    if (!q_info) {
        printf("Error allocating memory for pbs information\n");
        srvr_free_control_msg(_my_dna_ptl, &mhandle);
        return -4;
    }

    rc = srvr_comm_put_reply(&mhandle, q_info, xfer_len);

    if (rc == -1) {
        printf("(%s) - error receiving pbs information.\n",
                CPstrerror(CPerrno));
    }
    
    i=j=k=0;
    /* Peel off the header information from qstat */
    while (j++ < 5) {
	while(q_info[i++] != '\n');
    }
    count = i;
    j = 0;
    while(q_info[i]) {
	if (q_info[i++] == '\n') j++;
    }
    i=count;
    qstat_info = malloc(j * sizeof(qstat_entry));

    /* Each loop sticks the information about one job in the qstat_entry    */
    /* Structure							    */
    while (k < j) {

	/* Get to first non white-space */
	while ((q_info[i] == ' ') || (q_info[i] == '\t') || (q_info[i] == '\n')
                || !(q_info[i])) i++;
	/* Now at job ID. Put job ID in struct */
	count=i;
	while ((q_info[i] != ' ') && (q_info[i] != '\t') && (q_info[i] != '\n')
		&& (q_info[i])) i++;
	tmp = malloc(1+i-count);
	strncpy(tmp,q_info+count,i-count);
	tmp[i-count]='\0';	
	qstat_info[k].job_id=atoi(tmp);
	free(tmp);
	
	/* Skip White space */
	while ((q_info[i] == ' ') || (q_info[i] == '\t') || (q_info[i] == '\n')
                || !(q_info[i])) i++;

	/* Now getting username */
	count=i;
	while ((q_info[i] != ' ') && (q_info[i] != '\t') && (q_info[i] != '\n')
                && (q_info[i])) i++;
        tmp = malloc(1+i-count);
        strncpy(tmp,q_info+count,i-count);
        tmp[i-count]='\0';
	qstat_info[k].username=tmp;

	while ((q_info[i] == ' ') || (q_info[i] == '\t') || (q_info[i] == '\n')
                || !(q_info[i])) i++;

	/* queue name */
	count=i;
        while ((q_info[i] != ' ') && (q_info[i] != '\t') && (q_info[i] != '\n')
                && (q_info[i])) i++;
	tmp = malloc(1+i-count);
        strncpy(tmp,q_info+count,i-count);
        tmp[i-count]='\0';
	qstat_info[k].queue=tmp;

	while ((q_info[i] == ' ') || (q_info[i] == '\t') || (q_info[i] == '\n')
                || !(q_info[i])) i++;

	/* Job name */
	count=i;
	while ((q_info[i] != ' ') && (q_info[i] != '\t') && (q_info[i] != '\n')
                && (q_info[i])) i++;
        tmp = malloc(1+i-count);
        strncpy(tmp,q_info+count,i-count);
        tmp[i-count]='\0';
	qstat_info[k].jobname=tmp;

	while ((q_info[i] == ' ') || (q_info[i] == '\t') || (q_info[i] == '\n')
                || !(q_info[i])) i++;

	/* Session ID */
        count=i;
        while ((q_info[i] != ' ') && (q_info[i] != '\t') && (q_info[i] != '\n')
                && (q_info[i])) i++;
        tmp = malloc(1+i-count);
        strncpy(tmp,q_info+count,i-count);
        tmp[i-count]='\0';
	qstat_info[k].sessID=atoi(tmp);
	free(tmp);

	while ((q_info[i] == ' ') || (q_info[i] == '\t') || (q_info[i] == '\n')
                || !(q_info[i])) i++;

	/* Time in Queue */
        count=i;

	/* If qstat reports "--" set time to -1 */
	if (q_info[count]=='-') {
	    qstat_info[k].q_time=-1;
	    while ((q_info[i] != ' ') && (q_info[i] != '\t') && 
		    (q_info[i] != '\n') && (q_info[i])) i++;
	}
	/* Otherwise parse hh:mm format */
	else {
	    while ((q_info[i] != ':') && (q_info[i] != ' ') && 
		    (q_info[i] != '\t') &&
		    (q_info[i] != '\n') && (q_info[i])) i++;
	    tmp = malloc(1+i-count);
	    strncpy(tmp,q_info+count,i-count);
	    tmp[i-count]='\0';
	    qstat_info[k].q_time=3600 * atoi(tmp);
	    free(tmp);
	    count=++i;
	    while ((q_info[i] != ' ') && (q_info[i] != '\t') && 
		    (q_info[i] != '\n') && (q_info[i])) i++;
	    tmp = malloc(1+i-count);
            strncpy(tmp,q_info+count,i-count);
            tmp[i-count]='\0';
	    qstat_info[k].q_time+= 60 * atoi(tmp);
	    free(tmp);
	}

	while ((q_info[i] == ' ') || (q_info[i] == '\t') || (q_info[i] == '\n')
                || !(q_info[i])) i++;

	/* Number of nodes requested */
	count=i;
	while ((q_info[i] != ' ') && (q_info[i] != '\t') && (q_info[i] != '\n')
                && (q_info[i])) i++;
	tmp = malloc(1+i-count);
        strncpy(tmp,q_info+count,i-count);
        tmp[i-count]='\0';
        qstat_info[k].req_nodes=atoi(tmp);
	free(tmp);

	while ((q_info[i] == ' ') || (q_info[i] == '\t') || (q_info[i] == '\n')
                || !(q_info[i])) i++;

	/* Amount of time requested. 0 if requested time is -- */
        count=i;
        if (q_info[count]=='-') {
	    qstat_info[k].req_time=0;
	    while ((q_info[i] != ' ') && (q_info[i] != '\t') &&
                    (q_info[i] != '\n') && (q_info[i])) i++;
	}    
        else {
            while ((q_info[i] != ':') && (q_info[i] != ' ') && 
                    (q_info[i] != '\t') &&
                    (q_info[i] != '\n') && (q_info[i])) i++;
            tmp = malloc(1+i-count);
            strncpy(tmp,q_info+count,i-count);
            tmp[i-count]='\0';
            qstat_info[k].req_time=3600 * atoi(tmp);
            free(tmp);
            count=++i;
            while ((q_info[i] != ' ') && (q_info[i] != '\t') && 
                    (q_info[i] != '\n') && (q_info[i])) i++;
            tmp = malloc(1+i-count);
            strncpy(tmp,q_info+count,i-count);
            tmp[i-count]='\0';
            qstat_info[k].req_time+= 60 * atoi(tmp);
            free(tmp);
        }

        while ((q_info[i] == ' ') || (q_info[i] == '\t') || (q_info[i] == '\n')
                || !(q_info[i])) i++;

	/* Status - one letter code */
        qstat_info[k].status=q_info[i++];

        while ((q_info[i] == ' ') || (q_info[i] == '\t') || (q_info[i] == '\n')
                || !(q_info[i])) i++;


	/* Elapsed time. -1 if qstat returns '--' */
        count=i;
        if (q_info[count]=='-') {
            qstat_info[k].elapsed_time=-1;
            while ((q_info[i] != ' ') && (q_info[i] != '\t') &&
                    (q_info[i] != '\n') && (q_info[i])) i++;
        }
        else {
            while ((q_info[i] != ':') && (q_info[i] != ' ') &&
                    (q_info[i] != '\t') &&
                    (q_info[i] != '\n') && (q_info[i])) i++;
            tmp = malloc(1+i-count);
            strncpy(tmp,q_info+count,i-count);
            tmp[i-count]='\0';
            qstat_info[k].elapsed_time=3600 * atoi(tmp);
            free(tmp);
            count=++i;
            while ((q_info[i] != ' ') && (q_info[i] != '\t') &&
                    (q_info[i] != '\n') && (q_info[i])) i++;
            tmp = malloc(1+i-count);
            strncpy(tmp,q_info+count,i-count);
            tmp[i-count]='\0';
            qstat_info[k].elapsed_time+= 60 * atoi(tmp);
            free(tmp);
        }

	/* Get to new line */
	while ((q_info[i] != '\n') && q_info[i++]);

	/* Start on the next job's information */
	k++;
    }	

    srvr_free_control_msg(_my_dna_ptl, &mhandle);

    *rval=qstat_info;
    free(q_info);
    return j;

}

int
CplantPBSqueues(char ***queue_list) {


int rc, bnid, bpid, bptl;
int list, count;
int i,j,k;
control_msg_handle mhandle;
time_t t1;
int msg_size;
int mtype;
int  msg[2];
char **q_list;

int xfer_len;
char * q_info, *tmp;

#ifdef USE_DB
int bebopd_nid;
#endif

    if (bebopd_timeout < 0) update_timeout();

    msg[0] = _my_dna_ptl;

    msg[1] = geteuid();

    msg_size=2*sizeof(int);
#ifdef USE_DB

    bebopd_nid = getdb("BEBOPD");
    if (bebopd_nid == NULL ) {
        bnid=SRVR_INVAL_NID;
    }
    else {
        bnid = (int) strtol( bebopd_nid, &eptr, 10);
        if ( eptr== bebopd_nid ) {
            bnid = altBnid;
        }
    }

#else
    if ( cplantHost_getNid("bebopd", 1, &list, &count) != OK ){
        fprintf(stderr, "Can not get nid from cplant-host file\n");
        return -1;
    }
    if (count != 1){
        fprintf(stderr, "Expected one bebopd entry; got %d\n", count);
    }

    bnid = list;
#endif

    bpid = PPID_BEBOPD;

    bptl = (int) DNA_PTL;

    rc = srvr_send_to_control_ptl(bnid, bpid, bptl, BEBOPD_LIST_QUEUES,
        (char *)msg, msg_size);
    if ( rc < 0 ) {
        printf("Errr sending to bebopd\n");
        return -1;
    }

    t1 = time(NULL);
    while(1) {

        SRVR_CLEAR_HANDLE(mhandle);

        rc = srvr_get_next_control_msg(_my_dna_ptl, &mhandle, &mtype, &xfer_len,
                (char **)&q_info);

	if (rc == 1) break;
        if (rc < 0 ) {
            printf("(%s) - getting queue information from bebopd\n",
            CPstrerror(CPerrno));
            return -1;
        }

        if ((time(NULL) - t1) > bebopd_timeout) {
            printf("Timed out waiting for bebopd - %d\n",bebopd_timeout);
            return -2;
        }
    }

    if (mtype == REPLY_NO_DATA) {
        srvr_free_control_msg(_my_dna_ptl, &mhandle);
        return 0;
    }

    q_info = (char *)malloc(xfer_len);

    if (!q_info) {
        printf("Error allocating memory for pbs information\n");
        srvr_free_control_msg(_my_dna_ptl, &mhandle);
        return -4;
    }

    rc = srvr_comm_put_reply(&mhandle, q_info, xfer_len);

    if (rc == -1) {
        printf("(%s) - error receiving pbs information.\n",
                CPstrerror(CPerrno));
    }

    i=j=k;
    /* Peel off the header information */
    while(j++ < 5) {
	while(q_info[i++] != '\n');
    }

    /* Figure out how many entries there actually are keeping	*/
    /* In mind that there is trailing information too		*/
    count=i;
    j=-2;
    while(q_info[i]) {
	if (q_info[i++] == '\n') j++;
    }

    i=count;
    q_list = malloc(j * sizeof(char *));
    while (k < j) {
	/* get to first non white-space */
        while ((q_info[i] == ' ') || (q_info[i] == '\t') || (q_info[i] == '\n')
                || !(q_info[i])) i++;
	count=i;
        while ((q_info[i] != ' ') && (q_info[i] != '\t') && (q_info[i] != '\n')
                && (q_info[i])) i++;
	tmp = malloc(1+i-count);
	strncpy(tmp,q_info+count,i-count);
	tmp[i-count]='\0';
	q_list[k]=tmp;

        /* Get to new line */
        while ((q_info[i] != '\n') && q_info[i++]);
	k++;
    }
    *queue_list = q_list;
    srvr_free_control_msg(_my_dna_ptl, &mhandle);
    free(q_info);
    return j;
}

void
check_server_entry(char * str, server_info *s_info) {

char *tmp;
int i,j;

    j = strlen("default_queue = ");
    if (!memcmp(str,"default_queue = ",j)) {
	i=j;
	while(str[i] != '\n') i++;
	tmp=malloc(1+i-j);
	strncpy(tmp,str+j,i-j);
	tmp[i-j]='\0';
	s_info->default_queue=tmp;
	return;
    }
    j = strlen("resources_available.size = ");
    if (!memcmp(str,"resources_available.size = ",j)) {
	i=j;
        while(str[i] != '\n') i++;
        tmp=malloc(1+i-j);
        strncpy(tmp,str+j,i-j);
        tmp[i-j]='\0';
	s_info->size_avail=atoi(tmp);
	free(tmp);
	return;
    }
    j = strlen("resources_assigned.size = ");
    if (!memcmp(str,"resources_assigned.size = ",j)) {
        i=j;
        while(str[i] != '\n') i++;
        tmp=malloc(1+i-j);
        strncpy(tmp,str+j,i-j);
        tmp[i-j]='\0';
        s_info->size_assign=atoi(tmp);
        free(tmp);
        return;
    }
    j = strlen("resources_max.size = ");
    if (!memcmp(str,"resources_max.size = ",j)) {
        i=j;
        while(str[i] != '\n') i++;
        tmp=malloc(1+i-j);
        strncpy(tmp,str+j,i-j);
        tmp[i-j]='\0';
        s_info->size_max=atoi(tmp);
        free(tmp);
        return;
    }
}

int
CplantPBSserver(server_info *s_info) {

int rc, bnid, bpid, bptl;
int list, count;
int i,j,k;
control_msg_handle mhandle;
time_t t1;
int msg_size;
int mtype;
int  msg[2];

int xfer_len;
char * q_info;

#ifdef USE_DB
int bebopd_nid;
#endif

    if (bebopd_timeout < 0) update_timeout();

    msg[0] = _my_dna_ptl;

    msg[1] = geteuid();

    msg_size=2*sizeof(int);
#ifdef USE_DB

    bebopd_nid = getdb("BEBOPD");
    if (bebopd_nid == NULL ) {
        bnid=SRVR_INVAL_NID;
    }
    else {
        bnid = (int) strtol( bebopd_nid, &eptr, 10);
        if ( eptr== bebopd_nid ) {
            bnid = altBnid;
        }
    }

#else
    if ( cplantHost_getNid("bebopd", 1, &list, &count) != OK ){
        fprintf(stderr, "Can not get nid from cplant-host file\n");
        return -1;
    }
    if (count != 1){
        fprintf(stderr, "Expected one bebopd entry; got %d\n", count);
    }

    bnid = list;
#endif

    bpid = PPID_BEBOPD;

    bptl = (int) DNA_PTL;

    rc = srvr_send_to_control_ptl(bnid, bpid, bptl, BEBOPD_QUERY_SERVER,
        (char *)msg, msg_size);
    if ( rc < 0 ) {
        printf("Errr sending to bebopd\n");
        return -1;
    }

    t1 = time(NULL);
    while(1) {

        SRVR_CLEAR_HANDLE(mhandle);

        rc = srvr_get_next_control_msg(_my_dna_ptl, &mhandle, &mtype, &xfer_len,                (char **)&q_info);

        if (rc == 1) break;
        if (rc < 0 ) {
            printf("(%s) - getting queue information from bebopd\n",
            CPstrerror(CPerrno));
            return -1;
        }

        if ((time(NULL) - t1) > bebopd_timeout) {
            printf("Timed out waiting for bebopd - %d\n",bebopd_timeout);
            return -2;
        }
    }

    if (mtype == REPLY_NO_DATA) {
        srvr_free_control_msg(_my_dna_ptl, &mhandle);
        return 0;
    }

    q_info = (char *)malloc(xfer_len);

    if (!q_info) {
        fprintf(stderr,"Error allocating memory for pbs information\n");
        srvr_free_control_msg(_my_dna_ptl, &mhandle);
        return -4;
    }

    rc = srvr_comm_put_reply(&mhandle, q_info, xfer_len);

    if (rc == -1) {
        fprintf(stderr,"(%s) - error receiving pbs information.\n",
                CPstrerror(CPerrno));
    }


    i=j=k=0;    

    /* Skip the first 5 lines */

    while (j++ < 5) {
	while (q_info[i++] != '\n');
    }
    count = i;
    while(q_info[i] != '\0') {
	/*first skip the white space */
        while ((q_info[i] == ' ') || (q_info[i] == '\t') || (q_info[i] == '\n')
                || !(q_info[i])) i++;
	/* Now check to see if entries match */
	check_server_entry(q_info+i,s_info);
	while(q_info[++i] != '\n' && q_info[i] != '\0');
    }
	

    srvr_free_control_msg(_my_dna_ptl, &mhandle);
    free(q_info);
    return j;
}

/* 
** This function allows a user to set some sort of information associated   **
** with their job, have bebopd keep track of it and allow other jobs and    **
** processes with the same euid to examine it. It is currently pending 	    **
** further thought and the bebopd response code has not been checked in.    **
*/

int
CplantSetUserDef(char *buf, int len) {

int rc, bnid, bpid, bptl;
int mle, list, count;
int msglen, msg[2];
time_t t1;

#ifdef USE_DB
int bebopd_nid;
#endif

    if (bebopd_timeout < 0) update_timeout();

    if (!_my_dna_ptl) {
	fprintf(stderr,"init_dyn_alloc must be called first\n");
	return -1;
    }

    msg[0] = _my_dna_ptl;
    msg[1] = _my_pnid;
    msglen = 2*sizeof(int);

#ifdef USE_DB

    bebopd_nid = getdb("BEBOPD");
    if (bebopd_nid == NULL ) {
        bnid=SRVR_INVAL_NID;
    }
    else {
        bnid = (int) strtol( bebopd_nid, &eptr, 10);
        if ( eptr== bebopd_nid ) {
            bnid = altBnid;
        }
    }

#else
    if ( cplantHost_getNid("bebopd", 1, &list, &count) != OK ){
        fprintf(stderr, "Can not get nid from cplant-host file\n");
        return -1;
    }
    if (count != 1){
        fprintf(stderr, "Expected one bebopd entry; got %d\n", count);
    }

    bnid = list;
#endif

    bpid = PPID_BEBOPD;

    bptl = (int) DNA_PTL;

    mle=srvr_comm_put_req(buf, len, BEBOPD_SET_USER_DATA,(char *)msg, msglen,
	1, &bnid, &bpid, &bptl);

    t1 = time(NULL);

    while (1){
	rc = srvr_test_read_buf(mle, 1);

	if (rc < 0){
	    fprintf(stderr,"data buf to send user information failed\n");
	    return -1;
	}
	if (rc == 1) break;

	if ((time(NULL) -t1) > bebopd_timeout) {
	    fprintf(stderr,"Cannot wait any longer for bebopd to read user data.\n");
	    break;
	}
    }
    return 1;
}

