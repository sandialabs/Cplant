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
** $Id: job_data.c,v 1.3 2002/01/18 23:52:56 pumatst Exp $
**
**    manage data pertaining to yod environment, jobs started
**      by yod, and so on.
*/
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "sys_limits.h"
#include "config.h"
#include "job_private.h"

/***************************************************************************
** libjob.a links in some yod files, which need these objects defined
***************************************************************************/

int daemonWaitLimit;

/***************************************************************************
** yod environment
***************************************************************************/

yod_env _myEnv;
int _jobArraySize=0;
yod_job *_jobList=NULL;
char _tmpBuf[TMPLEN];
jobOpt jobOptions;

static int envSet=0;
static int cwdSet=0;

static int
_setCwd()
{
    _job_in("_setCwd");
    /*
    ** CURRENT WORKING DIRECTORY - can change 
    */
    if (!getcwd(_tmpBuf, TMPLEN)){
       _job_error_info("getcwd error (%s)\n",strerror(errno));
       JOB_RETURN_ERROR;
    }

    if (cwdSet && _myEnv.cwd){
        free(_myEnv.cwd);
    }

    _myEnv.cwd = (char *)malloc(strlen(_tmpBuf) + 1);

    if (!_myEnv.cwd){
        _job_error_info("makeEnv malloc error (%s)\n",strerror(errno));
        JOB_RETURN_ERROR;
    }
    else{
        strcpy(_myEnv.cwd, (const char *)_tmpBuf);
    }
    cwdSet = 1;

    JOB_RETURN_OK;
}
void
_updateUids()
{

    _job_in("_updateUids");
    /*
    ** IDs can change at runtime
    */
    _myEnv.uid = getuid();
    _myEnv.gid = getgid();
    _myEnv.euid = geteuid();
    _myEnv.egid = getegid();

    JOB_RETURN;
}

int
_makeEnv()
{
char *env_data;
extern char **environ;

    _job_in("_makeEnv");

    _setCwd();

    if (envSet) return 0;

    env_data = (char *)daemon_timeout();

    if (env_data){
        _myEnv.daemonWaitLimit = atoi(env_data);
    }
    else{
        _myEnv.daemonWaitLimit = 30;
    }

    daemonWaitLimit = _myEnv.daemonWaitLimit;  /* for yod routines */

    /*
    ** PBS ENVIRONMENT
    */
    if (getenv("PBS_ENVIRONMENT")){
        /*
        ** PBS jobs (which may include many invocations of "yod")
        ** may use no more than _myEnv.session_nodes nodes, the number of
        ** nodes allocated to the PBS jobs by PBS.  They may not
        ** be choosy, i.e. they may not specify with a node list
        ** the particular nodes they want to run on.  bebopd will
        ** keep track that a PBS job uses no more than it's
        ** allocated nodes at any point in time.
        */

        if (getenv("PBS_BATCH") ){
            _myEnv.pbs_job = PBS_BATCH;
        }
        else{
            _myEnv.pbs_job = PBS_INTERACTIVE;
        }

        if ((env_data = getenv("PBS_UNUSED_NNODES"))){
              _myEnv.session_nodes = atoi(env_data);
              _myEnv.session_priority = SCAVENGER;
        }
        else if ((env_data = getenv("PBS_NNODES"))){
              _myEnv.session_nodes = atoi(env_data);
              _myEnv.session_priority = REGULAR_JOB;
        }
        else{
            _job_error_info("Invalid PBS environment: no size request defined\n");
            JOB_RETURN_ERROR;
        }

        if ((env_data = getenv("PBS_JOBID"))){
            /*
            ** The PBS job ID is a string like this:  "number.server-name"
            ** The numbers are unique to each PBS server.  Since we use
            ** only one PBS server, we'll use this number alone as the
            ** PBS job ID.  The numbers range from 0 to 999999.
            */
            _myEnv.session_id = atoi(env_data);

        }
        else{
            _job_error_info("Invalid PBS environment: no PBS_JOBID defined\n");
            JOB_RETURN_ERROR;
        }
    }
    else{
        _myEnv.pbs_job = NO_PBS_JOB;
        _myEnv.session_id = INVAL;
        _myEnv.session_nodes = MAX_NODES;
        _myEnv.session_priority = REGULAR_JOB;
    }

    /*
    ** GENERAL ENVIRONMENT
    */

    setenv("PWD", _myEnv.cwd, 1);

    _myEnv.envlen = pack_string(environ, MAX_ENVP, _tmpBuf, TMPLEN);

    if (_myEnv.envlen < 0){
        JOB_RETURN_ERROR;
    }

    _myEnv.env = (char *)malloc(_myEnv.envlen);

    if (!_myEnv.env){
        _job_error_info("makeEnv malloc error (%s)\n",strerror(errno));
        JOB_RETURN_ERROR;
    }

    memcpy(_myEnv.env, _tmpBuf, _myEnv.envlen);

    _updateUids();

    _myEnv.ngroups = getgroups(0, NULL);

    if (_myEnv.ngroups < 0){
        _job_error_info("getgroups error (%s)\n",strerror(errno));
        JOB_RETURN_ERROR;
    }
    else if (_myEnv.ngroups > 0){
	_myEnv.groups = (gid_t *)malloc(_myEnv.ngroups * sizeof(gid_t));

	if (!_myEnv.groups){
	    _job_error_info("makeEnv malloc error (%s)\n",strerror(errno));
	    JOB_RETURN_ERROR;
	}

	getgroups(_myEnv.ngroups, _myEnv.groups);
    }

    envSet = 1;

    JOB_RETURN_OK;
}


