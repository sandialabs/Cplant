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
** $Id: dpctest2.c,v 1.2 2001/11/25 00:38:36 lafisk Exp $
**
** Test of dynamic process creation.  
*/

#include "config/cplant.h"

static char *jobStatusString(int status);
static void showFamilyInfo(jobFamilyInfo *jinfo);
static void failure(char *s);
static void parent(char *n);
static void child(jobFamilyInfo *job);
static void runJob(int nlines, char **pnames, char***argvs, 
              int *nnodes, int *nprocs);

int myRank, myJid, myNodes;

jobFamilyInfo parentJob, childJob;

#define TIMEOUT   20.0

main(int argc, char *argv[])
{
int i, rc;
jobFamilyInfo *rjob;

    /*
    ** some basic info about me
    */
    myRank = CplantMyRank();
    myJid  = CplantMyJobId();
    myNodes = CplantMySize();

    printf("Rank %d, job %d, size %d UP\n",
              myRank, myJid, myNodes);

    if ((argc > 1) && (myRank == 0)){

        sleep(2);

        printf("Rank %d, job %d program args are:\n",myRank,myJid);
        for (i=1;i<argc;i++){
            printf("    %s\n",argv[i]);
        }
    }

    /*
    ** I want to use the *Grp version of the functions.
    ** In these functions, the whole app fans in, node
    ** 0 makes the request to yod, and then the result
    ** is fanned back out.
    **
    ** Each of these *Grp functions has a non-collective
    ** counterpart.  (Omit the "Grp" from the function name.)  
    ** If you do your own group management and want one node
    ** to make calls directly to yod, use those.
    **
    ** To use the *Grp functions, I need to setup
    ** collective communcation through the server library:
    */
   
    rc = CplantInitCollective(TIMEOUT);

    if (rc){
        failure("CplantInitCollective");
    }

    if (myRank == 0){
        printf("Rank %d, job %d, SYNCH'ED\n",myRank,myJid);
    }

    /*
    ** Was I spawned by another job?
    */

    rjob = CplantMyParentGrp(0, NULL, TIMEOUT);

    if (!rjob){
       parent(argv[0]);
    }
    else{
       child(rjob);
    }

}

void
child(jobFamilyInfo *job)
{
int rc;
jobFamilyInfo *jinfo;

    if (myRank == 0){
        printf("Rank %d, job %d, CHILD, my parent info: ", myRank, myJid);
        showFamilyInfo(job);
        printf("\n");
    }

    memcpy((char *)&parentJob, (char *)job, sizeof(jobFamilyInfo));

    /*
    ** get my own handle and status
    */

    jinfo = CplantMySelfGrp(0, NULL, TIMEOUT);

    if (!jinfo){
        failure("CplantMySelfGrp");
    }

    if (myRank == 0){
        printf("Rank %d, job %d, QUERYING about me: ", myRank, myJid);
        showFamilyInfo(jinfo);
        printf("\n");
    }
    memcpy((char *)&childJob, (char *)jinfo, sizeof(jobFamilyInfo));


    /*
    ** Now, synchronize with parent application.
    */

    rc = CplantInterjobBarrierGrp(&parentJob, 0, NULL, TIMEOUT);

    if (rc == SYNC_ERROR){
        failure("attempt to start synchronization with parent app");
    }

    if (rc == SYNC_COMPLETED){
        if (myRank == 0){
            printf("Rank %d, job %d, Child/Parent SYNCHRONIZED\n", 
                        myRank, myJid);
        }
    }

    while (rc == SYNC_IN_PROGRESS){

        if (myRank == 0){
            printf(
            "Rank %d, job %d, child/parent synchronization IN PROGRESS\n",
                        myRank, myJid);
        }

        rc = CplantBarrierStatusGrp(&parentJob, 0, NULL, TIMEOUT);

        if (rc == SYNC_ERROR){
 	    failure("attempt to check synchronization with parent app");
        }

        if (rc == SYNC_COMPLETED){
	    if (myRank == 0){
	        printf("Rank %d, job %d, Child/Parent SYNCHRONIZED\n", 
			myRank, myJid);
	    }
            break;
        }
        sleep(5);
    }

    sleep(5);

    exit(0);   /* child is done, parent should detect this */
}

