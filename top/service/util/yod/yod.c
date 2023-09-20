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
** yod - invoked by user to load an application
**
**   yod contacts bebopd obtain nodes for the parallel application.
**   Then yod talks directly to pcts to load app processes.  Then 
**   yod waits around for IO requests from application.
**
**  $Id: yod.c,v 1.156.2.5 2002/07/24 00:58:49 ktpedre Exp $
*/

#ifdef __linux__
#include <getopt.h>
#endif
#ifdef __osf__
#include "linux-getopt.h"
#endif

#include <features.h>  /* this can go when the __USE_GNU lines do */
#define __USE_GNU  /* force __USE_GNU to get sysv_signal from signal.h */
#include <signal.h>
#undef  __USE_GNU  /* it conflicts in other places, e.g. off64_t in qkdefs.h */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/file.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include "puma.h"
#include "proc_id.h"
#include "sys_limits.h"
#include "appload_msg.h"
#include "pct_start.h"
#include "rpc_msgs.h"
#include "fileHandle.h"
#include "CMDhandlerTable.h"
#include "host_msg.h"
#include "config.h"
#include "cplant.h"
#include "puma_errno.h"
#include "srvr_comm.h"
#include "srvr_err.h"
#include "srvr_coll_fail.h"
#include "pct_ports.h"
#include "yod_data.h"
#include "yod_log.h"
#include "yod_comm.h"
#include "util.h"
#include "ppid.h"
#include "yod_tv.h"

extern int maxnamelen;
 
/* YOD exit codes */
#define YOD_NO_ERROR 0
#define YOD_APPLICATION_ERROR 1
#define YOD_LOAD_ERROR 2

static int get_yod_options(int , CHAR*[]);
static void node_list_lines(void);
void load_file_lines(void);
static void option_lines(void);
static void more_option_lines(void);
static void strace_option_lines();
static void debug_option_lines(void);
static void usage_message(void);
static void verbose_usage_message(void);
static void yod_err(const CHAR *s);
static void yod_ignore(const CHAR *s);
static void end_game(void);
static void force_kill(void);
static void launch_failed(launch_failure *lfail);
static void try_again(int ec);
static void perform_service(void);
static void setup_sig_handlers(void);
static void signal_handler(int sig);
static void propagate_user_signal(int sig);
static int display_done_messages(void);
static void display_launch_failure(void);
static void display_silent_pcts(void);
static void abend_notify(final_status final, int node, int rank);
static void finish(void);
static void copy_warning(loadMembers *mbr, int nd);
static void request_progress_msg(int , int *, int *, int);

/*
** command line options
*/
static int nprocs;      /* -sz request */
static int comm_space;
static int nprocs_limit;
static int usenx;
int show_link_versions;
int altBpid, altBnid;
static int get_bt;
static int log_startup_actions;
static int pauseForDebugger;
static int fname_index;    
static char **fname_argv;
static int fname_argc;
static int display_allocation;
static int attach_gdb;
static char *done_file;
static int interactive;
static int timing_data;
static char *node_string;
static int listsize;
static int bypass_link_version_check=0;
static char *straceDirectory;
static char *straceOptions;
static char *straceNodeList;
static int straceDirLen, straceListLen, straceOptLen;

static int autokill = 0;
static int proc_done_count = 0;
static int failCount = 0;
static int done_messages_displayed = 0;
static int fail_messages_displayed = 0;
static int silent_pcts_displayed = 0;
static volatile int pct_count = 0;
static CHAR abort_load = 0;
static CHAR reset_load = 0;
static CHAR app_main = 0;
static CHAR in_perform_service = 0;
static time_t tabort = 0;

static int Argc;
static CHAR **Argv;

int Dbglevel;
int Dbgflag;
int DebugSpecial=0;
int prog_phase = -1;
int sverrno=0;
int quiet=0;
int daemonWaitLimit=30;

#ifdef TWO_STAGE_COPY
extern int copy_executable(int member, int job_ID, int *nodeMap, int timing_data,
			   uid_t uid, gid_t gid);
#else
int move_executable(int rank, int timing_data, int jobid);
#endif

extern int cleanup_copied_files(int);
extern double dclock(void);

int pbs_job=NO_PBS_JOB;
int priority;
int retryLoad=0;

int NumProcs;        /* Actual number of nodes - could be nprocs argument, could */
                     /* be some default, or could be computed from load file.    */

int physnid2rank[MAX_NODES];             /* physical node -> rank */
int rank2physnid[MAX_PROC_PER_GROUP];    /* rank -> physical node */ 

#define MAXMSGLEN  256
char msgbuf[MAXMSGLEN];

static ppid_type ppidmap[MAX_PROC_PER_GROUP];   /* rank -> app portal id */
static spid_type pidmap[MAX_PROC_PER_GROUP];    /* rank -> app pid  */

static char         *backTraces[MAX_PROC_PER_GROUP];   /* indexed by rank */
static app_proc_done done_status[MAX_PROC_PER_GROUP];
static launchErrors  fail_status[MAX_PROC_PER_GROUP];
static char launch_err_types[LAST_LAUNCH_ERR+1];

static load_data_buffer LoadData;

static double td0,td2;
static int job_ID;
static time_t startTime, endTime;
static uid_t uid, gid, euid, egid;
static int copies=0;
static int app_initiated_kill=-1;
static int retryCount;

/*
** required for Tflops style logging
*/
static struct passwd *pw;
static char log_record[USERLOG_MAX];
static char cmd_line[USERLOG_MAX];
static char nlist[USERLOG_MAX];
static char terminator=PCT_NO_TERMINATOR;
static char appStartTime[64];
static char appEndTime[64];
static char start_logged=0;
static char end_logged=0;
static void mkdate(time_t t1, char *buf);
/*
**
*/

#ifdef SUPPRESS_FORTRAN_RTL
extern int rtlmessages;
#endif

