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
** $Id: yod2_data.c,v 1.3 2001/11/24 23:33:37 lafisk Exp $
**
**  Build those complex node requests.
**  Keep track of all those jobs.
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

#include "yod2.h"
#include "job.h"

jobTypes liveJobs[2] = {runningJob, dyingJob};

static int read_load_file(char *fname, ndrequest *nd);
static char *get_real_path_name(char *fname);
static int check_file_type(char *fname);

extern void load_file_lines();

/*
** Fix up fully specified path name
*/
static char full_path[MAXPATHLEN];

static char *
get_real_path_name(char *fname)
{
char *c;

    if (fname[0] == '/'){
        c = realpath(fname, full_path);

        if (!c){
            return NULL;
        }
        else{
           return full_path;
        }
    }
    else{
        yoderrmsg("get_real_path_name called with relative path name - error\n");
        return NULL;
    }
}
static int
check_file_type(char *fname)
{
struct stat statbuf;
int rc;

    rc = stat(fname, &statbuf);

    if (rc == ENOENT){
        yoderrmsg("Error examining %s\n",fname);
        return IS_NOGOOD;
    }

    if (!S_ISREG(statbuf.st_mode)){

        yoderrmsg("Sorry, %s is not a regular file\n",
                   fname);
        return IS_NOGOOD;
    }

    if (statbuf.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)){
        return IS_EXECUTABLE;
    }
    else{
        return IS_REGULAR;
    }
}


/*
********************************************************************************
** node request data structure
********************************************************************************
*/

#define ERR_NODE_REQUEST(s) {yoderrmsg(s); freeNodeRequest(nd); return NULL;}

ndrequest *
build_node_request(int argc, char **argv, int size, int nprocs, char *nodes)
{
ndrequest *nd;
int load_file, i, rc;
char *fname;

    nd = (ndrequest *)calloc(1,sizeof(ndrequest));

    if (!nd){
        yoderrmsg("malloc in build_node_request");
	return NULL;
    }

    nd->nmembers = 0;
    nd->globalSize = size;
    nd->globalProcsPerNode = nprocs;

    nd->computedNumNodes = 0;

    if (nodes){
        nd->globalList = strdup(nodes);

	if (nd->globalList == NULL){
	    ERR_NODE_REQUEST("out of memory\n");
	}
    }
    else{
        nd->globalList = NULL;
    }


    if (argv[0] == NULL){
        ERR_NODE_REQUEST("No load file name or program name\n");
    }

    if (argv[0][0] != '/'){             

        if ((fname = job_find_in_cwd(argv[0])) == NULL){

	    fname = job_find_in_path(argv[0]);
	}
    }
    else{
        fname = get_real_path_name(argv[0]);
    }

    if (fname == NULL){
         ERR_NODE_REQUEST(
	 "Unable to find file in current working directory or in your PATH\n")
    }

    load_file = check_file_type(fname);

    if (load_file == IS_EXECUTABLE){

	nd->nmembers = 1;
	nd->localSizes = NULL;
	nd->procsPerNode = NULL;
	nd->localLists = NULL;

	nd->pnames = (char **)malloc(sizeof(char *) * 2);

        if (!nd->pnames){
	    ERR_NODE_REQUEST(" No load file name or program name\n");
        }

	nd->pnames[0] = strdup(fname);
        nd->pnames[1] = NULL;

	if (argc > 1){

            nd->pargs = (char ***)malloc(sizeof(char **));

	    if (!nd->pargs){
	        ERR_NODE_REQUEST("out of memory");
	    }

            nd->pargs[0] = (char **)malloc(sizeof(char *) * argc);

	    if (!nd->pargs[0]){
	        ERR_NODE_REQUEST("out of memory");
	    }

	    for (i=0; i<(argc-1); i++){
                nd->pargs[0][i] = strdup(argv[i+1]);

	        if (!nd->pargs[0][i]){
		    ERR_NODE_REQUEST("out of memory");
		}
	    }
	    nd->pargs[0][argc-1] = NULL;
	}
    }
    else if (load_file == IS_REGULAR){

        rc = read_load_file(fname, nd);

	if (rc < 0){
	    load_file_lines();
	    ERR_NODE_REQUEST("\nFix your load file\n");
	}
    }
    else {
        ERR_NODE_REQUEST("File access denied\n");
    }

    return nd;
}
/********************************************************************************
** parse a heterogeneous load file
********************************************************************************
*/

