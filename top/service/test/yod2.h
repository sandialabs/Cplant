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
** $Id: yod2.h,v 1.3 2001/11/24 23:33:37 lafisk Exp $
*/

#ifndef YOD2DATAH
#define YOD2DATAH

#include "srvr_comm.h"
#include "cplant.h"
#include "bebopd.h"

/* YOD exit codes */
#define YOD_NO_ERROR 0
#define YOD_APPLICATION_ERROR 1
#define YOD_LOAD_ERROR 2

/* YOD file types */
#define IS_EXECUTABLE 1
#define IS_REGULAR    2
#define IS_NOGOOD     3

#define INVALID_PARENT_HANDLE  (-1)

#define NRETRIES 3

void free_node_request(ndrequest *nd);
ndrequest *build_node_request(int argc, char **argv, int size, int nprocs, char *nodes);
void display_node_request(ndrequest* nd);

/*
** job records
*/
typedef enum{
noType,
loadingJob,
runningJob,
dyingJob,
finishedJob,
reportedJob,
tempType,
lastType
}jobTypes;

extern jobTypes liveJobs[2];
extern int priority;

#define INITIAL_JOB             1
#define LOADING_JOB             2
#define RUNNING_JOB             3
#define KILL1_JOB               4
#define KILL2_JOB               5
#define DONE_JOB                6
#define COMPLETION_REPORTED_JOB 7
#define LOAD_FAILED_JOB         8

typedef struct _timeInfo{
    time_t tm;
    char   *tmstr;
}timeInfo;

typedef struct _jobTree{
   int handle;
   int job_id;
   int nprocs;
   int ndone;
   int nfail;
   int killType;
   int abendWarnings;
   int status;

   ndrequest *ndreq;

   jobTypes listType;
   time_t   lastLoadRequest;
   int *nodeList;

   timeInfo startLoad;
   timeInfo startRun;
   timeInfo startKill1;
   timeInfo startKill2;
   timeInfo endTime;

   final_status *endCode;

   char *log_error;
   char *log_status;
   int   log_done;

   char *syncWithYod;
   short *syncPtl;
   short *syncPid;
   int  nsynced;
   
   struct _jobTree *parent;
   struct _jobTree *leftSibling;
   struct _jobTree *rightSibling;
   struct _jobTree *child;
} jobTree;

void printAllJobs(int verbose);
char *yodJobPname(jobTree *job, int rank);
jobTree *findJob(int handle);
jobTree *new_job(ndrequest *ndreq, int parentHandle);
int load_done(jobTree *job);
int retry_load(jobTree *job);
int numRemainingNodes(void);

jobTree *yodJobInit(int handle, int parentHandle, int job_id, ndrequest *nd);
int yodJobLoading(jobTree *job);
int yodJobReLoading(jobTree *job, int newID);
int yodJobRunning(jobTree *job);
int yodJobFirstKill(jobTree *job); 
int yodJobSecondKill(jobTree *job);
int yodJobDone(jobTree *job);
int yodJobReported(jobTree *job);
int yodJobDelete(jobTree *job);


extern char *stateStrings[10];
extern jobTree **jobs[lastType];
extern int numJobs[lastType];
extern jobTree origJob;

#define NUM_APPS_LEFT \
   (numJobs[loadingJob] + numJobs[runningJob] + numJobs[dyingJob] + numJobs[finishedJob])

int addToTmpList(jobTree *job);

void load_file_lines(void);
void yodmsg(char* format, ...);
void yoderrmsg(char* format, ...);

void add_to_log_string(char **str, const char *fmt, ...);
void log_user(jobTree *job);

/* YOD exit codes */
#define YOD_NO_ERROR 0
#define YOD_APPLICATION_ERROR 1
#define YOD_LOAD_ERROR 2

int perform_service_job(jobTree *job, int srcrank, control_msg_handle *mh);
int register_sync_request(jobTree *job, int srcrank, control_msg_handle *mh);



#endif
