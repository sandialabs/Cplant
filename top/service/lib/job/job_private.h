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
#ifndef JOBPRIVATEH
#define JOBPRIVATEH

#include <stdlib.h>
#include <errno.h>
#include <sys/param.h>
#include "puma.h"
#include "appload_msg.h"
#include "job.h"

extern int maxnamelen;

/**********************************************************************/
/********    from yod_data.h ******************************************/
/**********************************************************************/

#define NO_PBS_JOB  0
#define PBS_BATCH   1
#define PBS_INTERACTIVE 2

/*
** What is a "load member":  users can supply yod with the name of an executable,
** or with a load file.  Each line of the load file is a "load member".  It
** specifies an executable name and program arguments (and may specify a "-sz"
** or "-list" argument).  Some of the executables may be the same, but with
** different command line arguments.
*/
typedef struct _loadMbrs {
    loadMemberData data;         /* see appload_msg.h */

    /*
    ** executable path and info - some lines in load file may refer to the
    ** same executable.  We only want to read it in once.
    */
    char *pname;      /* pathname of executable */
    int pnameCount;   /* first entry counts number of members requiring this executable */
    struct _loadMbrs *pnameSameAs;  /* earlier member with same executable */ 
    char *exec;                /* executable file in memory */
    unsigned char exec_check;  /* check sum on executable   */
    char *execPath;    /* executable copied to a location where PCT can read it in */

    char *argstr;

    int localSize;      /* number of nodes requested */
    int localNprocs;    /* number of processes per node */
    int localListSize;  /* number of nodes in optional node list */
    int *localList;    /* list of nodes in optional node list */
    char *localListStr; /* optional node list string */

    int send_or_copy;
    
}loadMbrs;

/**********************************************************************/

/*
** structs to keep track of yod environment
**   and all the Cplant jobs it's started
*/

typedef struct _yod_env{    /* ONE environment per yod */
    char *env;
    int envlen;
    char *cwd;
    int pbs_job;
    int session_id;      /* pbs job ID */
    int session_nodes;   /* pbs nodes allocated */
    int session_priority;

    uid_t uid, euid;
    gid_t gid, egid;

    int ngroups;
    gid_t *groups;

    int bebopdPtl;

    int bnid, bpid, bptl;

    int daemonWaitLimit;
} yod_env;

typedef struct _nidOrder{
   unsigned short nid;
   unsigned short rank;
} nidOrder;

typedef struct _yod_job{     /* MANY Cplant jobs per yod, potentially */

    yod_request *requests;  /* request (or series of requests) to bebopd */
    char **ndListStr;
    int **ndList;
    int nrequests;

    bebopd_status reply;      /* bebopd's reply with job ID, rc */

    int globalSize;       /* number of nodes for whole job */
    int globalNprocs;     /* number of processes per node for whole job */
    int globalListSize;   /* number of nodes in optional node list */
    int *globalList;      /* nodes in optional node list */
    char *globalListStr;  /* optional node list string - list for whole job */

    int nMembers;              /* lines in the load file, (one if no load file) */
    loadMbrs *Members; 

    load_msg1 msg1;                      /* initial message sent to PCTs */
    load_msg2 msg2;                      /* second message to PCTs */
    int nnodes;                          /* nodes in use */
    int *pctNidMap;                     /* PCT phys nids by rank */
    
    nidOrder *nidOrderMap;       /* nid/rank in order by phys node number */

    straceInfo *straceMsg;

    spid_type *pidmap;          /* app system pids */
    ppid_type *ppidmap;         /* app portal pids */

    int    job_id;

    int              nkids;
    struct _yod_job *parent;

    int pctPtl;
    int appPtl;
    int ioWarnings;

    short jobStatus;
    char retryLoad;

    char start_logged;    /* for logging */
    char end_logged;
    char terminator;
    char *userName;
    char *startTime;
    char *endTime;
    char *cmdLine;

    short failCount;                  /* termination info */
    short proc_done_count;

    char          **backTraces;
    app_proc_done *done_status;
    launchErrors  *fail_status;
    char          launch_err_types[LAST_LAUNCH_ERR+1];

    char used;
} yod_job;

#define NMAP(job, rank)   (jobList[job].pctNidMap ? jobList[job].pctNidMap[rank] : (-1))

/*
** structs for bebopd communication
*/

/*
** structs for pct communication
*/

/*********************************************************************************/

/*
** job_data.c
*/
extern yod_env _myEnv;
extern int     _jobArraySize;
extern yod_job *_jobList;

void _updateUids();
int _makeEnv();

int _newJob();
int _getJobHandle(yod_job *job);
yod_job *_getJob(int handle);
void _rollBackToUnallocated(yod_job *yjob);
int _removeJob(int handle);
void _initMember(loadMbrs *mbr);
int _nid2rank(yod_job *yjob, int nid);