#define ARG_STR_COUNT 3
#define REQUIRES_VAL  1   /* this yod argument requires a value */
#define NO_ARG        2   /* this yod argument does not         */

static struct{
   const char *arg;
   int val;
}yodArgs[ARG_STR_COUNT] =
    {
    {"size", REQUIRES_VAL},
    {"sz", REQUIRES_VAL},
    {"list", REQUIRES_VAL}
    };

#define ISBLANK(c) (!isgraph(c))

static int
get_yod_arg(char *c)
{
int i, num = -1;
const char *c2;

    for (i=0; i<ARG_STR_COUNT; i++){
        c2 = yodArgs[i].arg;
        while (*c && *c2 && !ISBLANK((int)*c) && (*c++ == *c2++));
        if ( (*c == 0) || ISBLANK(*c)){
            num = i;
            break;
        }
    }
    return num;
}

#define GET_INT_VAL(ptr, val, line) \
    ptr += 2;                              \
    while (isalpha((int)*ptr)) ptr++;      \
    while (ISBLANK((int)*ptr)) ptr++;      \
    if ((*ptr == 0) || !(isdigit((int)*ptr))){                   \
        yoderrmsg( "can't parse this line in load file\n"); \
        yoderrmsg( "%s\n",line);                            \
        return 0;                          \
    }                                      \
    val = atoi(ptr);

#define GET_STRING_VAL(ptr, val, line) \
    ptr += 2;                          \
    while (isalpha((int)*ptr)) ptr++;  \
    while (ISBLANK((int)*ptr)) ptr++;  \
    if (*ptr == 0){                    \
        yoderrmsg( "can't parse this line in load file\n"); \
        yoderrmsg( "%s\n",line);                            \
        return 0;                      \
    }                                  \
    val = ptr;                         \
    while (!ISBLANK((int)*ptr)) ptr++; \
    *ptr = 0;

static char *loadFileText=NULL;
static char **loadFileLines=NULL;

#define ERR_READ_FILE(s) { \
  yoderrmsg(s); \
  if (loadFileText) {free(loadFileText); loadFileText = NULL;} \
  if (loadFileLines) {free(loadFileLines); loadFileLines = NULL;} \
  return -1;}