/***************************************************************************
** job management
***************************************************************************/

static void
initJob(yod_job *job)
{
    _job_in ("initJob");

    memset((void *)job, 0, sizeof(yod_job));

    job->pctPtl = job->appPtl = SRVR_INVAL_PTL;

    job->terminator = PCT_NO_TERMINATOR;

    JOB_RETURN;
}
int
_newJob()
{
int handle, i;

    _job_in("_newJob");
    
    for (i=0; i<_jobArraySize; i++){
        if (_jobList[i].used == 0){

	     initJob(_jobList + i);
             _jobList[i].used = 1;

             JOB_RETURN_VAL(i);
        }
    }

    _jobArraySize += 8;

    _jobList = realloc(_jobList, sizeof(yod_job) * _jobArraySize);

    if (!_jobList){
        _job_error_info("newJob malloc error (%s)\n",strerror(errno));
        JOB_RETURN_ERROR;
    }

    for (i = _jobArraySize - 8; i < _jobArraySize; i++){
        _jobList[i].used = 0;
    }

    handle = _jobArraySize - 1;

    initJob(_jobList + handle);

    _jobList[handle].used = 1;

    JOB_RETURN_VAL(handle);
}

int
_getJobHandle(yod_job *job)
{
    return (job - _jobList);
}
yod_job *
_getJob(int handle)
{
    _job_in("_getJob");

    if ( (handle<0) ||
         (handle>=_jobArraySize) ||
         (_jobList[handle].used == 0)){

         JOB_RETURN_VAL(NULL);
    }

    JOB_RETURN_VAL(_jobList + handle);
}
void
_initMember(loadMbrs *mbr)
{
    _job_in("_initMember");

    mbr->data.fromRank  = mbr->data.toRank = -1;
    mbr->data.argbuflen = mbr->data.execlen = 0;

    mbr->pname       = NULL;
    mbr->pnameCount  = 0;
    mbr->pnameSameAs = NULL;
    mbr->exec        = NULL;
    mbr->exec_check  = 0;
    mbr->execPath    = NULL;

    mbr->argstr       = NULL;
    mbr->localListStr = NULL;
    mbr->localList    = NULL;
    mbr->localListSize     = 0;
    mbr->localSize   = 0;
    mbr->localNprocs = 0;
    mbr->send_or_copy = INVALID_MSG;

    JOB_RETURN;
}
static void
_removeMember(loadMbrs *mbr)
{
    _job_in("_removeMember");

    if (mbr->pnameSameAs == NULL){

	if (mbr->execPath){
	    sprintf(_tmpBuf,"rm -f %s",mbr->execPath);
	    system(_tmpBuf);

	    free(mbr->execPath);
	}

        if (mbr->pname) free(mbr->pname);
        if (mbr->exec)  free(mbr->exec);
    }

    if (mbr->argstr) free(mbr->argstr);
    if (mbr->localListStr) free(mbr->localListStr);
    if (mbr->localList) free(mbr->localList);

    memset((void *)mbr, 0, sizeof(loadMbrs));

    JOB_RETURN;
}
void
_rollBackToUnallocated(yod_job *yjob)
{
    _job_in("_rollBackToUnallocated");
    /*
    ** go back to state where job request is set up
    ** but bebopd has not allocated us any nodes yet
    ** i.e. job_request() has been called, but not
    ** job_allocate().
    */
    if (yjob->pctNidMap)    free(yjob->pctNidMap);
    if (yjob->nidOrderMap)  free(yjob->nidOrderMap);
    if (yjob->pidmap)       free(yjob->pidmap);
    if (yjob->ppidmap)      free(yjob->ppidmap);

    yjob->pctNidMap = NULL;
    yjob->nidOrderMap = NULL;
    yjob->pidmap = NULL;
    yjob->ppidmap = NULL;

    yjob->reply.job_id = INVAL;
    yjob->reply.rc     = 0;

    yjob->nnodes = 0;
    yjob->job_id = INVAL;

    yjob->start_logged = 0;
    yjob->end_logged   = 0;

    if (yjob->userName) free(yjob->userName);
    if (yjob->cmdLine) free(yjob->cmdLine);
    if (yjob->startTime) free(yjob->startTime);
    if (yjob->endTime) free(yjob->endTime);

    yjob->userName = yjob->cmdLine = yjob->startTime = yjob->endTime = NULL;

    if (yjob->pctPtl != SRVR_INVAL_PTL){
        srvr_release_control_ptl(yjob->pctPtl);
    }

    if (yjob->appPtl != SRVR_INVAL_PTL){
        srvr_release_control_ptl(yjob->appPtl);
    }
    yjob->appPtl = yjob->pctPtl = SRVR_INVAL_PTL;

    yjob->jobStatus = JOB_NODE_REQUEST_BUILT;

    JOB_RETURN;
}
int
_removeJob(int handle)
{
int i;

    _job_in("_removeJob");

    if ( (handle<0) ||
         (handle>=_jobArraySize) ||
         (_jobList[handle].used == 0)){

         JOB_RETURN_ERROR;
    }

    if (_jobList[handle].parent){

        (_jobList[handle].parent)->nkids--;
    }

    for (i=0; i<_jobList[handle].nMembers; i++){

        _removeMember(_jobList[handle].Members + i);
    }

    if (_jobList[handle].requests)        free(_jobList[handle].requests);
    if (_jobList[handle].ndListStr)       free(_jobList[handle].ndListStr);
    if (_jobList[handle].ndList)          free(_jobList[handle].ndList);
    if (_jobList[handle].globalList)      free(_jobList[handle].globalList);
    if (_jobList[handle].globalListStr)   free(_jobList[handle].globalListStr);
    if (_jobList[handle].Members)         free(_jobList[handle].Members);
    if (_jobList[handle].pctNidMap)       free(_jobList[handle].pctNidMap);
    if (_jobList[handle].nidOrderMap)     free(_jobList[handle].nidOrderMap);
    if (_jobList[handle].straceMsg)       free(_jobList[handle].straceMsg);
    if (_jobList[handle].pctNidMap)       free(_jobList[handle].pctNidMap);
    if (_jobList[handle].nidOrderMap)     free(_jobList[handle].nidOrderMap);
    if (_jobList[handle].pidmap)          free(_jobList[handle].pidmap);
    if (_jobList[handle].ppidmap)         free(_jobList[handle].ppidmap);
    if (_jobList[handle].done_status)     free(_jobList[handle].done_status);
    if (_jobList[handle].fail_status)     free(_jobList[handle].fail_status);
    if (_jobList[handle].userName)        free(_jobList[handle].userName);
    if (_jobList[handle].startTime)       free(_jobList[handle].startTime);
    if (_jobList[handle].cmdLine)         free(_jobList[handle].cmdLine);

    if (_jobList[handle].backTraces){

        for (i=0; i<_jobList[handle].nnodes; i++){
	    if (_jobList[handle].backTraces[i]) 
                free (_jobList[handle].backTraces[i]);
	}
	free(_jobList[handle].backTraces);
    }

    if (_jobList[handle].pctPtl != SRVR_INVAL_PTL){
        srvr_release_control_ptl(_jobList[handle].pctPtl);
    }

    if (_jobList[handle].appPtl != SRVR_INVAL_PTL){
        srvr_release_control_ptl(_jobList[handle].appPtl);
    }

    initJob(_jobList + handle);

    JOB_RETURN_OK;
}
int
_nid2rank(yod_job *yjob, int nid)
{
nidOrder *no;
int left, right, mid, rank;

    _job_in("_nid2rank");

    rank = -1;

    if ((nid < 0) || (nid >= MAX_NODES)){
        JOB_RETURN_VAL(rank);
    }

    no = yjob->nidOrderMap;

    if (!no) JOB_RETURN_ERROR;

    left = 0;
    right = yjob->nnodes - 1;

    while (left <= right){  /* binary search */

	mid = (right + left) / 2; 

        if (nid < no[mid].nid){
	    right = mid - 1;
	}
	else if (nid > no[mid].nid){
	    left = mid + 1;
	}
	else {
	    rank = no[mid].rank;
	    break;
	}
    }

    JOB_RETURN_VAL(rank);
}