int
main(int argc, CHAR *argv[], CHAR *envp[])
{
int rc, i, new_umask, put_data_len;
int trial, okToLoad;
fileHandle_t *fh;
CHAR wdname[MAXPATHLEN], ch[80];
load_msg1 init_data;
INT32 envlen;
CHAR *env_data, *mdata;
const char *c; 
INT32 mtype;
INT32 pctRank;
double timeout;
loadMembers *mbr;
straceInfo *straceMsg;
int *replies;

/*        log_to_stderr(1); */
/*        SrvrDbg = 1; */

    _my_ppid = register_ppid(&_my_taskInfo, PPID_AUTO, GID_YOD, "yod");
    if ( _my_ppid == 0 )
    {
        yoderrmsg( "Can not register myself with PPID=%d\n", PPID_AUTO);
        exit(-1);
    }

    rc = server_library_init();

    CLEAR_ERR;

    job_ID = INVAL;

    Argc = argc;
    Argv = argv;

    startTime = endTime = 0;

    if ((c = daemon_timeout())) daemonWaitLimit = atoi(c);

    td0 = dclock();

    log_open("yod");

    if (!getcwd(wdname, MAXPATHLEN)){
       yoderrmsg(
         "yod %d: getcwd error\n",_my_pnid);
       yod_err("current working directory");
    }

    retryCount = ((c = getenv("YODRETRYCOUNT")) ? atoi(c) : NRETRIES);

    if (getenv("PBS_ENVIRONMENT")){
        /*
        ** PBS jobs (which may include many invocations of "yod")
        ** may use no more than nprocs_limit nodes, the number of
        ** nodes allocated to the PBS jobs by PBS.  They may not
        ** be choosy, i.e. they may not specify with a node list
        ** the particular nodes they want to run on.  bebopd will
        ** keep track that a PBS job uses no more than it's 
        ** allocated nodes at any point in time.
        */

	env_data = getenv("PBS_ENVIRONMENT");
	if (strcmp(env_data, "PBS_INTERACTIVE") == 0) {
	    pbs_job = PBS_INTERACTIVE;
	} else {
	    pbs_job = PBS_BATCH;
	}

        if ((env_data = getenv("PBS_UNUSED_NNODES"))){
              nprocs_limit = atoi(env_data);
	      priority = SCAVENGER;
        }
        else if ((env_data = getenv("PBS_NNODES"))){
              nprocs_limit = atoi(env_data);
	      priority = REGULAR_JOB;
        }
        else{
            yod_err("Invalid PBS environment: no size request defined.\n");
        }
  
        if ((env_data = getenv("PBS_JOBID"))){
	    /*
	    ** The PBS job ID is a string like this:  "number.server-name"
	    ** The numbers are unique to each PBS server.  Since we use
	    ** only one PBS server, we'll use this number alone as the
	    ** PBS job ID.  The numbers range from 0 to 999999.
	    */
            init_data.session_id = atoi(env_data);

	    add_to_log_status("PBS #%d ",init_data.session_id);
        }
        else{
            yod_err("Invalid PBS environment: no PBS_JOBID defined");
        }


        if (DBG_FLAGS(DBG_PBS)){
            yodmsg("PBS job # %d, %d nodes allocated\n",
                 init_data.session_id, nprocs_limit);
        }

    }
    else{
        pbs_job = NO_PBS_JOB;
	nprocs_limit = MAX_NODES;
        init_data.session_id = INVAL;
	priority = REGULAR_JOB;
    }

    init_data.parent_handle = INVAL;   /* dynamic process creation fields */
    init_data.parent_job_id = INVAL;
    init_data.my_handle     = INVAL;

    setup_sig_handlers();

    /*************************************************************
    ** get yod options
    *************************************************************/
    
    display_allocation = FALSE;
    attach_gdb = FALSE;
    nprocs = 0;
    listsize = 0;
    node_string = NULL;
    comm_space = 0;         /* for nx, not implemented */
    usenx = 0;
    show_link_versions = 0;  
    get_bt    = 0;          /* user built app with debugging symbols */
    log_startup_actions = 0;/* startup can log progress to PCT */
    pauseForDebugger = 0; 
    fname_index = 0;
    Dbglevel = 0;

    if (pbs_job == PBS_BATCH) {
        interactive = 0;
    } else {
        interactive = 1;
    }

    timing_data = 0;
    done_file = NULL;
    DebugSpecial = 0;
    altBpid = SRVR_INVAL_PID;
    altBnid = SRVR_INVAL_NID;
    straceDirectory=NULL;
    straceOptions=NULL;
    straceNodeList=NULL;
    straceDirLen = straceOptLen = straceListLen = 0;

    /*
    ** Read in command line arguments up to but not including
    ** the executable or load file name.
    */

    rc = get_yod_options(argc, argv);
 
    if (rc < 0){
        yod_err("command line options");
    }
    if (Dbgflag & DBG_ENVIRON){
        extern char **environ;
        char *env;
        int i;

        for (env = environ[0], i=0; env; env = environ[++i]){
            yodmsg("%s\n",env);
        }
        yodmsg("\n");
    }

    uid = getuid();
    gid = getgid();
    euid = geteuid();
    egid = getegid();

    ngroups = getgroups(NGROUPS_MAX, groupList);

    if (ngroups < 0){
	yod_err("getgroups error");
    }

    /*
    ** Read in the file name following the yod arguments.  It's either
    ** the name of the executable to run, or the name of a load file
    ** containing multiple executables to run.  Set up these members
    ** of the parallel application along with their arguments in a
    ** data structure.
    */

    init_data.n_members = determine_members(fname_argc, fname_argv, 
          pbs_job, nprocs, node_string, listsize, euid, egid, wdname);

    if (init_data.n_members <= 0){
        exit(0);
    }

    if ((init_data.n_members > 1) && DBG_FLAGS(DBG_HETERO)){
        yodmsg("%d members in parallel application\n",
                init_data.n_members);
        display_members();
    }

    NumProcs = init_data.nprocs = total_compute_nodes();

    if ((pbs_job != NO_PBS_JOB) && (NumProcs > nprocs_limit)){

        yoderrmsg(
        "The total number of nodes requested in your load file (%d)\n",
           NumProcs);
        yoderrmsg(
        "exceeds the number of nodes allocated to your job by PBS (%d).\n",
           nprocs_limit);
  
        yod_err("Try again.");
    }

    /* do link version check(s) before contacting any load daemons */
    for (i=0; i < init_data.n_members; i++){

      mbr = member_data(i);
      if (mbr->exec == NULL) { /* not read in yet */

        rc = read_executable(i, timing_data);

        if (rc){
          yod_err("read executable into memory");
        }

        if (!bypass_link_version_check){
          rc = check_link_version(i);
          if (rc){
            add_to_log_error("bad link version");
            yod_err("checking link version");
          }
        }
      }
    }

    init_data.yod_id.nid = _my_pnid;
    init_data.yod_id.pid = _my_ppid;

    LoadData.msg2.priority = priority;

    LoadData.msg2.option_bits = 0;
 
    /** 
    if (usenx){
       LoadData.msg2.option_bits |= OPT_INCL_NX;
    }
    **/
    if (get_bt){
       LoadData.msg2.option_bits |= OPT_BT;
    }

    if (attach_gdb){
       LoadData.msg2.option_bits |= OPT_ATTACH;
    }

    if (log_startup_actions){
       LoadData.msg2.option_bits |= OPT_LOG;
    }
 
    if (DebugSpecial){
       LoadData.msg2.option_bits |= OPT_SPECIAL;
    }

    if (pauseForDebugger == 1){
       LoadData.msg2.option_bits |= OPT_SLEEP_1;
    }
    else if (pauseForDebugger == 2){
       LoadData.msg2.option_bits |= OPT_SLEEP_2;
    }
    else if (pauseForDebugger == 3){
       LoadData.msg2.option_bits |= OPT_SLEEP_3;
    }
    else if (pauseForDebugger == 4){
       LoadData.msg2.option_bits |= OPT_SLEEP_4;
    }

    if (comm_space){   

        /* 
        ** We'll use this unused option to make a yod Dummy that
        ** displays initial data and sleeps some requested number
        ** of seconds before exiting.  It doesn't disturb the bebopd
        ** or otherwise impact a running Cplant. Helpful in testing
	** job scheduler/execution environment.
        */
  
         yodmsg("Initial load messages:\n");
         yodmsg("    job_id      NOT SET YET\n");
         yodmsg("    session_id  %d\n",init_data.session_id);
         yodmsg("    nprocs      %d\n",init_data.nprocs);
         yodmsg("    n_members   %d\n",init_data.n_members);
  
         sleep(comm_space);
         exit(0);
    }

    /*************************************************************
    ** start up what it takes to talk to bebopd, pcts, and to 
    ** listen for application requests, get pct list from bebopd
    *************************************************************/

    prog_phase = 0;

    yodmsg("Contacting node allocation daemon...\n");

    if (timing_data){
        td2 = dclock();
    }

    job_ID = initialize_yod(listsize, node_string, 
		    init_data.session_id, nprocs_limit,
		    LoadData.thePcts, euid);

    if (job_ID == INVAL){
        yod_err(NULL);
    }

    add_to_log_status("Cplant #%d ",job_ID);

    init_data.job_id  = job_ID;

    if (timing_data){
        yodmsg("YOD TIMING: allocate nodes %f\n",dclock() - td2);
    }

    yodmsg("%d nodes are allocated to your job ID %d.\n",NumProcs,job_ID);

    init_data.yod_id.ptl = get_pct_portal();

    LoadData.msg2.app_serv_ptl = get_app_srvr_portal();

    /*************************************************************
    ** build load data for remote pcts
    *************************************************************/
 
    envlen = pack_up_env( &env_data, envp, wdname);
 
    if (envlen < 0){
        yod_err("pack_up_env");
    }

    LoadData.msg2.envbuflen = envlen;
 
    initStdioFileHandles();
 
    fh = checkFileHandleList( STDIN_FILE_NAME );

    LoadData.msg2.fstdio[0].retVal = (long)fh;
    LoadData.msg2.fstdio[0].info.openAck.curPos         = fh->curPos;
    LoadData.msg2.fstdio[0].info.openAck.isattyFlag     = isatty(fh->fd);
    LoadData.msg2.fstdio[0].info.openAck.srvrNid        = _my_pnid;
    LoadData.msg2.fstdio[0].info.openAck.srvrPid        = _my_ppid;
    LoadData.msg2.fstdio[0].info.openAck.srvrPtl  = LoadData.msg2.app_serv_ptl;
 
    fh = checkFileHandleList( STDOUT_FILE_NAME );
 
    LoadData.msg2.fstdio[1].retVal = (long)fh;
    LoadData.msg2.fstdio[1].info.openAck.curPos         = fh->curPos;
    LoadData.msg2.fstdio[1].info.openAck.isattyFlag     = isatty(fh->fd);
    LoadData.msg2.fstdio[1].info.openAck.srvrNid        = _my_pnid;
    LoadData.msg2.fstdio[1].info.openAck.srvrPid        = _my_ppid;
    LoadData.msg2.fstdio[1].info.openAck.srvrPtl   = LoadData.msg2.app_serv_ptl;
 
    fh = checkFileHandleList( STDERR_FILE_NAME );
 
    LoadData.msg2.fstdio[2].retVal = (long)fh;
    LoadData.msg2.fstdio[2].info.openAck.curPos         = fh->curPos;
    LoadData.msg2.fstdio[2].info.openAck.isattyFlag     = isatty(fh->fd);
    LoadData.msg2.fstdio[2].info.openAck.srvrNid        = _my_pnid;
    LoadData.msg2.fstdio[2].info.openAck.srvrPid        = _my_ppid;
    LoadData.msg2.fstdio[2].info.openAck.srvrPtl   = LoadData.msg2.app_serv_ptl;
 
    new_umask = 0x0000;
    LoadData.msg2.u_mask = umask(new_umask);
    umask(LoadData.msg2.u_mask);

    LoadData.msg2.uid = uid;
    LoadData.msg2.gid = gid;
    LoadData.msg2.euid = euid;
    LoadData.msg2.egid = egid;
 
 /* (VOID) fyod_read_configFile(&LoadData.msg2.fyod_nid); */

    LoadData.msg2.fyod_nid = 0;  /* PCTs get fyod nids now */

    if (Dbglevel > 0) {
      printf("fyod_nid= %d\n", LoadData.msg2.fyod_nid);
    }

    LoadData.msg2.ngroups = ngroups;

    if (ngroups <= FEW_GROUPS){
	for (i=0; i<ngroups; i++){
	    LoadData.msg2.groups[i] = groupList[i];
	}
    }

    if (straceDirectory){
        char *c;

        LoadData.msg2.straceMsgLen =
                  sizeof(straceInfo) +
		  straceDirLen + straceOptLen + straceListLen +
                  3;                           /* null bytes */

        straceMsg = (straceInfo *)malloc(LoadData.msg2.straceMsgLen);

	if (!straceMsg){
            yod_err("allocating memory for strace information");
	}

        if (DBG_FLAGS(DBG_MEMORY)){
            yodmsg("memory: %p (%u) strace info\n",straceMsg,LoadData.msg2.straceMsgLen);
        }


        straceMsg->job_ID = job_ID;
        straceMsg->dirlen = straceDirLen;
        straceMsg->optlen = straceOptLen;
        straceMsg->listlen = straceListLen;

        c = (char *)straceMsg + sizeof(straceInfo);

        strcpy(c, straceDirectory);
        c += straceMsg->dirlen;
        c++;

        if (straceMsg->optlen){

            strcpy(c, straceOptions);
            c += straceMsg->optlen;
            c++;
        }
	else{
	    *c++ = 0;
	}

        if (straceMsg->listlen){
            strcpy(c, straceNodeList);
        }
	else{
	    *c++ = 0;
	}
    }
    else{
        straceMsg = NULL;
        LoadData.msg2.straceMsgLen = 0;
    }

    /*************************************************************
    ** Notify all pcts that we are ready to load.  Don't procede
    ** until all reply OK_TO_LOAD.  In a system supporting
    ** cycle-stealing jobs, the PCTs allocated to my job may be 
    ** in the process of killing off cycle-stealers.  This can
    ** take a few minutes since we're nice and give them time
    ** to clean up.  PCTs will reply OK when they're ready.  If
    ** we don't support cycle-stealing jobs, the PCTs are always
    ** ready when they are handed to us by the bebopd.
    **
    ** If you don't support cycle-stealing jobs, you can comment
    ** out this step in yod. 
    **
    ** (Cycle stealing jobs come from a low priority PBS queue.
    ** They are jobs that are killable and that run on the nodes 
    ** that regular PBS jobs have been allocated
    ** but are not currently using.  When the regular jobs want
    ** their nodes back, bebopd tells the PCTs to kill them,
    ** and the regular job can load an application as soon as
    ** the cycle stealer exits.)
    **
    ** (Interactive low priority jobs {"yod -nice"} are jobs that
    ** can be killed when a regular priority interactive job needs
    ** the nodes.)
    *************************************************************/

#ifndef STRIPDOWN
#define REQUEST_TO_LOAD
#endif

#ifdef REQUEST_TO_LOAD

    replies = (int *)malloc(NumProcs * sizeof(int));
    mdata = (char *)malloc(NumProcs * sizeof(int));

    if (!replies || !mdata){
	yod_err("Out of memory");
    }

    for (trial=0; trial<14; trial++){

	if (DBG_FLAGS(DBG_LOAD_2)){
	    yodmsg("Send REQUEST_TO_LOAD message to pcts\n");
	}

	rc =  send_pcts_control_message(MSG_REQUEST_TO_LOAD,
			 (CHAR *)&init_data, sizeof(load_msg1), NULL);

	if (rc){
	    retryLoad = 1;
	    yod_err("sending REQUEST_TO_LOAD to the pcts");
	}

	if (DBG_FLAGS(DBG_LOAD_2)){
	    yodmsg("AWAIT OK message from pcts\n");
	}


	rc = all_get_pct_control_message(replies, mdata, sizeof(int), daemonWaitLimit);

	if (rc){
	    yod_err("Problem getting OK TO LOAD messages from the PCTs");
	}

	okToLoad = 1;

	for (i=0; i<NumProcs; i++){

	    if (replies[i] == TRY_AGAIN_MSG){

	         request_progress_msg(trial, replies, (int *)mdata, NumProcs);

		 okToLoad = 0;
		 break;
	    }
	    else if (replies[i] == REJECT_LOAD_MSG){

	         send_pcts_control_message(MSG_CANCEL_REQUEST_TO_LOAD,
			 (CHAR *)&job_ID, sizeof(int), NULL);

	         yod_err("A node refuses to load the job.");
	    }
	}
	if (okToLoad) break;

	sleep(30);

    }
    if (DBG_FLAGS(DBG_LOAD_2)){
	yodmsg("All PCTs reported OK TO LOAD.\n");
    }
    free(replies);
    free(mdata);
#endif /* REQUEST_TO_LOAD */


    /*************************************************************
    ** Notify all pcts that they are hosting a parallel app.  They
    ** will pull the load data and pct map from us.
    *************************************************************/

    if (DBG_FLAGS(DBG_LOAD_2)){
        yodmsg("Send INIT_LOAD message to pcts\n");
    }

    if (timing_data){
        td2 = dclock();
    }
    put_data_len = sizeof(load_data_buffer) -
                   (sizeof(int) * (MAX_NODES - NumProcs));

    for (i=0; i < init_data.n_members; i++){

        mbr = member_data(i);

        memcpy((char *)&(LoadData.msg2.data), &(mbr->data), 
                sizeof(loadMemberData));

        rc = send_pcts_put_message(MSG_INIT_LOAD,
                          (CHAR *)&init_data, sizeof(load_msg1),
                          (CHAR *)&LoadData, put_data_len,
                           daemonWaitLimit, PUT_ALL_PCTS, i);

        if (rc){
	    retryLoad = 1;
            yod_err("send initial data and pct map to pcts");
        }
    }
    if (timing_data){
        yodmsg("YOD TIMING: send initial msg to all pcts %f\n",dclock()-td2);
    }

    prog_phase = 1;

    /*************************************************************
    ** Pcts are initializing their membership and their collective
    ** communication structures.  Then they acknowledge and let yod
    ** know if executable fits in RAM disk (SEND_EXEC_MSG) or does
    ** not (COPY_EXEC_MSG).
    *************************************************************/

    if (DBG_FLAGS(DBG_LOAD_2)){
        yodmsg("Await OK message from root pct\n");
    }
    
    if (timing_data){
        td2 = dclock();
    }

    pct_count = NumProcs;    /* pcts may be in loading state now */

    for (i=0; i<init_data.n_members; i++){
        mbr = member_data(i);
        mbr->send_or_copy = INVALID_MSG;
    }
    for (i=0; i<init_data.n_members; i++){
        rc = await_pct_msg(&mtype, &mdata, &pctRank, daemonWaitLimit);

        if (mtype == LAUNCH_FAILURE_MSG){
            launch_failed((launch_failure *)mdata);
            yod_err("load failed, gathering post mortem");
        }
        if ((rc < 0) || 
            ( (mtype!=SEND_EXEC_MSG) && (mtype!=COPY_EXEC_MSG)) ||
            (pctRank < 0) || (pctRank >= NumProcs)  ) {

             yodmsg( "Apparent problem with compute nodes.\n");
	     retryLoad = 1;
             yod_err("aborting load");
         }

         mbr = which_member(pctRank);

         if (mbr->send_or_copy != INVALID_MSG){
             yodmsg( "Apparent problem with compute nodes.\n");
             yod_err("aborting load");
         }

         if (mtype == SEND_EXEC_MSG){
             mbr->send_or_copy = SEND_EXEC_MSG;
         }
         else if (mtype == COPY_EXEC_MSG){
             mbr->send_or_copy = COPY_EXEC_MSG;
             copy_warning(mbr, (int)(*(int *)mdata));

#ifndef TWO_STAGE_COPY
	     rc = move_executable(pctRank, timing_data, job_ID);

	     if (rc) yod_err("abandon load");

             copies = 1;
#endif
         }
    }

    if (timing_data){
        yodmsg("YOD TIMING: pcts form group %f\n",dclock()-td2);
    }

    /*************************************************************
    ** Send load data to the pcts.  The program arguments and
    ** executable must be sent to the root pct of each member.
    ** The environment is the same for all members so is sent
    ** only to the root pct of the entire application.
    *************************************************************/

    if (DBG_FLAGS(DBG_LOAD_2)){
        yodmsg("Send program arguments to root PCT\n");
    }
    if (timing_data){
        td2 = dclock();
    }
    for (i=0; i < init_data.n_members; i++){

        mbr = member_data(i);

        rc = send_pcts_put_message(MSG_PUT_ARGS, (CHAR *)&job_ID, sizeof(int),
              mbr->argstr, mbr->data.argbuflen, daemonWaitLimit,
              PUT_ROOT_PCT_ONLY, i);
        if (rc){
	    retryLoad = 1;
            yod_err("send user command line args to pcts");
        }
    }

    if (timing_data){
        yodmsg("YOD TIMING: pcts pull arg data %f\n",dclock() - td2);
    }

    if (envlen > 0){
        if (DBG_FLAGS(DBG_LOAD_2)){
            yodmsg("Send user environment to root PCT\n");
        }
        if (timing_data){
            td2 = dclock();
        }
        rc = send_pcts_put_message(MSG_PUT_ENV, (CHAR *)&job_ID, sizeof(int),
                  env_data, envlen, daemonWaitLimit,
                  PUT_ROOT_PCT_ONLY, 0);

        if (rc){
	    retryLoad = 1;
            yod_err("send user environment to pcts");
        }
        if (timing_data){
            yodmsg("YOD TIMING: pcts pull env data %f\n",dclock() - td2);
        }

    }

    if (ngroups > FEW_GROUPS){
        if (DBG_FLAGS(DBG_LOAD_2)){
            yodmsg("Send group list to root PCT\n");
        }
        if (timing_data){
            td2 = dclock();
        }
        rc = send_pcts_put_message(MSG_PUT_GROUPS, (CHAR *)&job_ID, sizeof(int),
                  (char *)groupList, ngroups*sizeof(gid_t), 
		  daemonWaitLimit,
                  PUT_ROOT_PCT_ONLY, 0);

        if (rc){
	    retryLoad = 1;
            yod_err("send groups to pcts");
        }
        if (timing_data){
            yodmsg("YOD TIMING: pcts pull group data %f\n",dclock() - td2);
        }

    }

    if (straceMsg){
        if (DBG_FLAGS(DBG_LOAD_2)){
            yodmsg("Send strace info to root PCT\n");
        }
        if (timing_data){
            td2 = dclock();
        }
        rc = send_pcts_put_message(MSG_PUT_STRACE, (CHAR *)&job_ID, sizeof(int),
                  (char *)straceMsg, LoadData.msg2.straceMsgLen,
		  daemonWaitLimit,
                  PUT_ROOT_PCT_ONLY, 0);

        if (rc){
	    retryLoad = 1;
            yod_err("send strace request to pcts");
        }
        if (timing_data){
            yodmsg("YOD TIMING: pcts pull strace request info %f\n",dclock() - td2);
        }
    }

    if (timing_data){
        yodmsg("YOD TIMING: Tell pcts to come get executable\n");
        td2 = dclock();
    }
    for (i=0; i < init_data.n_members; i++){

        mbr = member_data(i);

        if (mbr->send_or_copy == SEND_EXEC_MSG){

            if (DBG_FLAGS(DBG_LOAD_2)){
                yodmsg("Send program executable file to PCT %d\n",
                                 mbr->data.fromRank);
            }

            /* moved link version check to much earlier in 
               the load...
            */

	    rc = send_executable(i, job_ID);
	    if (rc){
		retryLoad = 1;
		yod_err("send executable to pcts");
	    }

        }
        else{

            if (DBG_FLAGS(DBG_LOAD_2)){
                yodmsg("Send program path name to PCT %d\n",
                                 mbr->data.fromRank);
            }

#ifdef TWO_STAGE_COPY
            rc = copy_executable(i, job_ID, rank2physnid, timing_data, uid, gid);
            if (rc){
                yod_err("copy executable to remote location");
            }
            copies = 1;
#else
	    rc = send_pcts_put_message(MSG_PUT_EXEC_PATH, (CHAR *)&job_ID, sizeof(int),
		  mbr->execPath, strlen(mbr->execPath)+1, daemonWaitLimit,
		  PUT_ROOT_PCT_ONLY, i);

	    if (rc){
		yoderrmsg("Error sending name %s to pcts",mbr->execPath);
		yod_err("abandon load");
	    }
#endif
        }
    }
    if (timing_data){
        yodmsg("       executable(s) sent to compute partition: %f\n",
                       dclock()-td2);
    }


    /*************************************************************
    ** PCTs fork the application process. Each app process sends a
    ** message to its PCT with its portal ID. Each PCT sends the
    ** portal ID and application process ID (if different) to yod.
    ** yod builds a portal ID map and sends it up to the root PCT
    ** for broadcast.
    **
    ** It is essential that no one send portals messages to the
    ** PCT during the fork call, as these messages may be lost.
    ** (Portals 2.0, linux 2.0.35 "feature").
    ** To ensure this, each PCT sends the READY token to yod and
    ** sits down for awhile.  After receiving the message to come
    ** get the global portal ID map, the PCT knows all PCTs are
    ** out of fork. It is now safe for the PCTs to resume talking
    ** to each other.
    **
    ** Of course if the user interrupts yod at this point, yod
    ** will send an ABORT message to the PCTs which may get lost
    ** if it arrives during the fork call.  Our user will hopefully
    ** notice nothing happened and interrupt yod again, this
    ** time successfully.  
    *************************************************************/

    i        = 0;

    if (pauseForDebugger){
        timeout = 90.0;     /* allow more time since app process will sleep(60) */
    }                  
    else{
        timeout = 30.0;
    }

    td2      = dclock();

    yodmsg(
    "\nAwaiting synchronization of compute nodes before beginning user code.\n");

    replies = (int *)malloc(NumProcs * sizeof(int));
    mdata   = (char *)malloc(NumProcs * SRVR_USR_DATA_LEN);

    if (!replies || !mdata){
       yod_err("Out of memory");
    }

    rc = all_get_pct_control_message(replies, mdata, SRVR_USR_DATA_LEN, timeout);

    if (rc){
        yod_err("Error awaiting synchronization messages from PCTs\n");
    }

    for (pctRank=0, c=mdata; pctRank<NumProcs; pctRank++, c+=SRVR_USR_DATA_LEN)  {

	if (replies[pctRank] == APP_PORTAL_ID){

	    if (DBG_FLAGS(DBG_PROGRESS)){
		yodmsg("Received portal ID message from node %d\n", rank2physnid[pctRank]);
	    }

	    pidmap[pctRank]  = ((appID *)c)->pid;
	    ppidmap[pctRank] = ((appID *)c)->ppid;

	    continue;
	}
	else if (replies[pctRank] == LAUNCH_FAILURE_MSG){
	     launch_failed((launch_failure *)(c));
	     yod_err("load failed, gathering post mortem");
	} 
	else{
	     yoderrmsg("want APP_PORTAL_ID, mtype = %d (%s)\n",replies[pctRank],
		       select_pct_to_yod_string(replies[pctRank]));
	     yod_err("Apparent problem with compute nodes.");
	}
    }
    free(mdata);

    if (timing_data){
        yodmsg("       compute partition synchronized: %f\n",
                       dclock()-td2);
    }
    /* Send map of portal IDs to root PCT for broadcast
    */
    if (DBG_FLAGS(DBG_LOAD_2)) {
      yodmsg( "Send portal ID map to root PCT\n");
    }
    if (timing_data) {
      td2 = dclock();
    }
    
    rc = send_pcts_put_message(MSG_PUT_PORTAL_IDS, (CHAR*)&job_ID, sizeof(int),
         (char*)ppidmap, (sizeof(ppid_type) * NumProcs),
         daemonWaitLimit, PUT_ROOT_PCT_ONLY, 0);

    if (rc){
	retryLoad = 1;
        yod_err("send portal ID map to pcts");
    }

    if (timing_data){
      yodmsg("YOD TIMING: root pct pulls portal ID map %f\n",dclock()-td2);
    }
    /* PCTs supply the portal ID map to the application processes, the
       app processes complete pre-main initialization, and send a READY
       message to PCT. The PCTs synchronize, and root PCT sends a
       READY mesaage to yod.
    */

    if (DBG_FLAGS(DBG_LOAD_2)){
        yodmsg( "Await READY message from root PCT\n");
    }

    rc = await_pct_msg(&mtype, &mdata , NULL, timeout);
    
    if ( (rc < 0) || (mtype != APP_READY_MSG)){
         if (mtype == LAUNCH_FAILURE_MSG){
             launch_failed((launch_failure *)mdata);
         }
         else{
	     retryLoad = 1;
             yodmsg( "Apparent problem with compute nodes.\n");
             yodmsg( "Please notify system administration.\n");
         }
         yod_err("aborting load");
    }
    if (DBG_FLAGS(DBG_PROGRESS)){
        yodmsg("Received READY message from node %d\n", rank2physnid[0]); 
    }
    if (timing_data){
        yodmsg(
        "YOD TIMING: %f elapsed from start of executable fanout to apps ready\n",
                    dclock()-td2);
    }

    /* "yod -a" displays PCTs allocated and prompts to send application
       along to user code
    */

    init_TotalView_symbols( NumProcs, rank2physnid, pidmap );

    if (display_allocation){

        display_pct_list();

        yodmsg( "Proceed to application main? (y/n) \n");
        scanf("%s", ch);

        if ( (ch[0] != 'y') && (ch[0] != 'Y')){
            force_kill();     
        }
    }

    if ( MPIR_being_debugged ) {
	MPIR_debug_state = MPIR_DEBUG_SPAWNED;
        MPIR_Breakpoint();
    }

    if ( attach_gdb ) {
      display_pct_list();
      yodmsg( "Press <ENTER> to proceed to main:\n");
//      scanf("%s", ch);
      getchar();
    }

    app_main = 1;

    rc = srvr_send_to_control_ptl(LoadData.thePcts[0], PPID_PCT,
           PCT_LOAD_PORTAL, MSG_GO_MAIN,
           (CHAR *)&job_ID, sizeof(int));

    if (rc < 0){
        yod_err("sending go main message");
    }
    yodmsg("Application processes begin user code.\n");

    /*
    ** Generate user logs like Tflops logs.  Send these to bebopd
    ** for logging.
    */
    pw = getpwuid(euid);

    for (i=0, cmd_line[0]=0; i<argc; i++){
       strcat(cmd_line, argv[i]); strcat(cmd_line, " ");
       if ((i+1 < argc) && 
	   (strlen(cmd_line) + strlen(argv[i+1]) >= USERLOG_MAX))
	   break;
    }
    write_node_list(rank2physnid, NumProcs, nlist, USERLOG_MAX);

    mkdate(time(NULL), appStartTime);

    snprintf(log_record, USERLOG_MAX-1,
        ">> Starting Cplant user %s | base %d | start %s | "
	"cmd_line %s | nnodes %d | Nodes %s | PBS #%d | Cplant #%d",
           ((pw && pw->pw_name) ? pw->pw_name : "unknown"), rank2physnid[0], appStartTime,
           cmd_line, NumProcs, nlist, init_data.session_id, job_ID);

    send_to_bebopd(log_record, strlen(log_record)+1,
	    YOD_USERLOG_RECORD, BEBOPD_GET_USERLOG_RECORD);

    log_msg(log_record); /* send a copy to syslog */
    start_logged = 1;

    /*************************************************************
    ** Apps are proceeding to main.  Go in to service loop.
    *************************************************************/

    if (timing_data){
        yodmsg("YOD TIMING: total load %f\n",dclock()-td0);
    }

    startTime = time(NULL);

    prog_phase = 2;
 
    perform_service();

    /*
    ** yod is done
    */
    finish();
    return 0;
}
/*
********************************************************************************
  Command line processing
********************************************************************************
*/
void
update_Dbgflag(const char *optarg)
{
    if (!optarg) return;

    if (!strcmp(optarg, "io")){   
        /* list application IO requests */
        Dbgflag |= DBG_IO_2;
    }
    else if (!strcmp(optarg, "iomore")){ 
        /* detailed handling of app IO requests */
        Dbgflag |= (DBG_IO_1 | DBG_IO_2);
    }
    else if (!strcmp(optarg, "memory")){ 
        /* list all buffers allocated */
        Dbgflag |= (DBG_MEMORY);
    }
    else if (!strcmp(optarg, "load")){   
        /* load protocol steps */
        Dbgflag |= DBG_LOAD_2;
    }
    else if (!strcmp(optarg, "loadmore")){ 
        /* detailed handling of load protocol steps */
        Dbgflag |= (DBG_LOAD_1 | DBG_LOAD_2);
    }
    else if (!strcmp(optarg, "progress")){ 
        /* display app process "done" messages */
        Dbgflag |= DBG_PROGRESS;
    }
    else if (!strcmp(optarg, "alloc")){   
        /* node allocation details */
        Dbgflag |= DBG_ALLOC;
    }
    else if (!strcmp(optarg, "hetero")){   
        /* heterogeneous load details */
        Dbgflag |= DBG_HETERO;
    }
    else if (!strcmp(optarg, "pbs")){   
        /* PBS information */
        Dbgflag |= DBG_PBS;
    }
#ifdef SUPPRESS_FORTRAN_RTL
    else if (!strcmp(optarg, "rtl")){   
        /* Display the Fortran Run Time Library messages from compute nodes */
        Dbgflag |= DBG_FORRTL;
    }
#endif
    else if (!strcmp(optarg, "failure")){ 
        /* display app process load progress and "done" messages */
        Dbgflag |= DBG_FAILURE;
    }
    else if (!strcmp(optarg, "debug")){ 
        /* display efforts to obtain application debugging data */
        Dbgflag |= DBG_DBG;
    }
    else if (!strcmp(optarg, "bebopd")){ 
        /*  bebopd communications */
        Dbgflag |= DBG_BEBOPD;
    }
    else if (!strcmp(optarg, "comm")){ 
        /* communications structures */
        Dbgflag |= DBG_COMM;
    }
    else if (!strcmp(optarg, "environ")){ 
        /* environment variables */
        Dbgflag |= DBG_ENVIRON;
    }
    else if (!strcmp(optarg, "phase1")){ 
        /* debugging only BEFORE the application starts */
        Dbgflag |= DBG_PHASE_1;
    }
    else if (!strcmp(optarg, "phase2")){ 
        /* debugging only AFTER the application starts */
        Dbgflag |= DBG_PHASE_2;
    }
    return;
}
static struct option yod_options[] =
{
    {"attach", no_argument, 0,       'a'},
    {"bt", no_argument, 0,           'b'},
    {"debug", required_argument, 0,  'd'},
    {"g", no_argument, 0,            'g'},
    {"nid", required_argument, 0,    'n'},
    {"pid", required_argument, 0,    'p'},
    {"show", no_argument, 0,         's'},
    {"xxx", no_argument, 0,          'x'},
    {"alloc", no_argument, 0,            'A'},
    {"batch", no_argument, 0,            'B'},
    {"comm", required_argument, 0,       'C'},
    {"Debug", no_argument, 0,            'D'},
    {"file", required_argument, 0,       'F'},
    {"help", no_argument, 0,             'H'},
    {"interactive", no_argument, 0,      'I'},
    {"kill", no_argument, 0,             'K'},
    {"nice", no_argument, 0,             'l'},
    {"Log", no_argument, 0,              'L'},
    {"NOBUF", no_argument, 0,            'N'},
    {"quiet", no_argument, 0,            'Q'},
    {"x", no_argument, 0,                'R'},
    {"timing", no_argument, 0,           'T'},
    {"vhelp", no_argument, 0,            'V'},
    {"list", required_argument, 0,       'X'},
    {"sz", required_argument, 0,         'Z'},
    {"size", required_argument, 0,       'Z'},
    {"sleep", required_argument, 0,            'M'},
    {"strace", required_argument, 0,            '1'},
    {"straceoptions", required_argument, 0,     '2'},
    {"stracenodes", required_argument, 0,       '3'},
    {0,0,0,0}
};
extern CHAR *optarg;
extern int optind, optopt;
 