static int
read_load_file(char *fname, ndrequest *nd)
{
struct stat statbuf;
int i, ii, fsize, rc, fd;
int sz, argnum, nlines, nargs, len;
char *local_ndlist;
char *c, *endbuf, *yodargs, *pname, *pargs;

    /*
    ** open load file and read into buffer
    */
    rc = stat(fname, &statbuf);

    if (rc == ENOENT){
        yoderrmsg("Can't stat %s\n",fname);
        return 0;
    }

    fsize = statbuf.st_size;

    loadFileText = (char *)malloc(fsize+1);

    if (!loadFileText){
        ERR_READ_FILE("out of memory");
    }

    fd = open(fname, O_RDONLY);

    if (fd < 0){
        ERR_READ_FILE("Can't open load file");
    }
    rc = read(fd, loadFileText, fsize);

    if (rc != fsize){
        ERR_READ_FILE("Can't read load file");
    }
    close(fd);

    loadFileText[fsize] = 0;

    endbuf = loadFileText + fsize;

    /*
    ** Null terminate each line in the load file
    */
    c = loadFileText;
    nlines = 0;

    while (c < endbuf){

        if (*c == '\n'){

            *c = 0;
            nlines++;     /* upper bound on number of load file entries */
        }
        c++;
    }
    nlines++;    /* maybe last entry wasn't followed by a new line */

    c = loadFileText;
    nd->nmembers = 0;

    loadFileLines = (char **)malloc(sizeof(char *) * nlines);

    if (!loadFileLines){
        ERR_READ_FILE("out of memory");
    }

    /*
    ** Identify the lines in the load file that list executables
    */
    while (1){

        while ( ((*c == 0) || ISBLANK((int)*c)) && (c < endbuf)) c++;

        if (c == endbuf) break;

        if (*c != '#'){
            loadFileLines[(nd->nmembers)++] = c;
        }
        while (*c) c++;
    }
    if (nd->nmembers == 0){
        ERR_READ_FILE("No entries detected in load file");
    }

    nd->localSizes = (int *)malloc(sizeof(int) * nd->nmembers);
/*
** not implemented yet
**
    nd->procsPerNode = (int *)malloc(sizeof(int) * nd->nmembers);
*/
    nd->procsPerNode = NULL;

    nd->localLists = (char **)malloc(sizeof(char *) * nd->nmembers);
    nd->pnames     = (char **)malloc(sizeof(char *) * (nd->nmembers + 1));
    nd->pargs      = (char ***)malloc(sizeof(char **) * nd->nmembers);

    if (!nd->localSizes || !nd->localLists || !nd->pnames || !nd->pargs){

        ERR_READ_FILE("out of memory\n");
    }
    nd->pnames[nd->nmembers] = NULL; 

    /*
    ** Format of each line is:
    **
    ** {optional yod arguments} executable-name {optional program arguments}
    */

    for (i=0; i<nd->nmembers; i++){

        sz = 0;
        local_ndlist = NULL;

        yodargs = NULL;   /* will point to first yod arg, or NULL */
        pname = NULL;     /* will point to first char of executable name */
        pargs = NULL;     /* will point to program's first arg, or NULL */

        /*
        ** Locate start of yod argments, executable name, and start of
        ** executable arguments.  Verify yod arguments are valid.
        **
        ** Null terminate the executable name.
        */
        c = loadFileLines[i];     /* first non blank in the line */

        if (*c == '-'){
            yodargs = c;    /* check these are valid yod arguments */
            while (*c == '-'){
                argnum = get_yod_arg(c+1);
                if (argnum < 0){
                    ERR_READ_FILE("bad argument in load file\n");
                }

                while (*c && !ISBLANK((int)*c)) c++;  /* pass argument name */
                if (yodArgs[argnum].val == REQUIRES_VAL){
                    while (*c && ISBLANK((int)*c)) c++;
                    if (*c == 0){
                        ERR_READ_FILE("bad argument in load file\n");
                    }
                    while (*c && !ISBLANK((int)*c)) c++;  /* pass argument value */
                }
                while (*c && ISBLANK((int)*c)) c++;
            }
            pname = c;
            if (*pname == 0){
                ERR_READ_FILE("bad format in load file\n");
            }
        }
        else{             /* no yod arguments preceding executable name */
           pname = c;
        }

        c = pname;
        while (*c && !ISBLANK((int)*c)) c++;
        if (*c){
            *c = 0;    /* null terminate executable name */
            pargs = c+1;
            while (*pargs && ISBLANK((int)*pargs)) pargs++;
            if (*pargs == 0) pargs = NULL;
        }
        else{
            pargs = NULL;
        }


        /*
        ** At most two yod arguments can appear before the program
        ** name, they are -size and -list.
        **
        ** Program name is null terminated so we don't need to worry
        ** about picking up program arguments with strstr.
        */

        if ((c = strstr(loadFileLines[i], "-s"))){
            GET_INT_VAL(c, sz, loadFileLines[i]);
        }

        nd->localSizes[i] = sz;

        if ((c = strstr(loadFileLines[i], "-l"))){
            GET_STRING_VAL(c, local_ndlist, loadFileLines[i]);
        }

        if (local_ndlist){

            nd->localLists[i] = strdup(local_ndlist);

            if (!nd->localLists[i]){
                ERR_READ_FILE("out of memory");
            }
        }
        else{
            nd->localLists[i] = NULL;
        }

        nd->pnames[i] = strdup(pname);

        if (!nd->pnames[i]){
            ERR_READ_FILE("out of memory");
        }

        /*
        ** Anything following the executable name is assumed to be
        ** program arguments.
        */
        nargs = 0;

        if (pargs){
            c = pargs;
            while (1){
                nargs++;
                while (*c && !ISBLANK((int)*c)) c++;  /* skip over arg */
                if (*c == 0) break;

		*c = 0; c++;    /* null terminate the arg */

                while (*c && ISBLANK((int)*c)) c++;   /* get to start of arg */
                if (*c == 0) break;
            }
        }
        if (nargs > 0){
            nd->pargs[i] = (char **)malloc((nargs+1) * sizeof(char *));

            if (!nd->pargs[i]){
                ERR_READ_FILE("out of memory");
            }

            c = pargs;
	    len = -1;

            for (ii=0; ii<nargs; ii++){

		c += (len+1);
                while (*c && ISBLANK((int)*c)) c++; /* get to start of arg */

                nd->pargs[i][ii] = strdup(c);

                if (!nd->pargs[i][ii]){
                    ERR_READ_FILE("out of memory");
                }
		len = strlen(c);
            }
            nd->pargs[i][nargs] = NULL;
        }
        else{
            nd->pargs[i] = NULL;
        }
    }

    return 0;
}