/***************************************************************************
** run parameters
***************************************************************************/

/*
** node list
*/
int
_process_list(char *list, int *listsize, int **nodes)
{
int *nlist;
int nlistsize;

    _job_in("_process_list");

    nlistsize = parse_node_list(list, (int *)_tmpBuf, (TMPLEN/sizeof(int)),
			       0, MAX_NODES-1);

    if (nlistsize <= 0){
	_job_error_info( "Your node list string is invalid.\n");
	JOB_RETURN_ERROR;
    }

    nlist = (int *)malloc(nlistsize * sizeof(int));

    if (!nlist){
	_job_error_info( "malloc error in job_request (%s)\n",strerror(errno));
	JOB_RETURN_ERROR;
    }

    *listsize = nlistsize;
    *nodes    = nlist;

    memcpy((void *)nlist, (void *)_tmpBuf, nlistsize * sizeof(int));

    JOB_RETURN_OK;
}

/*
** Program name
*/
static char full_path_test[MAXPATHLEN];
static char full_path[MAXPATHLEN];

char *
_real_path_name(char *fname)
{
char *c;

    _job_in("_real_path_name");

    if (fname[0] == '/'){
        c = realpath(fname, full_path);


        if (!c){
	    JOB_RETURN_VAL(NULL);
        }
        else{
            JOB_RETURN_VAL(full_path);
        }
    }
    _job_error_info( "real_path_name called with relative path name - error\n");
    JOB_RETURN_VAL(NULL);
}
/*
** Check in current working directory for path name.
*/
char *
job_find_in_cwd(char *fname)
{
char *c;

    _job_in("job_find_in_cwd");

    _setCwd();

    if (fname[0] == '/'){
        _job_error_info("find_in_cwd called with relative path name - error\n");
	JOB_RETURN_VAL(NULL);
    }

    sprintf(full_path_test, "%s/%s", _myEnv.cwd, fname);

    c = realpath(full_path_test, full_path);

    if (!c){
	JOB_RETURN_VAL(NULL);
    }
    JOB_RETURN_VAL(full_path);
}