loadMbrs *_sameExecutable(yod_job *yjob, char *name, int me);
char *_find_in_path(char *fname);
char *_find_in_cwd(char *fname, char *cwd);
char *_real_path_name(char *fname);
int _process_list(char *list, int *listsize, int **nodes);

/*
** job_comm.c
*/
#define MAX_BT_SIZE   1024*1024

#define PUT_ROOT_PCT_ONLY    1
#define PUT_ALL_PCTS         0

int _initComm();
int _await_bebopd_reply(yod_job *yjob);
int _await_bebopd_get(char *buf, int len, int mtype);
int _send_to_bebopd(char *buf, int len, int sendtype, int gettype);
int _job_send_pct_control_message(int pctNid, int msg_type,
                          char *buf, int len);
int _job_send_all_pcts_control_message(yod_job *yjob, int msg_type,
                          char *buf, int len);
int _job_move_executable(yod_job *yjob, loadMbrs *mbr);
void _job_remove_executables(yod_job *yjob);
char *_job_get_stack_trace(yod_job *yjob, int pctRank, int tmout);
int _job_send_root_pct_get_message(yod_job *yjob, int msg_type,
		   char *user_data, int user_data_len,
		   char *get_data, int get_data_len, int tmout);
int _job_send_pcts_put_message(yod_job *yjob, int msg_type, char *user_data,
		   int user_data_len, char *put_data, int put_data_len,
		   int tmout, BOOLEAN rootPctOnly, int member);
int _job_await_pct_msg(yod_job *yjob, int *mtype, char **user_data, 
                   int *pctRank, int tmout);
int _job_get_pct_control_message(yod_job *yjob, int *mtype, 
                   char **user_data, int *pctRank);
int _job_all_get_pct_control_message(yod_job *yjob, int *mtypes,
		    char *udataBufs, int udataBufLen, int timeout);
int _job_check_link_version(yod_job *yjob, int member);
int _job_read_executable(yod_job *job, int member);
int _job_send_executable(yod_job *yjob, int member);
void _job_mkdate(time_t t1, char **buf);

void _job_log_done(yod_job *yjob);
void _job_log_start(yod_job *yjob);


/*
** job_request.c
*/

extern char _jobErrStr[];
extern char _tmpBuf[];

#define BIGBUF (MAX_NODES*sizeof(int))
#define TMPLEN ((MAXPATHLEN>BIGBUF) ? MAXPATHLEN:BIGBUF)

extern double dclock(void);


/**********************************************************************/
/********    debugging stuff ******************************************/
/**********************************************************************/

void _jobMsg(char* format, ...);
void _jobErrorMsg(char* format, ...);

void _job_in(char *which);
void _job_out();
void _job_error_info(const char *fmt, ...);

int _job_launch_failed(yod_job *yjob, launch_failure *lfail);

void _displayNodeAllocationRequest(yod_job *yjob);
void _displayNodeList(yod_job *yjob);
void _displayJob(yod_job *job);
void _displayAllInfo();
void _displayInitialMsg(load_msg1 *msg1);
void _displayLoadMsg(load_msg2 *msg);

static inline void *_job_malloc(size_t size)
{
void *buf;

    buf = malloc(size);

    if (DBG_FLAGS(DBG_MEMORY)){
       _jobMsg("malloc buffer size %d at %p (%s)\n",size,buf,job_stack_current());
    }
    return buf;
}
static inline void *_job_calloc(size_t nmemb, size_t size)
{
void *buf;

    buf = calloc(nmemb, size);

    if (DBG_FLAGS(DBG_MEMORY)){
       _jobMsg("calloc buffer size %d at %p (%s)\n",size*nmemb,buf,job_stack_current());
    }
    return buf;

}
static inline void *_job_realloc(void *ptr, size_t size)
{
void *buf;

    buf = realloc(ptr, size);

    if (DBG_FLAGS(DBG_MEMORY)){
       _jobMsg("realloc buffer from %p for size %d at %p (%s)\n",
                  ptr,size,buf,job_stack_current());
    }
    return buf;
}
static inline void _job_free(void *ptr)
{
    free(ptr);

    if (DBG_FLAGS(DBG_MEMORY)){
       _jobMsg("free buffer at %p (%s)\n",ptr,job_stack_current());
    }
}

#define malloc(a)     _job_malloc(a)
#define calloc(a,b)   _job_calloc(a,b)
#define realloc(a,b)  _job_realloc(a,b)
#define free(a)       _job_free(a)

#define JOB_RETURN_ERROR    { _job_out(); return -1;}
#define JOB_RETURN_OK       { _job_out(); return 0;}
#define JOB_RETURN_VAL(p)   { _job_out(); return (p);}
#define JOB_RETURN          { _job_out(); }

#endif