/*
********************************************************************************
** tree of jobs
********************************************************************************
*/
char *stateStrings[10] =
{
"no state",
"new job",
"loading",
"running",
"interrupting job",
"killing job",
"done",
"done/reported",
"load failed",
"invalid"
};

jobTree **jobs[lastType];
int numJobs[lastType];

jobTree origJob;
static int jobListSize;

static int
initJobLists()
{
int i;

    jobListSize = 10;

    for (i=0; i<lastType; i++){
        jobs[i] = (jobTree **)calloc(sizeof(jobTree *) , 10);
        if (!jobs[i]) return -1;
        numJobs[i] = 0;
    }
    return 0;
}
static int
growJobLists()
{
int i;

    jobListSize += 10;

    for (i=0; i<lastType; i++){
        jobs[i] = (jobTree **)realloc(jobs[i], sizeof(jobTree *) * jobListSize);
        if (!jobs[i]) return -1;

        memset((void *)(jobs[i] + jobListSize - 10), 0,
                        10 * sizeof(jobTree *));
    }
    return 0;
}
static int
removeFromJobList(jobTypes type, jobTree *job)
{
int i;
int last;

    last = numJobs[type] - 1;

    for (i=0; i <= last; i++){

        if (jobs[type][i] == job){

            if (i != last){
                jobs[type][i] = jobs[type][last];
            }

            jobs[type][last] = NULL;

            numJobs[type]--;

            break;
        }
    }
    return 0;
}
static int
addToJobList(jobTypes type, jobTree *job)
{
int rc;

    if (jobListSize == 0){
        rc = initJobLists();

        if (rc) return -1;
    }

    if (job->listType == type){
        return 0; /* already on the list */
    }
    else if (job->listType != noType){
        removeFromJobList(job->listType, job);
        job->listType = noType;
    }

    if (numJobs[type] >= jobListSize){
        rc = growJobLists();

        if (rc) return -1;
    }

    jobs[type][numJobs[type]] = job;
    numJobs[type]++; 

    job->listType = type;
    
    return 0;
}
int
addToTmpList(jobTree *job)
{
int rc;

    /*
    ** Many parts of the code loop through a job list,
    ** and then want to change the job's state (which
    ** means in part remove the job from that list).
    ** We can't remove a job from a list while we're 
    ** looping through that list, so we add the job
    ** to a temporary list.  Then when done processing
    ** every job on the list, we process the temporary
    ** list and change the state of jobs on that list
    ** accordingly.
    */

    if (jobListSize == 0){
        rc = initJobLists();

        if (rc) return -1;
    }

    if (numJobs[tempType] >= jobListSize){
        rc = growJobLists();

        if (rc) return -1;
    }

    jobs[tempType][numJobs[tempType]] = job;
    numJobs[tempType]++; 

    return 0;
}