/*
** Search PATH environment variable for a file name
*/

char *
job_find_in_path(char *fname)
{
char *mypath, *dirloc, *delim, *endchar;
int dlen, fplen;
char *c;

    _job_in("job_find_in_path");

    if (fname[0] == '/'){
        _job_error_info( "find_in_path called with relative path name - error\n");
	JOB_RETURN_VAL(NULL);
    }

    mypath = getenv("PATH");

    if (mypath) {

        dirloc = mypath;

        endchar = mypath + strlen(mypath);

        fplen = strlen(full_path) + 1;

        while (dirloc < endchar){  /* search user's path */

            delim = strchr(dirloc, ':');

            if (delim){
                dlen = (int)(delim - dirloc);
            }
            else{
                dlen = (int)(endchar - dirloc);
            }

            if (dlen == 0){
                 dirloc++;
                 continue;
            }

            strncpy(full_path_test, dirloc, dlen);

            sprintf(full_path_test + dlen, "/%s", fname);

            c = realpath(full_path_test, full_path);

            if (c == NULL){
                if (delim){
                    dirloc = delim + 1;
                    continue;
                }
                else{
                    dirloc = endchar;
                    continue;
                }
            }
            break;     /* found it */
        }

        if (dirloc >= endchar){
	    JOB_RETURN_VAL(NULL);
        }
    }
    JOB_RETURN_VAL(full_path);
}

loadMbrs *
_sameExecutable(yod_job *yjob, char *name, int me)
{
char *c;
int i;
    _job_in("_sameExecutable");

    if (!yjob->Members){
        JOB_RETURN_VAL(NULL);
    }

    for (i=0; i < me; i++){

        if ((c = yjob->Members[i].pname) && !strcmp(c, name)){

	    JOB_RETURN_VAL(yjob->Members + i);
	}
    }

    JOB_RETURN_VAL(NULL);
}

/*
** Program arguments
*/
int
_makeArgs(loadMbrs *mbr, char **args)
{
    _job_in("_makeArgs");

    mbr->data.argbuflen = pack_string(args, MAX_ARGV, _tmpBuf, TMPLEN);

    if (mbr->data.argbuflen < 0){
        JOB_RETURN_ERROR;
    }

    mbr->argstr = (char *)malloc(mbr->data.argbuflen);

    if (!mbr->argstr){
       _job_error_info( "malloc problem in _makeArgs (%s)\n",strerror(errno));
       JOB_RETURN_ERROR;
    }
    
    memcpy(mbr->argstr, _tmpBuf, mbr->data.argbuflen);

    JOB_RETURN_OK;
}
