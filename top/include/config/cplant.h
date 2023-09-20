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
#ifndef _CPLANT_H_
#define _CPLANT_H_

#include "getInfo.h"

#define CPLANT_PATH  "/cplant"

#define CPLANT_MAP   CPLANT_PATH"/etc/cplant-map"

#define SITE_FILE    CPLANT_PATH"/etc/site"

/*
** spawned job statuses
**
** If you add statuses here - fix up displayJobStatus in job_debug.c
*/
#define JOB_NODE_REQUEST_BUILT  ( 1 << 0)
#define JOB_PCT_LIST_ALLOCATED  ( 1 << 1)
#define JOB_REQUESTED_TO_LOAD   ( 1 << 2)
#define JOB_GOT_OK_TO_LOAD      ( 1 << 3)
#define JOB_APP_STARTED         ( 1 << 4)
#define JOB_APP_FINISHED        ( 1 << 5)
#define JOB_APP_MASS_MURDER     ( 1 << 30)

typedef struct _jobFamilyInfo{
   int job_id;          /* Cplant job id */
   int yodHandle;
   int status;          /* bit map of JOB_* values */
   int nprocs;
   int error;
   void *callerHandle;  /* caller can use this, we don't */
}jobFamilyInfo;

/*
** info functions
*/

int CplantMyNid(void) ;     /* query local environment */
int CplantMyPPid(void) ;
int CplantMyRank(void) ;
int CplantMyPid(void) ;
int CplantMySize(void) ;
int CplantMyJobId(void) ;
int *CplantMyNidMap(void) ;
int *CplantMyPidMap(void) ;
int CplantMyPBSid(void) ;

int CplantJobSize(int jobID) ;           /* query bebopd */
/*
** note - nid and pid map need not be in process rank order
*/
int CplantJobNidMap(int jobID, int ** nid_map);
int CplantJobPidMap(int jobID, int ** pid_map) ;
int CplantJobStatus(int jobID, job_info ** status) ;
int CplantPBSQueue(char * queue_name, char ** queue_info) ;
int CplantPBSqstat(qstat_entry ** rval) ;
int CplantPBSqueues(char ***queue_list) ;
int CplantPBSserver(server_info *s_info) ;
int CplantSetUserDef(char *buf, int len) ;

/*
** job spawn/synchronization functions           **  query yod2   **
*/
jobFamilyInfo *CplantSpawnJobGrp(int nlines, char **pnames, char ***argvs,
                                   int *nnodes, int *nprocs, int nMembers, 
                                   int *rankList, int tmout);
jobFamilyInfo *CplantSpawnJob(int nlines, char **pnames, char ***argvs,
                               int *nnodes, int *nprocs);
int CplantFamilyStatusGrp(jobFamilyInfo *job, int nMembers, 
                       int *rankList, int tmout);
int CplantFamilyStatus(jobFamilyInfo *job);
int CplantFamilyMapGrp(jobFamilyInfo *job,
                     int *nmap, int *pmap, int len,
                     int nMembers, int *rankList, int tmout);
int CplantFamilyMap(jobFamilyInfo *job,
                     int *nmap, int *pmap, int len);
int CplantFamilyTerminationGrp(jobFamilyInfo *job,
                     int *exitCode, int *termSig, int *terminator, int *done,
                     int len,
                     int nMembers, int *rankList, int tmout);
int CplantFamilyTermination(jobFamilyInfo *job,
                     int *exitCode, int *termSig, int *terminator, int *done, int len);

jobFamilyInfo *CplantMyParentGrp(int nMembers, int *rankList, int tmout);
jobFamilyInfo *CplantMyParent(void);
jobFamilyInfo *CplantMySelfGrp(int nMembers, int *rankList, int tmout);
jobFamilyInfo *CplantMySelf(void);

int CplantInterjobBarrierGrp(jobFamilyInfo *otherJob, int nMembers, 
                             int *rankList, int tmout);
int CplantInterjobBarrier(jobFamilyInfo *otherJob);
int CplantBarrierStatusGrp(jobFamilyInfo *otherJob,
                          int nMembers, int *rankList, int tmout);
int CplantBarrierStatus(jobFamilyInfo *otherJob);

int CplantSignalJob(int sig, jobFamilyInfo *job);
int CplantNodesRemaining(void);

#define PCT_TERMINATOR_UNSET (0)   /* we count on this to be zero */
#define PCT_NO_TERMINATOR    (1)
#define PCT_JOB_OWNER        (2)
#define PCT_ADMINISTRATOR    (3)

#define SYNC_COMPLETED   1
#define SYNC_IN_PROGRESS 0
#define SYNC_ERROR      (-1)

/*
** node requests
*/

typedef struct _ndrequest{
    int globalSize;
    int globalProcsPerNode;
    char *globalList;
    int nmembers;
    int *localSizes;    /* nodes */
    int *procsPerNode;  /* for SMP someday */
    char **localLists;
    char **pnames;
    char ***pargs;
    int  computedNumNodes;
} ndrequest;

void freeNodeRequest(ndrequest *nd);
int packNodeRequest(int nlines, char **pnames, char ***argvs,
            int *nnodes, int *nprocs, char **req, int *reqlen);
ndrequest *unpackNodeRequest(char *req, int reqlen, char *parentName);
void displayPackedRequest(char *buf, int len);
void displayNodeRequest(ndrequest* nd);

/*
** set up collective communications (*Grp functions)
*/
void CplantDoneCollective();
int CplantInitCollective(int tmout);
int CplantSynchWithYod(int itmout);



#endif 