static jobTree *
findJobSearch(jobTree *job, int handle)
{
jobTree *found;

    if (job->handle == handle) return job;

    if (job->child){
        found = findJobSearch(job->child, handle);
        if (found) return found;
    }

    if (job->rightSibling){
        found = findJobSearch(job->rightSibling,handle);
        if (found) return found;
    }

    return NULL;
}

static void
printJob(jobTree *job, int depth, int verbose)
{
char *dm, *dm2;

    if (depth < (MAXDEPTH-1)){
        dm = depthmarker[depth];
        dm2 = depthmarker[depth+1];
    }
    else{
        dm = depthmarker[MAXDEPTH-2];
        dm2 = depthmarker[MAXDEPTH-1];
    }

    printf("%sJob ID %d, %d nodes, %s", dm,
          job->job_id, job->nprocs, stateStrings[job->status]);

    if (job->parent){
        printf(" (parent job ID %d)", job->parent->job_id);
    }
    if (verbose){
        printf("\n");
        if (job->startLoad.tm){
            printf("%sstarted load %s", dm2,job->startLoad.tmstr);
        }
        if (job->startRun.tm){
            printf("%sjob started %s", dm2,job->startRun.tmstr);
        }
        if (job->startKill1.tm) {
              printf("%sSIGTERM sent %s",dm2,job->startKill1.tmstr);
        }
        if (job->startKill2.tm) {
              printf("%sSIGKILL sent %s",dm2,job->startKill2.tmstr);
        }
        if (job->endTime.tm){
            printf("%sjob terminated %s", dm2, job->endTime.tmstr);
        }
    }
    printf("\n");

    return;
}

static void
printJobTree(jobTree *job, int depth, int verbose)
{
    printJob(job, depth, verbose);
    
    if (job->child){
        printJobTree(job->child, depth+1, verbose);
    }
    
    if (job->rightSibling){
        printJobTree(job->rightSibling, depth, verbose);
    }

    return;
}
void
printAllJobs(int verbose)
{
    if (origJob.status == 0){
        printf("no jobs\n");
    }

    printJob(&origJob, 0, verbose);

    if (origJob.child){
       printJobTree(origJob.child, 1, verbose);
    }
}

int
yodJobDelete(jobTree *job)
{
    if (job->child) return -1;

    if (job->status == LOADING_JOB){
        job_load_cancel(job->handle);
        job_free(job->handle);
        log_user(job);
    }

    if (job->leftSibling) job->leftSibling->rightSibling = job->rightSibling;

    if (job->rightSibling) job->rightSibling->leftSibling = job->leftSibling;

    if (job->parent){
       if (job->parent->child == job){
           job->parent->child = job->rightSibling;
       }
    }

    if (job->listType != noType){
        removeFromJobList(job->listType, job);
    }

    if (job->nodeList)    free(job->nodeList);
    if (job->endCode)     free(job->endCode);
    if (job->log_error)   free(job->log_error);
    if (job->log_status)  free(job->log_status);
    if (job->syncWithYod) free(job->syncWithYod);
    if (job->syncPtl)     free(job->syncPtl);
    if (job->syncPid)     free(job->syncPid);

    if (job->ndreq)      freeNodeRequest(job->ndreq);

    job_free(job->handle);
    
    if (job != &origJob){
        free(job);
    }
    else{
        memset((void *)&origJob, 0, sizeof(jobTree));
    }

    return 0;
}