static int
get_yod_options(int argc, CHAR *argv[])
{
int opttype;
int val;
 
    optind = 0;

    while(1){
 
        opttype = getopt_long_only(argc, argv, "+", yod_options, 0);
 
        if (opttype == EOF){
            break;
        }

        switch (opttype){
            case 'H':
                usage_message();
                finish();

            case 'V':
                verbose_usage_message();
                finish();

            case 'A':
                if (attach_gdb != TRUE) {
                  display_allocation = TRUE;
                }
                break;

            case 'a':
                attach_gdb = TRUE;
                if (display_allocation == TRUE) {
                  display_allocation = FALSE;
                }
                if (get_bt == TRUE) {
                  yodmsg("WARNING: -bt and -attach incompatible; -attach taking precedence\n");
                  get_bt = FALSE;
                }
                break;

            case 'x':
              /* bypass link version check */
                bypass_link_version_check = 1;
                break;
 
            case 'Z':
                nprocs = atoi(optarg);

                if ((nprocs <= 0) || (nprocs > MAX_PROC_PER_GROUP)){
                    yoderrmsg("invalid size: try  1 through %d nodes\n",
                           MAX_PROC_PER_GROUP);

                    option_lines();
                    return -1;
                }
                if ((pbs_job != NO_PBS_JOB) && (nprocs > nprocs_limit)){
                    yoderrmsg(
                    "Compute nodes requested (%d) exceed PBS allocation (%d).\n",
                     nprocs, nprocs_limit);
                    return -1;
               }

               break;
 
            case 'C':
                comm_space = atoi(optarg);
                break;

            case 'l':
		priority = SCAVENGER;
		break;
 
            case 'K':
                /* the autokill option. this is referred to in
                   abend_notify() on notification of abornmal
                   termination of one of the processes. what
                   we'd like to have happen in certain instances
                   is that the other processes are killed automatically
                   w/o having to enter into a dialogue w/ yod. for
                   instance we may be doing some automated testing
                   or running in batch node. abend_notify() will
                   check if !interactive (batch mode) or autokill
                   is set and do end_game() if so. having an autokill
                   allows us to invoke this behavior w/o being in
                   batch mode.
                */
                autokill=1;
                break;

            case 'N':
                /* the NOBUF option. 
                   for the most part, yod uses fprintf() for output. 
                   but for output from an application it uses write().
                   depending on how you call yod (interactively or not)
                   these two forms of output may be buffered differently
                   leading to some unexpected results (the application's
                   output preceding any of yod's output for example in
                   case yod is run noninteractively so the fprinf()s
                   are buffered while write() is not...)
                   to force consistency, we may use this option to turn
                   off buffering of stdout. more precise control could
                   be obtained my calling setvbuf().
                */
                setbuf(stdout,NULL);
                break;
 
            case 'p':
                altBpid = atoi(optarg);
                break;
 
            case 'n':
                altBnid = atoi(optarg);
                break;
 
            case 'g':
                yodmsg("the -g   option in yod is deprecated: use\n");
                yodmsg("the -bt  option for back trace support\n");
                return -1;
                break;

            case 'b':
                if (attach_gdb == TRUE) {
                  yodmsg("WARNING: -bt and -attach incompatible; -attach taking precedence\n");
                }
                else {
                  get_bt = TRUE;
                }
                break;
 
            case 's':
                show_link_versions = TRUE;
                break;
 
            case 'L':
                log_startup_actions = TRUE;
                break;
 
            case 'M':
	        val = atoi(optarg);
                pauseForDebugger = val;
                break;

            case 'X':

                if (pbs_job != NO_PBS_JOB){
                    yoderrmsg(
                    "Node list requests can not be honored for PBS jobs.\n");
                    yoderrmsg(
                     "Rerun your job without requesting particular nodes.\n");
                    yoderrmsg(
                     "Try using just the \"-sz\" option to request nodes.\n");

                    return -1;
                }
                else{
                    node_string = optarg;

                    listsize = parse_node_list(optarg, NULL, 0,
                                               0, MAX_NODES-1);

                    if (listsize <= 0){
                        node_list_lines();
                        return -1;
                    }
                }
 
                break;
 
            case 'T':
                timing_data = 1;
                break;
 
            case 'I':
                interactive = 1;
                break;
 
            case 'Q':
                quiet = 1;
                break;
 
            case 'B':
                interactive = 0;
                break;
 
            case 'F':
                done_file = optarg;
                break;
 
            case 'D':
                Dbglevel++;
                
                break;

            case 'd':
                update_Dbgflag(optarg);
                
                break;

            case 'R':            /* not for ordinary users, it's a signal */
                DebugSpecial = 1;/* to PCT to execute special debugging code */
                break;
 
            case '?':
                yodmsg("Invalid options\n");
                option_lines();
                return -1;

            case '1':
	        straceDirectory = optarg;
		straceDirLen = strlen(optarg);
                break;
		
            case '2':
	        straceOptions = optarg;
		straceOptLen = strlen(optarg);
                break;

            case '3':
	        straceNodeList = optarg;
		straceListLen = strlen(optarg);
                break;
 
            default:
                break;
        }
    }
    if (Dbglevel > 0){
        update_Dbgflag("load");
        update_Dbgflag("alloc");
        update_Dbgflag("hetero");
        update_Dbgflag("pbs");

        if (Dbglevel > 1){
            update_Dbgflag("loadmore");
            update_Dbgflag("comm");
            update_Dbgflag("bebopd");

            if (Dbglevel > 2){
                update_Dbgflag("failure");
                update_Dbgflag("progress");
                update_Dbgflag("memory");
            }
        }
    }
    /*
    ** Done with yod arguments, what follows is program
    ** name and program arguments, or load file name.  
    ** "+" in option string causes getopt_long_only to 
    ** stop when it got to first non-argument, and optind 
    ** is set to this index.
    */
 
    if (argc <= optind){
        yodmsg( "Big problem: program or load file name is missing.\n");
        option_lines();
        return -1;
    }
    fname_argv = argv + optind;
    fname_argc = argc - optind;
 
    /*
    ** Do some sanity checks
    */
    if ((straceOptions || straceNodeList) && !straceDirectory){
        yodmsg("You specified -straceoptions or -stracenodes.\n");
	yodmsg("You must supply a directory to which strace output will go.\n");
	yodmsg("Use the -strace option to do this.  Ask system administration for\n");
	yodmsg("advice about writable directories mounted on the compute nodes.\n\n");
	strace_option_lines();
    }

    if (!interactive && display_allocation){
        yodmsg(">>>\n");
        yodmsg("Turned off display of node allocation in batch mode.\n");
        yodmsg("Requires user to specifically send processes to main.\n");
        yodmsg(">>>\n");
        display_allocation = 0;
    }
    /*
    ** If "-sz {n} -l {nodelist}" then we'll try to get n nodes from nodelist.
    ** "n" can be less than the number of nodes in the nodelist.  If no -sz
    ** specified, we try to get all the nodes in the nodelist.  If neither
    ** -sz nor -l are specified, we try to get one node from anywhere.
    */
    if (listsize > 0){

        if (nprocs > listsize){
            yodmsg(
      "Requested partition size of %d will not fit in list of requested nodes\n",
                     nprocs);
            option_lines();
            node_list_lines();
            return -1;
        }
    }
    
    return 0;
}
static void
node_list_lines()
{
yodmsg("\n");
yodmsg("A node-list is a comma separated list of nodes or node ranges.  A node\n");
yodmsg("range is two node numbers separated by one or more dots.  No white space\n");
yodmsg("in node-list please.  EXAMPLE: 1,5,7,12..18,100..150,152\n\n");
yodmsg("Your [n] processes will run on the first [n] free nodes found in the list.\n");
yodmsg("Application ranks will observe the order of the list.  In the example above\n");
yodmsg("rank 0 will be on node 1, rank 1 on node 5, and so on.\n\n");
}
void
load_file_lines()
{
yodmsg("\n");
yodmsg("Instead of listing a single program-name, a load file can be listed which\n");
yodmsg("contains the names of executables and the number of nodes on which each should\n");
yodmsg("run.  It would look like this:\n\n");
yodmsg("\t#\n");
yodmsg("\t# here are some comments, ignored by yod\n");
yodmsg("\t#\n");
yodmsg("\t-sz 10 program1 arg1 arg2\n");
yodmsg("\t-sz 50 program2 arg3 arg4\n");
yodmsg("\t-sz 150 program3 arg5\n\n");
yodmsg("Instead of or in addition to the \"-sz\" argument, a \"-l node-list\" can\n");
yodmsg("appear before the program name.  Defaults are the same as described above.\n\n");
yodmsg("A \"load-file\" is recognized because it does not have execute permission.\n");
yodmsg("Consequently, an executable must have execute permission.\n\n");
}
static void 
option_lines()
{
yodmsg("\n");
yodmsg("YOD USAGE:\n\n");
yodmsg("\tyod {yod options} program-name {program arguments}\n");
yodmsg("\t\t\t\t-- OR --\n");
yodmsg("\tyod {yod options} load-file-listing-program-names-and-arguments\n\n");
yodmsg("yod options are:\n\n");
yodmsg("-l [node-list] Allocate compute partition from these nodes only.  If not spec-\n");
yodmsg("               ified, nodes will be allocated from pool of all available nodes.\n");
yodmsg("-sz [n]       Number of nodes requested.  If not specified, assume all nodes in\n");
yodmsg("              the node-list, or 1 node from anywhere if there's no node-list.\n");
yodmsg("-alloc        After load, display allocated nodes and wait for my OK to start\n");
yodmsg("              the parallel code.\n");
yodmsg("-attach       After load, display allocated nodes and wait for my OK to start\n"); 
yodmsg("              processes. Intended for use with GDB proxy utility, cgdb,\n");
yodmsg("              although essentially the same as -alloc.\n");
yodmsg("-batch        yod is NOT running in interactive mode.\n");
yodmsg("-interactive  yod IS running in interactive mode (default).\n");
yodmsg("-kill         Kill my job if any processes terminate abnormally.\n");
yodmsg("-nice         Kill my job (with SIGTERM/wait 5 min./SIGKILL) if another job needs the nodes.\n");
yodmsg("-D            Display load debugging information.  Repeat to increase quantity.\n");
yodmsg("-bt           Notify yod that the parallel application was compiled with debugging.\n");
yodmsg("              symbols.  yod will provide a stack trace if the application faults.\n");
yodmsg("-file [name]  write final status messages to file [name] instead of stdout.\n");
yodmsg("-show         yod will show link version nos. for both yod and executable\n");
yodmsg("-quiet        yod will not display any status or error messages.\n");
yodmsg("-help         display this message\n");
yodmsg("-vhelp        display more verbose help\n\n");
}
static void 
debug_option_lines()
{
yodmsg("\n");
yodmsg("-d io          Display application IO requests to yod.\n");
yodmsg("-d iomore      Display details of dispatch of IO requests.\n");
yodmsg("-d memory      Display all buffers allocate by yod.\n");
yodmsg("-d load        Display the steps of the load protocol.\n");
yodmsg("-d loadmore    Display the load protocol in great detail.\n");
yodmsg("-d alloc       Display nodes allocated to the job.\n");
yodmsg("-d hetero      Display the heterogeneous load information.\n");
yodmsg("-d pbs         Display the PBS information.\n");
yodmsg("-d environ     Display yod's environment variables.\n");
#ifdef SUPPRESS_FORTRAN_RTL
yodmsg("-d rtl         Display the Fortran run time library messages from compute nodes.\n");
#endif
yodmsg("-d progress    Display the progress of the application through load and termination.\n");
yodmsg("-d failure     Display all launch failure information.\n");
yodmsg("-d debug       Display efforts to obtain debugging data for application processes.\n");
yodmsg("-d bebopd      Display yod interactions with bebopd.\n");
yodmsg("-d comm        Display information about portals setup.\n");
yodmsg("-d phase1      Display information only until application starts.\n");
yodmsg("-d phase2      Display information only after application starts.\n");
}
static void
strace_option_lines()
{
yodmsg("-strace [directory]             Run application process with strace,\n");
yodmsg("                                and list output to [directory].\n");
yodmsg("-straceoptions [\"option list\"]  Use these strace options (in addition to\n");
yodmsg("                                 \"-o [directory]/strace.out.jobid.rank\").\n");
yodmsg("-stracenodes   [rank-list]      Run strace on these ranks, default is rank 0 only.\n");  
}
static void 
more_option_lines()
{
yodmsg("\n");
yodmsg("-Log          Start up actions (pre user code) will be logged on compute node.\n");
yodmsg("-sleep 1      Application process will sleep 60 seconds right after fork.\n");
yodmsg("-sleep 2      Application process will sleep 60 seconds just before exec.\n");
yodmsg("-sleep 3      Application process will sleep 60 seconds right after entering Cplant startup code.\n");
yodmsg("-sleep 4      Application process will sleep 60 seconds just before proceeding to main.\n");
yodmsg("-pid [n]      Override default portal pid of bebopd, for system testing only.\n");
yodmsg("-nid [n]      Override node ID of bebopd, for system testing only.\n");
yodmsg("-NOBUF        All yod\'s output (it\'s own and the app\'s) will be unbuffered.\n");
yodmsg("-xxx          Super-secret option to omit link version check and load app anyway.\n");
strace_option_lines();
}
static void
verbose_usage_message()
{
    option_lines();
    debug_option_lines();
    more_option_lines();
    node_list_lines();
    load_file_lines();
}
static void
usage_message()
{
    option_lines();
}
/*
********************************************************************************
** Await requests from compute partition.
********************************************************************************
*/
static void
perform_service()
{
app_proc_done *doneMsg;
launch_failure *failMsg;
INT32 mtype, rc, rank, srcNid, srcPid, ioErrReported;
CHAR *mdata;
hostCmd_t *rmsg;
INT32 tbl_offset; 
time_t pctnoise, appnoise, now;
control_msg_handle io_handle;

#ifdef DEBUG_ENDGAME
    yodmsg(">>>>perform_service, proc_done_count %d pct_count %d\n", 
                  proc_done_count, pct_count);
#endif

    in_perform_service = 1;

    appnoise = pctnoise = 0;

    ioErrReported = 0;

    if (pct_count == 0){
        return;
    }
 
    while (1){

	if ( MPIR_being_debugged && (MPIR_debug_state == 0 ) ) {
	    MPIR_debug_state = MPIR_DEBUG_SPAWNED;
            MPIR_Breakpoint();
	}

        if ((!interactive || autokill) && tabort &&
             ((time(NULL) - tabort) > daemonWaitLimit)){

            force_kill();
        }

        if (pctnoise || appnoise){
	    now = time(NULL);
            /*
	    ** any activity in the past two seconds?
	    ** if not, let's yield processor.
	    */
	    if ( pctnoise && ((now - pctnoise) > 2)){
		pctnoise = 0;
	    }
	    if ( appnoise && ((now - appnoise) > 2)){
		appnoise = 0;
	    }
	}
        if (!pctnoise && !appnoise){  
            usleep(200000);
            //sched_yield();
        }

        rc = get_pct_control_message(&mtype, &mdata, NULL);

        if (rc < 0){
            yoderrmsg( "Error on incoming PCT control portal\n");
            force_kill(); 
        }
 
        if (rc == 1){

           pctnoise = time(NULL);
 
           switch (mtype){

              case PROC_DONE_MSG:

                  doneMsg = (app_proc_done *)mdata;

                  if ((doneMsg->nid >= 0) && (doneMsg->nid < MAX_NODES) &&
		      (doneMsg->job_id == job_ID) ){

                    rank = physnid2rank[doneMsg->nid];

                    memcpy(done_status + rank, doneMsg, sizeof(app_proc_done));
                    proc_done_count++;

                    if (DBG_FLAGS(DBG_PROGRESS)){
                        yodmsg("Done message from %d\n",doneMsg->nid);
                        yodmsg("    job id %d, nid %d pid %d\n",
                             doneMsg->job_id, doneMsg->nid, doneMsg->pid);
                        yodmsg("    exit_code %d elapsed %d status %x bt_size %d\n",
                             doneMsg->final.exit_code, doneMsg->elapsed,
                             doneMsg->status, doneMsg->bt_size);

                    }
                    abend_notify(doneMsg->final, doneMsg->nid, doneMsg->rank);

                    if (get_bt && (doneMsg->bt_size > 0)){
                        if (DBG_FLAGS(DBG_DBG)) {
                          yodmsg("yod: getting stack trace: rank= %d,"
                                 "size of backtrace= %d\n", 
                                 rank, doneMsg->bt_size);
                        }
                        backTraces[rank] = 
                          get_stack_trace(rank, doneMsg->bt_size,
                                      doneMsg->job_id, daemonWaitLimit);

                        if (backTraces[rank] == NULL){
                              yod_ignore("couldn't get stack trace");
                        }
                    }
                  }
                  else{
                    yoderrmsg("Ignoring PROC_DONE_MSG from %d for job %d\n",
                                doneMsg->nid, doneMsg->job_id);
                    break;
                  }
 
                  /*
                  ** notify pcts when all processes in app have
                  ** exited
                  */
                  if ((proc_done_count + failCount) == pct_count){

                  /* recognize this event before finishing */
#if 0
                      rc = srvr_send_to_control_ptl(LoadData.thePcts[0],
		             PPID_PCT, PCT_LOAD_PORTAL, MSG_ALL_DONE, 
                             (CHAR *)&job_ID, sizeof(int));
 
                      if (rc < 0){
                          yoderrmsg( 
                           "Error sending ALL_DONE msg to compute partition\n");
                          yoderrmsg( 
                           "Please contact system administration.\n");
                          finish(); 
                      }
#endif
		      if (timing_data){
		  	  td2 = dclock();
		      }
                      /*
		      ** All PCTs have reported with either a launch
		      ** failure message or a process done message.
		      ** The parallel application is done.
		      */
		      return;
                  }
 
                  break;
 
              case LAUNCH_FAILURE_MSG:   /* failed before app got to main */
 
                  failMsg = (launch_failure *)mdata;

                  if ((failMsg->nid >= 0) && (failMsg->nid < MAX_NODES) &&
		       (failMsg->job_id == job_ID) ){

		      launch_failed(failMsg);
		      if (!interactive || (pbs_job == PBS_BATCH)){
			  force_kill();        
		      }
		      if ((failCount + proc_done_count) == pct_count){
			  /*
			  ** all nodes are done
			  */
			  force_kill();
		      }
                  }
		  else{
                    yoderrmsg(
		    "Ignoring LAUNCH_FAILURE_MSG from %d for job %d\n",
                                failMsg->nid, failMsg->job_id);
                    break;
		  }

                  break;
 
              default:
                  yoderrmsg(
                 "Ignoring unexpected message type %s at pct control portal\n",
                         select_pct_to_yod_string(mtype));
                              
                  break; 
 
            }
        }

        if (!app_main                         /* app hasn't started */
	    || (app_initiated_kill >= 0))   /* app procs can be killed in mid IO operation */

	    continue;

        rc = get_app_control_message(&io_handle);
 
        if (rc < 0){
            yoderrmsg("Error on incoming app control message portal.");
            force_kill();
        }
 
        if (rc == 1){

            appnoise = time(NULL);

	    srcNid = SRVR_HANDLE_NID(io_handle);
	    srcPid = SRVR_HANDLE_PID(io_handle);
	    rmsg   = (hostCmd_t *)SRVR_HANDLE_USERDEF(io_handle);

            if ((srcNid < 0) || (srcNid >= MAX_NODES)){
                yod_ignore("invalid src nid in app control message hdr");
                continue;
            }

            rank = physnid2rank[srcNid];
 
            if ((rank < 0) ||
                (srcPid != ppidmap[rank]))
            {       
 
                yod_ignore("invalid src pid in app control message hdr");
                continue;
            }
 
            if ((rmsg->type < FIRST_CMD_NUM) ||
                (rmsg->type > LAST_CMD_NUM)      ){
 
                yod_ignore("bad request type in app control message");
                continue;
 
            }
            tbl_offset= (rmsg->type - FIRST_CMD_NUM);
 
            if (DBG_FLAGS(DBG_IO_2)){
 
                yodmsg("rec'd req type -0x%x (%s) from %d/%d/%d\n",
                  -(rmsg->type), CMDstrings[tbl_offset],
                  srcNid, ppidmap[rank], pidmap[rank]); 
            }
 
            host_cmd_handler_tbl[tbl_offset](&io_handle);

            if (ioErrno) {

	        if (ioErrReported <= 5){
                    add_to_log_error(CPstrerror(ioErrno));
	            yoderrmsg("I/O error (node %d portal ID %d rank %d): %s\n",
	              srcNid,srcPid,rank,CPstrerror(ioErrno));
                    yoderrmsg("Interrupt yod to kill the parallel application.");
                    if (ioErrReported == 5){
                         yoderrmsg("Further error messages suppressed...\n");
	            }
                    ioErrReported++;	
	        }

                ioErrno = 0;
            }
	    free_app_control_message(&io_handle); 
        }
    }
}
/*
********************************************************************************
** Signal handlers
********************************************************************************
*/
static void
setup_sig_handlers(void)
{
int sig;
 
    for (sig = 0; sig < NSIG; sig++){
 
        if ( (sig == SIGKILL) ||
             (sig == SIGSTOP) ||
             (sig == SIGTSTP) ||
             (sig == SIGTTIN) ||
             (sig == SIGTTOU) ||
             (sig == SIGPOLL) ||
             (sig == SIGTRAP) ||
             (sig == SIGURG)  ||
             (sig == SIGCONT) ||
             (sig == SIGCHLD) ||
             (sig == SIGUSR1) ||
             (sig == SIGUSR2) ||
             (sig == SIGWINCH)
                                ){
 
            continue;
        }
 
	/*
	** force the system V semantics of signal(), this used
	** to be the default, but for some mysterious reason
	** we started getting bsd semantics, not what we want
	*/
        sysv_signal(sig, signal_handler);
    }

    sysv_signal(SIGUSR1, propagate_user_signal);
    sysv_signal(SIGUSR2, propagate_user_signal);
}
static void
propagate_user_signal(int sig)
{
send_sig sigType;
int rc;

    if (!app_main){
        yodmsg( "Ignoring SIGUSR (%d), app not started yet.\n",sig);
        return;
    }
    sigType.job_ID = job_ID;
    sigType.type = ( (sig == SIGUSR1) ? 1 : 2);

    yodmsg( "Propagating SIGUSR%d to application\n",sigType.type);

    rc = srvr_send_to_control_ptl(LoadData.thePcts[0], PPID_PCT,
         PCT_LOAD_PORTAL, MSG_SEND_SIGUSR, 
         (CHAR *)&sigType, sizeof(send_sig));

    if (rc){
        yod_err(
        "Can't send to compute partition - notify system administration.");
    }

    setup_sig_handlers();

    return; 
}
static void
signal_handler(int sig)
{
static int intLogged=0;
 
    yoderrmsg("yod received %s (%d)\n",select_signal_name(sig),sig);

    if (sig == SIGINT){  /* owner wants to interrupt app */
	sysv_signal(sig, signal_handler);

        if (!intLogged){
	    if (prog_phase < 2){ 
	        add_to_log_error("load killed by yod interruption"); 
	    }
	    else{
	        add_to_log_error("app killed by yod interruption"); 
	    }
	    intLogged = 1;
	}
    }
    else{
	add_to_log_error("yod got a %s ",select_signal_name(sig));
    }
    
    if (app_main){ /* apps may be in user code, interrupt them nicely */
        end_game();
    }
    else{          /* just kill the app, if any, and reset the pct */
        force_kill();
    }

}  /* end of signal_handler() */