void
parent(char *myname)
{
int rc, i;
jobFamilyInfo *jinfo;
char *pnames[10], **argvs[10];
int nnodes[10];
int nprocs[10];

    if (myRank == 0){
        printf("Rank %d, job %d, PARENT\n", myRank, myJid);
        printf("Rank %d, job %d, ATTEMPT JOB CREATIONS\n", 
                     myRank, myJid);
    }

    for (i=0; i<10; i++){
        argvs[i] = (char **)malloc(sizeof(char *) * 10);
    }

    /*
    ** Start a one node job, a copy of me.
    */ 

    nnodes[0] = 1;
    nprocs[0] = 1;
    pnames[0] = myname;  /* or leave it NULL, defaults to me */
    argvs[0][0] = "-a";
    argvs[0][1] = "-b";
    argvs[0][2] = "-c";
    argvs[0][3] = "-d";
    argvs[0][4] = NULL;
    
    runJob(1, pnames, argvs, nnodes, nprocs);

    /*
    ** Heterogeneous load.  4 nodes, same executable, different args
    */ 

    nnodes[0] = 1;
    nnodes[1] = -1;   /* means repeat same num nodes for each line */

    nprocs[0] = 1;
    nprocs[1] = -1;   /* means repeat same procs/node for each line */

    pnames[0] = myname; 
    pnames[1] = NULL;    /* means same executable name for each line */

    argvs[0][0] = "-a";
    argvs[0][1] = NULL;

    argvs[1][0] = "-b";
    argvs[1][1] = NULL;

    argvs[2][0] = "-c";
    argvs[2][1] = NULL;

    argvs[3][0] = "-d";
    argvs[3][1] = NULL;
    
    runJob(4, pnames, argvs, nnodes, nprocs);

    /*
    ** Heterogeneous load.  4 nodes, 2 different executables.
    */ 

    nnodes[0] = 1;
    nnodes[1] = 2;
    nnodes[2] = 1;

    nprocs[0] = 1;
    nprocs[1] = -1;   /* means repeat same procs/node for each line */

    pnames[0] = myname; 
    pnames[1] = "dpctest";
    pnames[2] = myname; 
    pnames[3] = NULL;       /* does this break anything? */

    runJob(3,        /* 3 command lines */
           pnames, 
           NULL,     /* no program arguments on any command line */
           nnodes, 
           nprocs);

    exit(0);

}
/*
** start a job and don't return until the child job exits
*/
static void
runJob(int nlines, char **pnames, char***argvs, int *nnodes,
         int *nprocs)
{
jobFamilyInfo *jinfo;
int rc;

    jinfo = CplantSpawnJobGrp(nlines,
                       pnames,
                       argvs,
                       nnodes,
                       nprocs,
                     0, NULL, TIMEOUT); /* whole group */

    if (!jinfo){
        failure("CplantSpawnJobGrp");
    }

    if (myRank == 0){
        printf("Rank %d, job %d, CREATED: ", myRank, myJid);
        showFamilyInfo(jinfo);
        printf("\n");
    }

    memcpy((char *)&childJob, (char *)jinfo, sizeof(jobFamilyInfo));

    /*
    ** Now, synchronize with child application.
    */

    rc = CplantInterjobBarrierGrp(&childJob, 0, NULL, TIMEOUT);

    if (rc == SYNC_ERROR){
        failure("attempt to start synchronization with child app");
    }

    if (rc == SYNC_COMPLETED){
        if (myRank == 0){
            printf("Rank %d, job %d, Parent/Child SYNCHRONIZED\n", 
                        myRank, myJid);
        }
    }

    while (rc == SYNC_IN_PROGRESS){

        if (myRank == 0){
            printf(
            "Rank %d, job %d, parent/child synchronization IN PROGRESS\n",
                        myRank, myJid);
        }

        rc = CplantBarrierStatusGrp(&childJob, 0, NULL, TIMEOUT);

        if (rc == SYNC_ERROR){
 	    failure("attempt to check synchronization with child app");
        }

        if (rc == SYNC_COMPLETED){
	    if (myRank == 0){
	        printf("Rank %d, job %d, Parent/Child SYNCHRONIZED\n", 
			myRank, myJid);
	    }
            break;
        }
        sleep(5);
    }
    /*
    ** Await termination of child process
    */

    while (!(childJob.status & JOB_APP_FINISHED)){

        if (childJob.error){
            failure("Error awaiting child job termination");
        }

        rc = CplantFamilyStatusGrp(&childJob, 0, NULL, TIMEOUT);

        if (rc){
             failure("CplantFamilyStatusGrp of child (2)");
        }

        if (myRank == 0){

            printf("Rank %d, job %d, QUERYING about child: ", myRank, myJid);
            showFamilyInfo(&childJob);
            printf("\n");
        }

        sleep(5);
    }
    return;
}
static void
showFamilyInfo(jobFamilyInfo *jinfo)
{
    printf("(job id %d, handle %d, 0x%x (%s), %d procs, error %d)",
       jinfo->job_id, jinfo->yodHandle, jinfo->status,
       jobStatusString(jinfo->status), jinfo->nprocs, jinfo->error);
}
static char *
jobStatusString(int status)
{
    if (status & JOB_APP_FINISHED){
        return "job has finished";
    }
    else if (status & JOB_APP_STARTED){
        return "job has started running on compute nodes";
    }
    else if (status & JOB_GOT_OK_TO_LOAD){
        return "PCTs are ready to load the job";
    }
    else if (status & JOB_REQUESTED_TO_LOAD){
        return "PCTs are not yet ready to load the job";
    }
    else if (status & JOB_PCT_LIST_ALLOCATED){
        return "PCT list allocated, but PCTs not yet contacted";
    }
    else if (status & JOB_NODE_REQUEST_BUILT){
        return "node request created but nodes not yet allocated";
    }
    return NULL; 
}

static void
failure(char *s)
{
    printf("Rank %d, job %d, %s FAILED\n",
              myRank, myJid, s);

    if (myRank == 0){
        printf("***********************************************************\n");
        printf("Only \"yod2\" can handle dynamic process creation requests.\n");
        printf("Did you launch this application with \"yod2\"?\n");
        printf("***********************************************************\n");
    }
    exit(0);
}