jobTree *
yodJobInit(int handle, int parentHandle, int job_id, ndrequest *ndreq)
{
jobTree *pjob, *leftjob, *job;

    leftjob = NULL;
    pjob = NULL;

    if (parentHandle == INVALID_PARENT_HANDLE){
        job = &origJob;
    }
    else{
        pjob = findJob(parentHandle);

        if (!pjob) return NULL;
   
        job = (jobTree *)malloc(sizeof(jobTree));
   
        if (!job) return NULL;
   
        if (pjob->child){
            leftjob = pjob->child;
   
            while (leftjob->rightSibling){
                leftjob = leftjob->rightSibling;
            }

            leftjob->rightSibling = job;
        }
        else{
            pjob->child = job;
        }
    }

    memset(job, 0, sizeof(jobTree));

    job->handle = handle;
    job->job_id = job_id;
    job->nprocs = ndreq->computedNumNodes;
    job->status = INITIAL_JOB;
    job->parent = pjob;
    job->leftSibling  = leftjob;

    job->ndreq = ndreq;

    return job;
}
int
yodJobLoading(jobTree *job)
{
int pbsID;

    job->startLoad.tm = time(NULL);
    job->startLoad.tmstr = strdup(ctime(&(job->startLoad.tm)));

    job->nodeList = jobInfo_nodeList(job->handle);

    job->status = LOADING_JOB;

    addToJobList(loadingJob, job);

    pbsID = jobInfo_PBSjobID(job->handle);

    if (pbsID != INVAL){
       add_to_log_string(&job->log_status, "PBS #%d, Cplant #%d",
                     pbsID, job->job_id);
    }
    else{
       add_to_log_string(&job->log_status, "Cplant #%d", job->job_id);
    }

    return 0;
}
int
yodJobReLoading(jobTree *job, int newID)
{
int pbsID;

    if (job->status != LOADING_JOB) return -1;

    if (job->nodeList) free(job->nodeList);

    job->nodeList = jobInfo_nodeList(job->handle);
    job->job_id   = newID;

    if (job->log_status){
        free(job->log_status);
        job->log_status = NULL;
    }
    if (job->log_error){
        free(job->log_error);
        job->log_error = NULL;
    }
    job->startLoad.tm = time(NULL);

    if (job->startLoad.tmstr) free(job->startLoad.tmstr);

    job->startLoad.tmstr = strdup(ctime(&(job->startLoad.tm)));

    if (job->endTime.tmstr){
        job->endTime.tm = 0;
        free(job->endTime.tmstr);
        job->endTime.tmstr = NULL;
    } 

    if (pbsID != INVAL){
       add_to_log_string(&job->log_status, "PBS #%d, Cplant #%d",
                     pbsID, job->job_id);
    }
    else{
       add_to_log_string(&job->log_status, "Cplant #%d", job->job_id);
    }

    return 0;
}
int
yodJobRunning(jobTree *job)
{
    job->startRun.tm = time(NULL);
    job->startRun.tmstr = strdup(ctime(&(job->startRun.tm)));

    job->status = RUNNING_JOB;

    addToJobList(runningJob, job);

    return 0;
}
int 
yodJobFirstKill(jobTree *job)
{
    addToJobList(dyingJob, job);

    job->startKill1.tm    = time(NULL);
    job->startKill1.tmstr = strdup(ctime(&(job->startKill1.tm)));

    job->status = KILL1_JOB;

    return 0; 
}
int
yodJobSecondKill(jobTree *job) 
{
    addToJobList(dyingJob, job);

    job->startKill2.tm    = time(NULL);
    job->startKill2.tmstr = strdup(ctime(&(job->startKill2.tm)));

    job->status = KILL2_JOB;

    return 0; 
}
int
yodJobDone(jobTree *job)
{
    addToJobList(finishedJob, job);

    job->endTime.tm    = time(NULL);
    job->endTime.tmstr = strdup(ctime(&(job->endTime.tm)));

    job->status = DONE_JOB;

    log_user(job);

    return 0;
}
int
yodJobReported(jobTree *job)
{
    addToJobList(reportedJob, job);

    job->status = COMPLETION_REPORTED_JOB;

    return 0;
}
char *
yodJobPname(jobTree *job, int rank)
{
int procNum, i;
ndrequest *nd;

    nd = job->ndreq;

    if (!nd || !nd->pnames) return NULL;

    if (nd->pnames[1] == NULL){
        /*
        ** all processes in job are running same executable
        */
        return nd->pnames[0];
    }

    procNum = 0;

    for (i=0; i< nd->nmembers; i++){

        procNum += nd->localSizes[i];

        if (rank < procNum){
            return nd->pnames[i];
        }
    }
    return NULL;
}
jobTree *
findJob(int handle)
{
   if (origJob.status == 0) return NULL;

   return findJobSearch(&origJob, handle);
}
int
load_done(jobTree *job)
{
int status;

   status = jobInfo_jobState(job->handle);

   if (status & JOB_APP_STARTED) return 1;
   else                          return 0;
}