/************************************************************************
**  error and normal completion scenarios
************************************************************************/

static int
display_done_messages()
{
int i, ii, iii, nmembers;
app_proc_done *dmsg;
int hours, minutes, seconds;
FILE *fp;
loadMembers *mbr;
int retValue, len;
     
    /* Default value */
    retValue = FALSE;

    if (quiet && !done_file){
        return 0;
    }

    if (done_file){
        fp = fopen(done_file, "a");

        if (!fp){
            yoderrmsg("Unable to open %s\n",done_file);
            yoderrmsg("Writing completion messages to stdout.\n");
            fp = (FILE *)stdout;                
        }
        fprintf(fp,"\n\n");
        for (i=0; i<Argc; i++){
            fprintf(fp,"%s ",Argv[i]);
        }
        fprintf(fp,"\n\n");
    }
    else {
        fp = (FILE *)stdout;
    }

    nmembers = total_members();

    phys2name(0);

    fprintf(fp,"\n");
    if (proc_done_count < NumProcs){
        fprintf(fp,
        "   ********* Responding compute nodes **********\n");
    }
    fprintf(fp, " Name");
    for (i=0; i<maxnamelen+1; i++) {
      fprintf(fp, " ");
    }
    fprintf(fp, "Rank  Node   SPID   Elapsed  Exit  Signal\n");
    for (i=0; i<maxnamelen+5; i++) {
      fprintf(fp, "-");
    }
    fprintf(fp, " ----  ----  -----  --------  ----  ------\n");

    for (i=0; i<nmembers; i++){

        mbr = member_data(i);

        if (nmembers > 1){
           if ((i == 0) || (mbr->pname != member_data(i-1)->pname)){
	       yodmsg("(%s)\n", mbr->pname);
           }
	}

        for (ii=mbr->data.fromRank; ii<=mbr->data.toRank; ii++){

            dmsg = done_status + ii;

            if (dmsg->pid){
                hours = dmsg->elapsed / 3600;
                minutes = (dmsg->elapsed % 3600) / 60;
                seconds = dmsg->elapsed % 60;

                fprintf(fp, " ");
                len = print_node_name(dmsg->nid, fp);

                for (iii=0; iii<(maxnamelen-len+4); iii++) {
                  fprintf(fp, " "); 
                }
                
                fprintf(fp," %4d  %4d  %5d  %02d:%02d:%02d",
                         ii, dmsg->nid, pidmap[ii], 
                         hours, minutes, seconds);
         
                if (! (dmsg->status & STARTED)){
                    retValue = TRUE;
                    fprintf(fp,"  code never started");
                }
                else if (!(dmsg->status & SENT_TO_MAIN)){
                    retValue = TRUE;
                    fprintf(fp,"  faulted in startup, before user code");
                }
                else if (dmsg->status & CHILD_NACK){
                    retValue = TRUE;
                    fprintf(fp,"  exited in startup, before user code");
                }
                else if (dmsg->status & SENT_KILL_2){
                    retValue = TRUE;
                    fprintf(fp,"  killed by SIGKILL request");
                }
                else if (dmsg->status & SENT_KILL_1){
                    retValue = TRUE;
                    fprintf(fp,"  killed by SIGTERM request");
                }
                else{
/* Check for any application errors (exit codes or signals) */
                    if ( dmsg->final.exit_code )
                        retValue = TRUE;

                    fprintf(fp,"   %3d",dmsg->final.exit_code);

                    if (dmsg->final.term_sig){
                        fprintf(fp,"   %d, %s",
                           dmsg->final.term_sig, 
			   select_signal_name(dmsg->final.term_sig));
                    }
                    else{
                        fprintf(fp,"   none");
                    }
                }
                fprintf(fp,"\n");

                if (backTraces[ii]){
                     fprintf(fp," /\n");
                  fprintf(fp,"%s",backTraces[ii]);
                  fprintf(fp,"- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
                }
            }
        }
    }
    if (done_file){
        fclose(fp);
    }
    return retValue;
}

static void
display_silent_pcts()
{
int i, ii;
     
    /* Default value */

    if (reset_load) return;

    /*
    ** Normally, a PCT ultimately sends to yod either a "launch failure" 
    ** message (if it can't start the user process) or a "process done" 
    ** message when the user process completes.  An exception occurs if
    ** the compute node was reset with "pingd -reset" or by yod sending
    ** a MSG_ABORT_RESET.  In that case the PCT merely resets itself to
    ** free without attempting to inform yod.
    **
    ** PCTs that were not reset by yod, and reported neither a "launch
    ** failure" or a "process done" are reported here.  The node or the
    ** PCT may have died.
    */
    if ((failCount + proc_done_count) < NumProcs){

        start_notify("pcts failing to report");

	for (i=0,ii=0; i<NumProcs; i++){

            if ((done_status[i].job_id == 0) && (fail_status[i].errType == 0)){

		if (ii > 5){
		    sprintf(msgbuf,
		    "Remaining messages suppressed.\n");

		    break;
		}
		else{
		    sprintf(msgbuf,
	    "job %d, NO COMPLETION STATUS FROM node %d, application rank %d\n",
			     job_ID,rank2physnid[i],i);

		    yoderrmsg("%s",msgbuf);
		    log_msg(msgbuf); /* send a copy to syslog */
		    notify(msgbuf);

		    ii++;
		}

            }
        }
        end_notify();
    }

    return;
}
static int notifyCount = 0;
static int notifyDisplay = 1;

static void
abend_notify(final_status final, int node, int rank)
{
int exit_code, term_sig;


    exit_code = (int)final.exit_code;
    term_sig  = (int)final.term_sig;

    if (term_sig){
	/*
	** Could be different on different nodes, like job could
	** fault on one node, then owner kills it.  This will
	** set terminator to that of last node to report 
	** termination.
	*/
	terminator = final.terminator;
    }

    if ((notifyCount == notifyDisplay) ||
        ((exit_code == 0) && (term_sig == 0)) ){

        return;
    }

    if (term_sig){
        yodmsg("\n>>> Terminating signal %s on node %d w/ rank %d<<<\n",
                     select_signal_name(term_sig), node, rank);


	if (final.terminator == PCT_ADMINISTRATOR){
            yodmsg(
	    ">>> Process terminated by Cplant administrator or PBS daemon.\n");
	}
        if (abort_load){
	    add_to_log_error("app killed by owner or admin");
        }
        else{
            add_to_log_error("%s on node %d", select_signal_name(term_sig), node);
	}
    } 
    if (exit_code){
        yodmsg("\n>>> Exit code %d on node %d <<<\n",
                     exit_code, node);
    }

    notifyCount++;

    if (notifyCount == notifyDisplay){
        yodmsg(">>> further messages suppressed\n");
    }

    /* in case of batch mode, kill all processes eventually... */
    if ( !interactive || autokill ) {
        end_game();
    }
    else if (!abort_load){
        yodmsg("\n>>> if your job appears to be hung, then\n");
        yodmsg(">>> at this point you may want to use Control-C\n");
        yodmsg(">>> to terminate your job...\n");

        yodmsg("\n>>> or you may use pingd to determine the\n");
        yodmsg(">>> status of your job\n");
        yodmsg(">>> (see the pingd man page at www.cs.sandia.gov/cplant/...)\n\n");
        if ( !get_bt ) {
          yodmsg(">>> you may also want to retry your job,\n");
          yodmsg(">>> compiling w/ the -g option and then\n"); 
          yodmsg(">>> running yod w/ the -bt option in order to\n");
          yodmsg(">>> obtain a GDB back trace...\n\n");
          yodmsg(">>> for a job that is hung the -bt option to yod\n");
          yodmsg(">>> now supports limited interactive debugging --\n"); 
          yodmsg(">>> viz asynchronous back trace requests using\n");
          yodmsg(">>> bt or C-bt (X version)... see the user\n\n");
          yodmsg(">>> documentation area at www.cs.sandia.gov/cplant\n\n");
        }
    }

    return;
}

static void
launch_failed(launch_failure *lfail)
{
int signum, exitcode, rank;

    failCount++;

    if ((lfail->nid < 0) || (lfail->nid >= MAX_NODES)){
        return;
    }
    rank = physnid2rank[lfail->nid];

    fail_status[rank].errType         = lfail->reason_code;
    fail_status[rank].reportingStatus = 0;

    if (lfail->reason_code == LAUNCH_ERR_PCT_GROUP){
        
        fail_status[rank].failedNid = lfail->type.peer.nid;
        fail_status[rank].failedPtl = lfail->type.peer.ptl;
        fail_status[rank].failedOp  = lfail->type.peer.op;
    }

    /*
    ** Normally report each type of error only once
    */
    if ((launch_err_types[lfail->reason_code] == 0) || 
         DBG_FLAGS(DBG_FAILURE) ||
        (lfail->reason_code == LAUNCH_ERR_CORRUPT_MSG) )
    {
        yoderrmsg(
         "\nFailure while hosting application process on node %d w/ rank %d.\n",
           lfail->nid, rank);

        yoderrmsg(
       "\tPCT report: %s\n",select_launch_err_string(lfail->reason_code));

        launch_err_types[lfail->reason_code]++;

        switch(lfail->reason_code){
            case LAUNCH_ERR_PCT_GROUP:
	        if (fail_status[rank].failedNid >= 0){
                    yoderrmsg("\tTimed out waiting for node %d.\n",
                          fail_status[rank].failedNid);
                }

                if (prog_phase < 2) { 
                    retryLoad = 1;
                }

                break;

            case LAUNCH_ERR_EXEC:
                yoderrmsg("\tUnable to run your code on compute node.\n");
                yoderrmsg("\tPerhaps the executable file is corrupt?\n\n");
                break;

            case LAUNCH_ERR_CHILD_NACK:  /* child sent nack before main */
                if (lfail->type.child.Errno)
                  yoderrmsg("\tApp process low level error: %s\n",
                     CPstrerror(lfail->type.child.Errno));
     
                if (lfail->type.child.CPerrno)
                  yoderrmsg("\tApp process library level error: %s\n",
                     CPstrerror(lfail->type.child.CPerrno));
     
                yoderrmsg("\tApp startup routine status: %s\n",
                         start_error_string(lfail->type.child.reason_code));
                break;

            case LAUNCH_ERR_CHILD_EXIT: /* child exited before main */
		yoderrmsg(
		"\tApp process terminated before entering user code.\n");
                signum = lfail->type.child.reason_code >> 16;
                exitcode = lfail->type.child.reason_code  & 0x00ff;

                if (signum){        
                    yoderrmsg("\tApp process terminating signal: %d, %s\n",
                                 signum, select_signal_name(signum));
                }
                yoderrmsg("\tApp process exit code: %d %s\n",
                         exitcode, start_error_string(exitcode));
                break;

            case LAUNCH_ERR_CORRUPT_MSG: /* bad hardware */

		yoderrmsg("\tReceived corrupt executable from node %d, rank %d\n",
		       lfail->type.peer.nid,  physnid2rank[lfail->type.peer.nid]);

                if (launch_err_types[lfail->reason_code] == 1){
	            yoderrmsg("\t****************************************************************\n");
	            yoderrmsg("\tThis is a rare event.  It may mean a network card has bad memory\n");
	            yoderrmsg("\tor perhaps a network switch is malfunctioning.  Please notify\n");
	            yoderrmsg("\tsystem administration with the node ID of the problem nodes.\n");
	            yoderrmsg("\t****************************************************************\n");
		}


                break;

            default:
                break;
        }
        if (in_perform_service && !abort_load && (launch_err_types[lfail->reason_code] == 1)){
            yoderrmsg(
             "Interrupt yod to kill the rest of the parallel application\n");
        } 
    }
}
static char pct_wait_msg[1024];
static char *sendwait = "->";
static char *recvwait = "<-";
static char reportStart;

/*
** Display chain of PCTs that are waiting for the next PCT.  Last one in line may be
** a PCT that crashed.
*/
static char *
show_chain(int startrank, int lastNid)
{
int rank, badNid, badRank, done;
char *failType;
char failString[64];

    rank = startrank;
    done = 0;

    /*
    ** print out the "waiting for" relationship
    */

    sprintf(pct_wait_msg,"%d ",rank2physnid[rank]);

    while (!done){

	failType = ((fail_status[rank].failedOp == SEND_OP) ? sendwait : recvwait);
	
	badNid = fail_status[rank].failedNid;
	badRank = physnid2rank[badNid];

	sprintf(failString,"%s %d ", failType, badNid);

        if ((strlen(failString) + strlen(pct_wait_msg)) > 1023){

	    sprintf(pct_wait_msg,"PCT list is too long to report, they are waiting for node %d\n",
		    lastNid);

            break;
	}
	strcat(pct_wait_msg, failString);

	rank = badRank;

        done = (fail_status[rank].reportingStatus == 3);
    }
    return pct_wait_msg;
}
void
mark_chain_done(int startrank)
{
int done, rank;

    /*
    ** Mark these PCTs done for reporting purposes
    */
    rank = startrank;
    done = 0;

    while (!done){

        done = (fail_status[rank].reportingStatus == 3);

        fail_status[rank].reportingStatus = 2;

	rank = physnid2rank[fail_status[rank].failedNid];
    }
    return;
}

/*
** Create string describing "waiting for" relationship, where it ends in a cycle,
** which means a PCT is waiting for a PCT which is waiting for ... which is waiting
** for the first PCT in the cycle.  The cycle part is in square brackets.
*/
static char *
show_cycle(int startrank, int cyclestart)
{
int rank, badNid, badRank, startCount;
char *failType;
char failString[64];

    rank = startrank;
    startCount = 0;

    while (1){   /* mark whole thing reported first */

        fail_status[rank].reportingStatus = 2;

	if (rank == cyclestart){
	    if (startCount == 1) break;
	    startCount++;
        }

	rank = physnid2rank[fail_status[rank].failedNid];
    }


    /*
    ** print out the cycle
    */
    rank = cyclestart;
    pct_wait_msg[0] = 0;

    while (1){

	failType = ((fail_status[rank].failedOp == SEND_OP) ? sendwait : recvwait);
	
	badNid = fail_status[rank].failedNid;
	badRank = physnid2rank[badNid];

	if (rank == cyclestart){  

	    sprintf(failString,"%d %s %d ", rank2physnid[rank], failType, badNid);
	}
	else{

	    sprintf(failString,"%s %d ", failType, badNid);
	}

        if ((strlen(failString) + strlen(pct_wait_msg)) > 1023){

	    sprintf(pct_wait_msg,"Cycle too long to report, includes node %d\n",
		    rank2physnid[cyclestart]);

            break;
	}
	strcat(pct_wait_msg, failString);

	if (badRank == cyclestart) break;

	rank = badRank;
    }
    return pct_wait_msg;
}
static char fmsg[1100];

static void
start_failure_msg()
{
    start_notify("load failure report"); /* mail to admins*/
    yoderrmsg("\nProblem report (\"%s\" sending to, \"%s\" waiting for a message from):\n",sendwait,recvwait);
    reportStart = 0;
}
static void
failure_msg(char *label, char *msg)
{

    sprintf(fmsg, "(job %d) %s: %s\n",job_ID,label, msg);

    yoderrmsg("%s",fmsg);
    log_msg(fmsg);
    notify(fmsg);
}    
static void
display_launch_failure()
{
int rank, startrank, badNid, badRank;
char *c;

    if (failCount == 0) {
        return;
    }

    reportStart = 1;

    /*
    ** Look for cycles - These PCTs are deadlocked in a group communication
    ** operation, at least one must be malfunctioning, or a message got lost.
    **
    ** Also look for a PCT that everyone else is waiting for.  Could be dead.
    */
    for (startrank = 0; startrank < NumProcs; startrank++){

        if ((fail_status[startrank].errType == LAUNCH_ERR_PCT_GROUP) &&
            (fail_status[startrank].reportingStatus == 0)                 ){

            rank = startrank;

            while (1){

                if (fail_status[rank].reportingStatus == 1){    /* cycle */
		    if (reportStart){
                        start_failure_msg();
		    }

                    c = show_cycle(startrank, rank);  /* marks it done too */

		    failure_msg("Deadlock", c);


                    break;
                }
                /*
                ** Mark this node as found, then we can detect if
                ** we come back to it when chasing the "waiting for"
                ** relationship.
                */
                fail_status[rank].reportingStatus = 1;

		badNid = fail_status[rank].failedNid;

		if ( (badNid < 0) || (badNid >= MAX_NODES) ||
		     (physnid2rank[badNid] < 0) || (physnid2rank[badNid] >= NumProcs) ){

                    /* reporting node gives invalid node id */

		    fail_status[rank].reportingStatus = 3;  /* end of chain */

		    if (reportStart){
                        start_failure_msg();
		    }

		    c = show_chain(startrank, badNid);

		    failure_msg("Waiting for invalid node",c);

		    mark_chain_done(startrank);

		    break;
		}

                badRank = physnid2rank[badNid];


                /*
                ** Was this node already reported because some
                ** other node was waiting for it?
		*/
                if (fail_status[badRank].reportingStatus == 2){

		    fail_status[badRank].reportingStatus = 3;  /* end of chain */
		    mark_chain_done(startrank);
                    break;

		}

                /*
                ** Did this node not report a group communication
		** failure to yod?
                */
                if (fail_status[badRank].errType != LAUNCH_ERR_PCT_GROUP){

		    fail_status[badRank].reportingStatus = 3;  /* end of chain */

		    if (reportStart){
                        start_failure_msg();
		    }

                    c = show_chain(startrank, badNid);
                    failure_msg("Waiting for node",c);
		    
		    mark_chain_done(startrank);

                    break;
                }

		rank = badRank;
            }
        }
    }

    if (reportStart == 0){
	end_notify();         /* finish off mail to admins */
    }
}

static void
yod_ignore(const CHAR *s)
{
    yoderrmsg("yod: ");
    yoderrmsg("%s - ",s);
    if (sverrno) yoderrmsg("%s\n",strerror(sverrno));
    if (CPerrno) yoderrmsg("%s\n",CPstrerror(CPerrno));
    yoderrmsg("ignoring\n");
}

/*
** For errors that occur before perform_service loop.  Application 
** processes may or may not have been started on the compute nodes.
*/
static void
yod_err(const CHAR *s)
{
int rc, mtype, oneTry;
char *mdata;
double t1;

    if (prog_phase < 2){    /* loading */
	add_to_log_error("load failed"); 
    }
    if (s){
        yoderrmsg("yod error: %s\n",s);
    }

    if (sverrno) yoderrmsg("%s\n",strerror(sverrno));
    if (CPerrno) yoderrmsg("%s\n",CPstrerror(CPerrno));

    /*
    ** Test control portal for pct failure messages.  If we've
    ** received at least one, wait around for about 5 seconds for more.
    */
    if ((pct_count > 0) && (failCount < pct_count)){
        oneTry = 1;
        t1 = dclock();

        while (oneTry || (failCount && ( (dclock() - t1) < 5.0))){

            rc = get_pct_control_message(&mtype, &mdata, NULL);

            if (rc == 1){
                if (mtype ==  LAUNCH_FAILURE_MSG){
                      launch_failed((launch_failure *)mdata);
                }
            }
            oneTry = 0;
        }
    }

    force_kill();
}
/*
** end_game - yod sends ABORT messages
**            pct kills app process
**            pct catches app process SIGCHLD and sends DONE message to yod
**            when all DONE messages are rec'd, yod displays completion
**            messages
*/
static void
end_game()
{
int rc;

#ifdef DEBUG_ENDGAME
    yodmsg(">>>>end_game, abort load %d, pct_count %d\n",
              abort_load, pct_count);
#endif
    
    /*
    ** If all PCTs have replied with app process termination
    ** we are done.
    */
    if ((pct_count==0) || ((failCount + proc_done_count) == pct_count)){
        finish();
    }

    /*
    ** If my user has logged out, switch me to batch mode.
    */
    if (interactive && (getppid() == 1)){
        interactive = 0;
    }

    if (abort_load == 2){  /* we've sent two already */
 
        if ((time(NULL) - tabort) > daemonWaitLimit){
            yodmsg("\n");
            yodmsg("Apparent problem on compute partition.  Please\n");
            yodmsg("notify system administration.\n");

            force_kill();
        }
        else{
            yodmsg("\nPlease be patient...\n");
        }
    }
    else if (abort_load == 1){ /* try a SIGKILL, SIGTERM didn't do it */

        if ((time(NULL) - tabort) > daemonWaitLimit){
            yodmsg(
            "\nWe're sending a SIGKILL to your application ...\n");
            yodmsg(
            "If we're still hanging around in %d seconds, control-C again.\n",
             daemonWaitLimit); 

            rc = srvr_send_to_control_ptl(LoadData.thePcts[0], PPID_PCT,
                 PCT_LOAD_PORTAL, MSG_ABORT_LOAD_2, 
                 (CHAR *)&job_ID, sizeof(int));

            if (rc){
                yod_err("send failure - notify system administration");
            }
            abort_load = 2;
            tabort = time(NULL);
        }
        else{
            yodmsg(
         "\nPlease be patient, awaiting compute node completion messages.\n");
        }
    }
    else if (abort_load == 0){

        yodmsg( 
         "\nWe're sending a SIGTERM to your application.\n");

        yodmsg( "Awaiting compute node completion messages.\n");
        yodmsg( 
   "If no response within %d seconds, try interrupting yod with control-C.\n",
            daemonWaitLimit);

        rc = srvr_send_to_control_ptl(LoadData.thePcts[0], PPID_PCT,
                 PCT_LOAD_PORTAL, MSG_ABORT_LOAD_1, 
                 (CHAR *)&job_ID, sizeof(int));

        if (rc){
            yod_err("send failure - notify system administration");
        }

        abort_load = 1;
        tabort = time(NULL);

        if (!in_perform_service){
            perform_service();
            finish();        
        }
    }
    return; 
}

/*
** force_kill when we've lost any hope of receiving pretty completion
** messages from the compute partition, or when we know the app
** process hasn't yet been started.  RESET message will clean
** up compute nodes.
*/
static void
force_kill()
{
#ifdef DEBUG_ENDGAME
    yodmsg(">>>>force_kill, proc_done_count %d, pct_count %d\n",
                    proc_done_count, pct_count);
#endif

    if (prog_phase == 0){
         /*
	 ** PCTs may be pending allocation to this job, but
	 ** we didn't begin the load.
	 */

         /* in the STRIPDOWN case, PCTs pending job init will
            timeout on their own...
         */
#ifndef STRIPDOWN
	 send_pcts_control_message(MSG_CANCEL_REQUEST_TO_LOAD,
		 (CHAR *)&job_ID, sizeof(int), NULL);
#endif
    }
    else if (pct_count){
        yodmsg( "Sending RESET messages to compute partition.\n");

        /*
        ** Things are really messed up, so send RESET to every node
        ** individually.  PCT group communication may be broken.
        */

	reset_load = 1;

        send_pcts_control_message(MSG_ABORT_RESET,
                     (CHAR *)&job_ID, sizeof(int), NULL);
    }
    
    finish();
}

static int exitValue=YOD_NO_ERROR;

static void
finish()
{

#ifdef DEBUG_ENDGAME
    yodmsg(">>>>finish, proc_done_count %d\n",proc_done_count);
#endif

    if (startTime != 0){    /* load completed */
        endTime = time(NULL);
    }
    else{
        exitValue = YOD_LOAD_ERROR;
        startTime = endTime = time(NULL);
    }
    if (prog_phase >= 0){      /* load started */

        if (job_ID != INVAL){
	    write_node_list(rank2physnid, NumProcs, msgbuf, MAXMSGLEN - 1);
            log_user(euid, startTime, endTime, NumProcs, msgbuf, fileName);
        }
	else{
            log_user(euid, startTime, endTime, NumProcs, NULL, fileName);
	}
    }

    if (start_logged && !end_logged){

        mkdate(time(NULL), appEndTime);

        if (terminator == PCT_NO_TERMINATOR){

            snprintf(log_record,USERLOG_MAX-1,
                ">> Finished Cplant user %s | base %d | start %s | "
		"end %s | cmd_line %s | nnodes %d | Nodes",
           ((pw && pw->pw_name) ? pw->pw_name : "unknown"), rank2physnid[0], appStartTime,
	   appEndTime, cmd_line, NumProcs);
        }
        else if (terminator == PCT_JOB_OWNER){
	
            snprintf(log_record,USERLOG_MAX-1,
	       ">> Killed Cplant user %s | base %d | start %s | "
	       "end %s | cmd_line %s | nnodes %d | Nodes",
           ((pw && pw->pw_name) ? pw->pw_name : "unknown"), rank2physnid[0], appStartTime,
	   appEndTime, cmd_line, NumProcs);
        }
        else if (terminator == PCT_ADMINISTRATOR){

           snprintf(log_record,USERLOG_MAX-1,
	       ">> Aborted Cplant user %s | base %d | time %s | cmd_line %s",

           ((pw && pw->pw_name) ? pw->pw_name : "unknown"), rank2physnid[0], appEndTime, cmd_line);
        }
        send_to_bebopd(log_record, strlen(log_record)+1,
	    YOD_USERLOG_RECORD, BEBOPD_GET_USERLOG_RECORD);

        log_msg(log_record);
	end_logged = 1;
    }


    if (proc_done_count  && !done_messages_displayed){
        if ( display_done_messages() )
            exitValue = YOD_APPLICATION_ERROR;
        else
            exitValue = YOD_NO_ERROR;

        /* so we don't print again from sig handler*/
        done_messages_displayed = 1; 
    }

    if (failCount && !fail_messages_displayed){
        display_launch_failure();

        /* so we don't print again from sig handler*/
        fail_messages_displayed = 1;  
    }

    if ((prog_phase > 0) &&
	!reset_load &&
	(proc_done_count + failCount < NumProcs) && 
	!silent_pcts_displayed){

        display_silent_pcts();

        /* so we don't print again from sig handler*/
        silent_pcts_displayed = 1;  
    }

#ifdef SUPPRESS_FORTRAN_RTL
    if (rtlmessages > 0){
        yodmsg(
        "\nPlease note: %d messages from the Fortran Run Time Library linked\n",
        rtlmessages-1);
        yodmsg(
        "with your application were suppressed.  Only the first was displayed.\n");
	yodmsg(
	"If you run yod with \"-d rtl\", these messages will not be suppressed.\n");
    }
#endif

    takedown_yod_portals();

    if (copies) cleanup_copied_files(timing_data);

    if (retryLoad){

        if (retryCount){
            try_again(exitValue);
	}
	else{
            yodmsg("\nApparently it is futile to keep retrying the load.  yod will exit now.\n");
	}

    }

    exit(exitValue);
}
/*
** If yod failed because of a launch failure (the PCTs failed to form a group, for
** example), it would be good to try again.  Although such an error should not happen,
** it does on a network where messages can get lost or inexplicable tens of second
** delays may occur.  Particularly for jobs started from unattended PBS scripts, we
** should try a few times before giving up.
**
** yod re-execs itself here, to try again.
*/
static void
try_again(int ec)
{
int pid;
char count[16];
char *yodpath;

#ifdef KERNEL_ADDR_CACHE
        if ( syscall(__NR_ioctl, cache_fd, CACHE_INVALIDATE, 0) <0) {
          yodmsg("CACHE_INVALIDATE failed\n");
          exit(ec);
        }
#endif

    if ((pid = fork()) > 0){

        waitpid(pid, NULL, 0);

        sleep(60);

        sprintf(count,"%d",--retryCount);

        setenv("YODRETRYCOUNT", count, 1);

	if (Argv[0][0] == '/'){
	    yodpath = real_path_name(Argv[0]);
	}
	else{
	    yodpath = find_in_path(Argv[0]);
	}

	if (yodpath == NULL){
	    yodmsg("Can't resolve %s, can not re-exec yod, sorry.\n",Argv[0]);
	    exit(ec);
	}
	Argv[0] = yodpath;

	execv(yodpath, Argv);

	yodmsg("New yod failed to get going... %s\n",strerror(errno));

	exit(ec);
    }

    yodmsg("\nyod will try again in a 60 seconds (retry number %d)\n",NRETRIES - retryCount + 1);

    exit(0);
}

static void
copy_warning(loadMembers *mbr, int nd)
{
    if (!mbr) return;

    yodmsg("\nWarning, your executable %s\n", mbr->pname);

    yodmsg("may take longer than usual to load because it's size (%d)\n",
                    mbr->data.execlen);
    yodmsg("exceeds the memory available on node %d to store it.\n", nd);
    yodmsg("We need to copy it somewhere.  Please be patient.\n\n");

}

void
CMDhandler_mass_murder(control_msg_handle *mh)
{
int srcnid;

      if (app_initiated_kill >= 0) return;

      srcnid = SRVR_HANDLE_NID(*mh);

      app_initiated_kill = physnid2rank[srcnid];

      yodmsg(
          "yod: Rank %d on node %d requested that yod should kill the application.\n",
	              app_initiated_kill, srcnid);
      yodmsg(
          "yod: Here we go.  Waiting for completion messages to come in.\n\n");

      abort_load = 1;
      end_game();
}
void
CMDhandler_heartbeat(control_msg_handle *mh)
{
       return;
}

static char *months[12]={"Jan", "Feb", "Mar",
                         "Apr", "May", "Jun",
                         "Jul", "Aug", "Sep",
                         "Oct", "Nov", "Dec"};
static void
mkdate(time_t t1, char *buf)
{
struct tm *t2;

    t2 = localtime(&t1);

    sprintf(buf, "%s %d, %d, %02d:%02d:%02d",
      months[t2->tm_mon], t2->tm_mday, 1900+t2->tm_year,
      t2->tm_hour, t2->tm_min, t2->tm_sec);

    return;
}

void yodmsg(char* format, ...) 
{
  va_list ap;
  if ( !quiet ) {
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
  }
}

void yoderrmsg(char* format, ...) 
{
  va_list ap;
  if ( !quiet ) {
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
  }
}
static void
request_progress_msg(int trial, int *mtypes, int *mdata, int nnodes)
{
int waittime, i;

    waittime = 0;

    for (i=0; i<nnodes; i++){
        if ((mtypes[i] == TRY_AGAIN_MSG) && (mdata[i] > waittime)) 
	     waittime = mdata[i];
    }

    if (trial > 0){
        yodmsg(
	"\n>>>Still waiting for currently running job(s) to exit. (%d seconds)\n", waittime);
    }
    else{
	if (pbs_job){
	    yodmsg(
	    "\n   A cycle stealing job is being killed to make way for your job.  Please wait.\n");
	}
	else{
	    yodmsg(
	    "\n   Some of the nodes you were allocated are running a low-priority job.\n");
	    yodmsg(
	    "   The job is being interrupted and killed.  This may take a few minutes.\n");
	    yodmsg(
	    "   If you don't wish to wait, interrupt yod with a control-C.\n");
	}
        yodmsg(
	"   At most %d seconds remain until the nodes will be available.\n",waittime);
    }
}