int
retry_load(jobTree *job)
{
int i, rc, job_id;
int status, nprocs;
int handle;

    status = -1;

    yoderrmsg("Load failed.\n");
    yoderrmsg("%s\n",job_strerror());

    add_to_log_string(&job->log_error, "load failed");
    log_user(job);

    handle = job->handle;
    nprocs = job->nprocs;

    if (jobOptions.retryCount && jobInfo_retryLoad(handle)){

        for(i=0; i<jobOptions.retryCount; i++){

            yodmsg("A retry may be successful.\n");

            job_id = job_allocate(handle, priority);

	    if (job_id < 0){
	        yoderrmsg("%s\n",job_strerror());
	        break;
	    }
            yodJobReLoading(job, job_id);

            yodmsg(
              "Retry: %d nodes have been allocated, new job ID %d.\n",
               nprocs, job_id);

            rc = job_load(handle, 0); 

            if (rc == 0){
                status = 0;  /* load OK */
                break;
            } 

            yoderrmsg("Again, load failed.\n");
            yoderrmsg("%s\n",job_strerror());

            add_to_log_string(&job->log_error, "load failed");
            log_user(job);

            if (!jobInfo_retryLoad(handle)){
                break;
            } 
        }
    }

    return status;
}
jobTree *
new_job(ndrequest *ndreq, int parentHandle)
{
int handle, job_id, rc;
jobTree *job;

    /*
    ** handle is for interacting with libjob.a
    */
    handle = job_request(ndreq->globalSize, ndreq->globalProcsPerNode,
                    ndreq->globalList,
                    ndreq->nmembers, 
		    ndreq->localSizes, ndreq->procsPerNode, 
                    ndreq->localLists,
		    ndreq->pnames, ndreq->pargs, parentHandle);

    if (handle < 0){
        yoderrmsg("%s\n",job_strerror());
        yoderrmsg("Can't create node request for bebopd\n");
        free(ndreq);
        return NULL;
    }

    job_id = job_allocate(handle, priority);

    if (job_id < 0){
        yoderrmsg("%s\n",job_strerror());
        yoderrmsg("Can't obtain nodes from bebopd\n");
        job_free(handle);
        free(ndreq);
        return NULL;
    }

    ndreq->computedNumNodes = jobInfo_numNodes(handle);

    yodmsg("%d nodes are allocated to your job ID %d.\n",
                  ndreq->computedNumNodes, job_id);

    job = yodJobInit(handle, parentHandle, job_id, ndreq);

    yodJobLoading(job);

    rc = job_load(handle, 0);   /* non-blocking load */

    if (rc){
        rc = retry_load(job);
        if (rc){
            yoderrmsg("Unable to load job, sorry.\n");
            yodJobDelete(job);
            return NULL;
        }
    }

    if (load_done(job)){
        yodJobRunning(job);
    }
    else{
        job->lastLoadRequest = time(NULL);
    }
    return job;
}
int
numRemainingNodes()
{
int totNodes, usedNodes;
int i, j, type;
jobTree *job;

    if (jobInfo_PBSjobID(origJob.handle) != INVAL){

        totNodes = jobInfo_PBSnumNodes(origJob.handle);
    }
    else{
        /* 
        ** app needs to query bebopd about interactive node availability 
        */
        return 0;
    }

    usedNodes = 0;

    for (i=0; i < 2; i++){

        type = liveJobs[i];

        for (j = 0; j < numJobs[type] ; j++){

            job = jobs[type][j];

            usedNodes += job->nprocs; 
       }
   }
   for (j = 0; j < numJobs[loadingJob] ; j++){

        /*
        ** Small possibility that we are double counting
        ** nodes.  If loading job is waiting for a cycle-stealer
        ** started by us to complete, we're counting it's
        ** nodes twice.
        **
        ** I suggest if remaining nodes is negative, app should
        ** wait a bit and query again.
        */

        job = jobs[loadingJob][j];
        usedNodes += job->nprocs; 
   }

   return (totNodes - usedNodes);
}
