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
** $Id: yod2.c,v 1.5 2002/01/18 23:59:08 pumatst Exp $
**
** to do:
**        many of the -d options are unimplemented
*/

#include <features.h>  /* this can go when the __USE_GNU lines do */
#define __USE_GNU  /* force __USE_GNU to get sysv_signal from signal.h */
#include <signal.h>
#undef  __USE_GNU  /* it conflicts in other places, e.g. off64_t in qkdefs.h */
#include <stdarg.h>
#ifdef __linux__
#include <getopt.h>
#endif
#ifdef __osf__
#include "linux-getopt.h"
#endif
#include <unistd.h>
/*
#include <sys/types.h>
*/
#include <sys/param.h>
#include <stdlib.h>
/*
#include <fcntl.h>
#include <ctype.h>
*/
#include <limits.h>
#include <string.h>

#include "sys_limits.h"
#include "appload_msg.h"
#include "config.h"
#include "job.h"
#include "yod2.h"

#define KILL1_GRACE_PERIOD  10
#define KILL2_GRACE_PERIOD  10

static void setup_sig_handlers(void);
static void usage_message();
static void option_lines();
static void strace_option_lines();
static void verbose_usage_message();
static void node_list_lines();
static int get_options(int argc, char *argv[]);
static void update_Dbgflag(const char *optarg);
static void process_done_msg(jobTree *job, 
            control_msg_handle *mh, int srcrank);

static void kill_jobs_and_exit(void);
static void completion_report(void);
static void kill_progress(void);
static void load_progress(void);
static int perform_service(void);

static int SessionInterrupt = 0;

/**************************************
** command line options
*/
static int jobNnodes=0;
static int jobNprocs;    /* processes per node - for future use */
int priority = REGULAR_JOB;
static char *node_string = NULL;
static int listsize = 0;
static int Dbglevel = 0;
static int DebugSpecial = 0;
static int straceDirLen = 0;
static int straceOptLen = 0;
static int straceListLen = 0;
static int jobCreationHistory = 0;
static char **fname_argv;
static int fname_argc;
/*
*****************************************
*/

#define NRETRIES 3

#define NICE 1
#define NOW  2

#include "job.h"

int exitCondition=YOD_NO_ERROR;

int
main(int argc, CHAR *argv[], CHAR *envp[])
{
int rc;
ndrequest *ndreq;
time_t nodenoise;

    setup_sig_handlers();

    rc = get_options(argc, argv);

    if (rc < 0){
        yoderrmsg("command line options");
	exit(YOD_LOAD_ERROR);
    }
    if (DBG_FLAGS(DBG_ENVIRON)){
        extern char **environ;
        char *env;
        int i;

        for (env = environ[0], i=0; env; env = environ[++i]){
            yodmsg("%s\n",env);
        }
        yodmsg("\n");
    }

    ndreq = build_node_request(fname_argc, fname_argv, 
                               jobNnodes, jobNprocs, node_string);

    if (!ndreq){
        yoderrmsg("Can't create node request for bebopd\n");
	exit(YOD_LOAD_ERROR);
    }

    if (DBG_FLAGS(DBG_LOAD_1)){
        yodmsg("Node request:\n");
	displayNodeRequest(ndreq);
    }

    /* 
    ** allocate nodes and start the load 
    */
    if (new_job(ndreq, INVALID_PARENT_HANDLE) == NULL){
        exit(YOD_LOAD_ERROR);
    }

    nodenoise = 0;

    while (1){

        /*
        ** poll for PCT's app completion messages and for
        ** app IO requests and spawn/synchronization requests
        */
        if (numJobs[runningJob]  + numJobs[dyingJob]){

            if (perform_service()){

                nodenoise = time(NULL);
            }

        }

        /*
        ** complete load of app
        */
        if (numJobs[loadingJob] > 0){
            load_progress(); 
        }

        /*
        ** maybe time to move along a job kill operation
        */
        if (numJobs[dyingJob] > 0){
            kill_progress();
        }

        /*
        ** display completion messages of terminated app
        */
        if (numJobs[finishedJob] > 0){

            completion_report();

            if (NUM_APPS_LEFT == 0){
                break;
            } 
        }

        /*
        ** yield processor when not too busy
        */
        if (nodenoise &&
            ((time(NULL) - nodenoise) > 2) ){

            nodenoise = 0;
        }

        if (!nodenoise) usleep(200000);
    }

    if (jobCreationHistory){
        printf("\nJob Creation History\n---------------------\n");
        printAllJobs(1);
    }
    exit(exitCondition);
}
static void
kill_jobs_and_exit()
{
int i, j, type;
jobTree *job;

    for (i=0; i < 2; i++){

        type = liveJobs[i];

        for (j = 0; j < numJobs[type] ; j++){

            job = jobs[type][j];

            yodmsg("Reset nodes in use by job %d\n",job->job_id);

            yodJobDone(job);

            job_nodes_reset(job->handle);
       }
   }
   for (j = 0; j < numJobs[loadingJob] ; j++){

        job = jobs[loadingJob][j];

        yodmsg("Cancel load of job %d\n",job->job_id);

        yodJobDelete(job);
   }
   exit(exitCondition);
}
static void
completion_report()
{
jobTree *job;
int j;

    numJobs[tempType] = 0;

    for (j=0; j < numJobs[finishedJob] ; j++){

        job = jobs[finishedJob][j];

        printf("\nJob %d has terminated:\n",job->job_id);

        jobPrintCompletionReport(job->handle, stdout);
 
        printf("\n");

        addToTmpList(job);
    }

    for (j=0; j < numJobs[tempType] ; j++){

        job = jobs[tempType][j];

        yodJobReported(job);
   }
}
static void
kill_progress()
{
jobTree *job;
int j;

    numJobs[tempType] = 0;

    for (j=0; j < numJobs[dyingJob] ; j++){

        job = jobs[dyingJob][j];

        if (job->status == KILL2_JOB){

            if ((time(NULL) - job->startKill2.tm) > KILL2_GRACE_PERIOD){

                job_nodes_reset(job->handle);
                addToTmpList(job);
            }
        }
        else if (job->status == KILL1_JOB){

            if ((time(NULL) - job->startKill1.tm) > KILL1_GRACE_PERIOD){

                job_send_signal(job->handle, SIGKILL);
                addToTmpList(job);
            }
        }
    }

    for (j=0; j < numJobs[tempType] ; j++){

        job = jobs[tempType][j];

        if (job->status == KILL2_JOB){
            yodJobDone(job);
        }
        else if (job->status == KILL1_JOB){
            yodJobSecondKill(job);
        }
    }
}
static void
load_progress()
{
jobTree *job;
int j, rc;
time_t t1;

    numJobs[tempType] = 0;

    t1 = time(NULL);

    for (j=0; j < numJobs[loadingJob] ; j++){

        job = jobs[loadingJob][j];

        if ((t1 - job->lastLoadRequest) < 60){
            continue;
        }
        job->lastLoadRequest = t1;

        rc = job_load(job->handle, 0); /* non-blocking request */

        if ((rc == 0) && load_done(job)){
            addToTmpList(job);  /* load completed */
        }
        else if (rc == -1){

            rc = retry_load(job);

            if (rc == 0){
                if (load_done(job)) addToTmpList(job);
            }
            else{ 
                yoderrmsg("Unable to load job, sorry.\n");
                job->status = LOAD_FAILED_JOB;
                addToTmpList(job);
                /*
                ** we don't delete these jobs, spawner may
                ** check back to see what happened to the load
                */
            }
        }
    }

    for (j=0; j < numJobs[tempType] ; j++){

        job = jobs[tempType][j];

        if (job->status == LOAD_FAILED_JOB){
            yodJobReported(job); 
        }
        else{
            yodJobRunning(job);
        }
    }
}


static int
perform_service()
{
jobTree *job;
int i, j, type, rc, getem;
int msgtype, nmsgs;
int srcnid, srcrank, killTypeWas;
control_msg_handle mh;

    numJobs[tempType] = 0;

    nmsgs = 0;

    /*
    ** Go look for PCT messages (usually saying a process
    ** is done) and APP messages (usually IO messages, but
    ** could be process spawning or synchronization
    ** messages).  Check all running and dying jobs.
    */
    for (i=0; i < 2; i++){

        type = liveJobs[i];

        for (j = 0; j < numJobs[type] ; j++){

            job = jobs[type][j];

            getem = 1;
            killTypeWas = job->killType;

            while (getem){

                rc = job_get_pct_message(job->handle, &msgtype,
                             &srcnid, &srcrank, &mh);

                if (rc == 1){
            
                    if (msgtype == PROC_DONE_MSG){

                        job->ndone++;

                        process_done_msg(job, &mh, srcrank);
                    }
                    else if (msgtype == LAUNCH_FAILURE_MSG){

                        job->nfail++;

                        if (job == &origJob){
                            exitCondition = YOD_LOAD_ERROR;
                        }

                        if (job->killType != NOW){
                            job->killType = NOW;
                            getem = 0;
                        }
                    }
                    nmsgs++;

                    job_free_pct_message(job->handle, &mh);
                }
                else{
                    getem = 0;
                }
            }
            if ((killTypeWas != job->killType) ||
                (job->ndone + job->nfail == job->nprocs)){

                addToTmpList(job);
            }

            getem = 1;

            while (getem){

                /*
                ** Look for a message from the application.  If
                ** it's IO, the request will be handled in the
                ** call.
                */

                rc = job_get_app_message(job->handle, &msgtype,
                             &srcnid, &srcrank, &mh);

                if (rc == 1){

                    if (msgtype == YO_ITS_IO){

                        /* was handled by libjob.a */
                    }
                    else if (msgtype == JOB_MANAGEMENT){

                        perform_service_job(job, srcrank, &mh);
                    }
                    else if (msgtype == INTRA_JOB_BARRIER){

                        register_sync_request(job, srcrank, &mh);
                    }
                    nmsgs++;
                    job_free_app_message(job->handle, &mh);
                }
                else{
                    getem = 0;
                }
            }
        }
    }

    /*
    ** If it's time for a job to go away, start the process
    ** of orderly termination and reporting.
    */
    if (numJobs[tempType] > 0){

        for (j=0; j<numJobs[tempType]; j++){

            job = jobs[tempType][j];

            if (job->ndone + job->nfail == job->nprocs){
                yodJobDone(job);
            }
            else if (job->killType == NICE){
                job_send_signal(job->handle, SIGTERM);
                yodJobFirstKill(job);
            }
            else if (job->killType == NOW){
                job_send_signal(job->handle, SIGKILL);
                yodJobSecondKill(job);
            }
        }
    }

    return nmsgs;
}
static void
process_done_msg(jobTree *job, control_msg_handle *mh, int srcrank)
{
app_proc_done *doneMsg;
int exitCode, termSig;
int srcnid;
final_status *fs;

    doneMsg = (app_proc_done *)SRVR_HANDLE_USERDEF(*mh);

    fs = &(doneMsg->final);

    exitCode = fs->exit_code;
    termSig  = fs->term_sig;

    srcnid = SRVR_HANDLE_NID(*mh);

    if ((termSig || exitCode) &&
        (job->listType == runningJob)     ) {

        if (job->abendWarnings == 0){
	    if (termSig){
	        add_to_log_string(&job->log_error, "%s on node %d",
	           select_signal_name(termSig),srcnid);
	    }
	    else {
	        add_to_log_string(&job->log_error, "exit code %d on node %d",
	           exitCode,srcnid);
	    }
            if (job == &origJob){
                exitCondition = YOD_APPLICATION_ERROR;
            }
        }

        if (job->abendWarnings < 4){
	    if (termSig){
	        yoderrmsg(
	       "Job %d node %d rank %d, terminated with %s\n",
	         job->job_id, srcnid, srcrank,
	         select_signal_name(termSig));
	    }
	    else{
	        yoderrmsg(
	       "Job %d node %d rank %d, terminated with exit code %d\n",
	         job->job_id, srcnid, srcrank, exitCode);
	    }
        }
        else if (job->abendWarnings == 4){
	   yoderrmsg(
     "Further Job %d abnormal termination messages suppressed\n",job->job_id);
        }

        job->abendWarnings++; 

        if (jobOptions.autokill || !jobOptions.interactive) {

            if (!job->killType){  
 	        job->killType = NICE;
            }
        }
    }
    if (!job->endCode){
        /*
        ** Zero all final_statuses to start - this sets the terminator
        ** fields to PCT_TERMINATOR_UNSET, flagging which processes have
        ** terminated and which have not.
        */
        job->endCode = (final_status *)calloc(job->nprocs, sizeof(final_status));

        if (!job->endCode){ 
            return;
        }
    }
    job->endCode[srcrank].term_sig   = termSig;
    job->endCode[srcrank].exit_code  = exitCode;
    job->endCode[srcrank].terminator = fs->terminator;
}
/*
********************************************************************************
** yod2 options 
********************************************************************************
*/
static struct option yod_options[] =
{
    {"attach", no_argument, 0,       'a'},
    {"bt", no_argument, 0,           'b'},
    {"debug", required_argument, 0,  'd'},
    {"show", no_argument, 0,         's'},
    {"xxx", no_argument, 0,          'x'},
    {"alloc", no_argument, 0,            'A'},
    {"batch", no_argument, 0,            'B'},
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
    {"history", no_argument, 0,       'h'},
    {0,0,0,0}
};
extern CHAR *optarg;
extern int optind, optopt;

static int
get_options(int argc, char *argv[])
{
int opttype, val;
char *c;

    optind = 0;

    memset((void *)&jobOptions, 0, sizeof(jobOpt));
    jobOptions.interactive = 1;
    jobOptions.altBnid = INVAL;
    jobOptions.altBpid = INVAL;

    jobOptions.retryCount = 
          ((c = getenv("YODRETRYCOUNT")) ? atoi(c) : NRETRIES);

    if ((c = strrchr(argv[0], (int)'/'))){
        jobOptions.myName = strdup(c+1);
    }
    else{
        jobOptions.myName = strdup(argv[0]);
    }


    while(1){

        opttype = getopt_long_only(argc, argv, "+", yod_options, 0);

        if (opttype == EOF){
            break;
        }

        switch (opttype){
            case 'H':
                usage_message();
                exit(YOD_NO_ERROR);

            case 'V':
                verbose_usage_message();
                exit(YOD_NO_ERROR);

            case 'A':
                if (jobOptions.attach_gdb != TRUE) {
                  jobOptions.display_allocation = TRUE;
                }
                break;

            case 'a':
                jobOptions.attach_gdb = TRUE;
                if (jobOptions.display_allocation == TRUE) {
                  jobOptions.display_allocation = FALSE;
                }
                if (jobOptions.get_bt == TRUE) {
                  yodmsg("WARNING: -bt and -attach incompatible; -attach taking precedence\n");
                  jobOptions.get_bt = FALSE;
                }
                break;

            case 'x':
                jobOptions.bypass_link_version_check = 1;
                break;

            case 'Z':
                jobNnodes = atoi(optarg);

                if ((jobNnodes <= 0) || (jobNnodes > MAX_PROC_PER_GROUP)){
                    yoderrmsg("invalid size: try  1 through %d nodes\n",
                           MAX_PROC_PER_GROUP);

                    option_lines();
                    return -1;
                }
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
                jobOptions.autokill=1;
                break;

            case 'N':
                /* the NOBUF option.
                   for the most part, yod2 uses fprintf() for output.
                   but for output from an application it uses write().
                   depending on how you call yod2 (interactively or not)
                   these two forms of output may be buffered differently
                   leading to some unexpected results (the application's
                   output preceding any of yod's output for example in
                   case yod2 is run noninteractively so the fprinf()s
                   are buffered while write() is not...)
                   to force consistency, we may use this option to turn
                   off buffering of stdout. more precise control could
                   be obtained my calling setvbuf().
                */
                setbuf(stdout,NULL);
                break;

            case 'b':
                if (jobOptions.attach_gdb == TRUE) {
                  yodmsg("WARNING: -bt and -attach incompatible; -attach taking precedence\n");
                }
                else {
                  jobOptions.get_bt = TRUE;
                }
                break;

            case 's':
                jobOptions.show_link_versions = TRUE;
                break;

            case 'L':
                jobOptions.log_startup_actions = TRUE;
                break;

            case 'M':
                val = atoi(optarg);
                jobOptions.pauseForDebugger = val;
                break;

            case 'X':

		node_string = optarg;

		listsize = parse_node_list(optarg, NULL, 0,
					   0, MAX_NODES-1);

		if (listsize <= 0){
		    node_list_lines();
		    return -1;
		}

                break;

            case 'T':
                jobOptions.timing_data = 1;
                break;

            case 'I':
                jobOptions.interactive = 1;
                break;

            case 'Q':
                jobOptions.quiet = 1;
                break;

            case 'B':
                jobOptions.interactive = 0;
                break;

            case 'F':
                jobOptions.done_file = optarg;
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

            case '1':
                jobOptions.straceDirectory = optarg;
                straceDirLen = strlen(optarg);
                break;

            case '2':
                jobOptions.straceOptions = optarg;
                straceOptLen = strlen(optarg);
                break;

            case '3':
                jobOptions.straceNodeList = optarg;
                straceListLen = strlen(optarg);
                break;

            case 'h':
                jobCreationHistory = 1;
                break;

            default:
                break;

            case '?':
                yodmsg("Invalid options\n");
                option_lines();
                return -1;

        }
    }

    if (Dbglevel > 0){
        job_debug_load();
        job_debug_pbs();
        job_debug_allocation();
        job_debug_heterogeneous_load();

        if (Dbglevel > 1){
	    job_debug_load_detail();
	    job_debug_communications();
	    job_debug_bebopd();

            if (Dbglevel > 2){
		job_debug_app_failure();
		job_debug_app_progress();
		job_debug_heap();
            }
        }
    }
    /*
    ** Done with yod2 arguments, what follows is program
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
    if ((jobOptions.straceOptions || jobOptions.straceNodeList) && 
          !jobOptions.straceDirectory){

        yodmsg("You specified -straceoptions or -stracenodes.\n");
        yodmsg("You must supply a directory to which strace output will go.\n");
        yodmsg("Use the -strace option to do this.  Ask system administration for\n");
        yodmsg("advice about writable directories mounted on the compute nodes.\n\n");
        strace_option_lines();
    }

    if (!jobOptions.interactive && jobOptions.display_allocation){
        yodmsg(">>>\n");
        yodmsg("Turned off display of node allocation in batch mode.\n");
        yodmsg("Requires user to specifically send processes to main.\n");
        yodmsg(">>>\n");
        jobOptions.display_allocation = 0;
    }
    /*
    ** If "-sz {n} -l {nodelist}" then we'll try to get n nodes from nodelist.
    ** "n" can be less than the number of nodes in the nodelist.  If no -sz
    ** specified, we try to get all the nodes in the nodelist.  If neither
    ** -sz nor -l are specified, we try to get one node from anywhere.
    */
    if (listsize > 0){

        if (jobNnodes > listsize){
            yodmsg(
      "Requested partition size of %d will not fit in list of requested nodes\n",
                     jobNnodes);
            option_lines();
            node_list_lines();
            return -1;
        }
    }

    return 0;

}
static void
update_Dbgflag(const char *optarg)
{
    if (!optarg) return;

    if (!strcmp(optarg, "io")){
        /* list application IO requests */
        job_debug_io();
    }
    else if (!strcmp(optarg, "iomore")){
        /* detailed handling of app IO requests */
        job_debug_io_detail();
    }
    else if (!strcmp(optarg, "job")){
        /* detailed handling of app job spawn requests */
        job_debug_job_spawn();
    }
    else if (!strcmp(optarg, "memory")){
        /* list all buffers allocated */
        job_debug_heap(); 
    }
    else if (!strcmp(optarg, "load")){
        /* load protocol steps */
        job_debug_load(); 
    }
    else if (!strcmp(optarg, "loadmore")){
        /* detailed handling of load protocol steps */
        job_debug_load_detail();
    }
    else if (!strcmp(optarg, "progress")){
        /* display app process "done" messages */
        job_debug_app_progress();
    }
    else if (!strcmp(optarg, "alloc")){
        /* node allocation details */
        job_debug_allocation();
    }
    else if (!strcmp(optarg, "hetero")){
        /* heterogeneous load details */
        job_debug_heterogeneous_load();
    }
    else if (!strcmp(optarg, "pbs")){
        /* PBS information */
        job_debug_pbs();
    }
#ifdef SUPPRESS_FORTRAN_RTL
    else if (!strcmp(optarg, "rtl")){
        /* Display the Fortran Run Time Library messages from compute nodes */
        job_debug_forrtl();
    }
#endif
    else if (!strcmp(optarg, "failure")){
        /* display app process load progress and "done" messages */
        job_debug_app_failure();
    }
    else if (!strcmp(optarg, "debug")){
        /* display efforts to obtain application debugging data */
        job_debug_app_debug();
    }
    else if (!strcmp(optarg, "bebopd")){
        /*  bebopd communications */
        job_debug_bebopd() ;
    }
    else if (!strcmp(optarg, "comm")){
        /* communications structures */
        job_debug_communications();
    }
    else if (!strcmp(optarg, "environ")){
        /* environment variables */
       job_debug_yod_environment(); 
    }
    return;
}


/*
********************************************************************************
** Signal handlers
********************************************************************************
*/
static void
sighup_handler(int sig)
{
    yodmsg("YOD2 got a sighup\n");
    printAllJobs(1);
    sysv_signal(sig, sighup_handler);
}
static void
signal_handler(int sig)
{
char *c;
int j;
jobTree *job;

    yoderrmsg("yod2 received %s (%d)\n",select_signal_name(sig),sig);

    if ((sig == SIGINT) || (sig == SIGTERM)){  /* owner wants to interrupt apps */
        sysv_signal(sig, signal_handler);

        if (SessionInterrupt == 0){

            SessionInterrupt = 1;
            yodmsg("Terminating all jobs.\n");

            numJobs[tempType] = 0;

            for (j = 0; j < numJobs[loadingJob]; j++){
                job = jobs[loadingJob][j];

                yodmsg("Cancelling load of job # %d.\n",job->job_id);

                add_to_log_string(&job->log_error,
                     "load killed by yod interruption");

                addToTmpList(job);
            }
            for (j = 0; j < numJobs[runningJob]; j++){
                job = jobs[runningJob][j];

                yodmsg("Sending SIGTERM to job # %d.\n",job->job_id);

                add_to_log_string(&job->log_error,
                     "app killed by yod interruption");

                job_send_signal(job->handle, SIGTERM);

                addToTmpList(job);
            }
            for (j = 0; j < numJobs[dyingJob]; j++){
                job = jobs[dyingJob][j];

                yodmsg("Resetting nodes hosting job # %d.\n",job->job_id);

                job_nodes_reset(job->handle);

                addToTmpList(job);
            }

            for (j = 0; j < numJobs[tempType]; j++){

                job = jobs[tempType][j];

                if (job->status == LOADING_JOB){
                    yodJobDelete(job);
                }
                else if (job->status == RUNNING_JOB){
                    yodJobFirstKill(job);
                }
                else if ((job->status == KILL1_JOB)||(job->status == KILL2_JOB)){
                    yodJobDone(job);
                }
            }

            SessionInterrupt = 2;
        }
        else{
            kill_jobs_and_exit();
        }
    }
    else{
        /*
	** Where in library did error occur?  Can remove this when
	** libjob.a is all debugged.
	*/
        c = job_strerror();
        if (c[0]){
	    yodmsg("job library error: %s\n",c);
	}
	yodmsg("job library stack trace:\n");
        job_stack_display();  

        add_to_log_string(&(origJob.log_error),
             "yod got a %s ",select_signal_name(sig));

        kill_jobs_and_exit();
    }

    setup_sig_handlers();

    return;
}  /* end of signal_handler() */

static void
propagate_user_signal(int sig)
{
int i, j, type;
jobTree *job;

    yodmsg( "Propagating SIGUSR%d to applications\n",(sig == SIGUSR1) ? 1 : 2);
    
    for (i=0; i < 2; i++){

        type = liveJobs[i];

        for (j = 0; j < numJobs[type] ; j++){

            job = jobs[type][j];

            job_send_signal(job->handle, sig);
        }
    }

    setup_sig_handlers();

    return;
}

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
             (sig == SIGHUP) ||
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

    sysv_signal(SIGHUP, sighup_handler);
}
/*
********************************************************************************
** yod2 output - suppress it all if "yod2 -quiet" selected
********************************************************************************
*/

void yodmsg(char* format, ...)
{
  va_list ap;
  if ( !jobOptions.quiet ) {
    va_start(ap, format);
    if (jobOptions.myName) printf("%s: ",jobOptions.myName);
    vprintf(format, ap);
    va_end(ap);
  }
}

void yoderrmsg(char* format, ...)
{
  va_list ap;
  if ( !jobOptions.quiet ) {
    va_start(ap, format);
    if (jobOptions.myName) fprintf(stderr, "%s: ",jobOptions.myName);
    vfprintf(stderr, format, ap);
    va_end(ap);
  }
}

/*
********************************************************************************
** usage info
********************************************************************************
*/
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
yodmsg("YOD2 USAGE:\n\n");
yodmsg("\tyod2 {yod2 options} program-name {program arguments}\n");
yodmsg("\t\t\t\t-- OR --\n");
yodmsg("\tyod2 {yod2 options} load-file-listing-program-names-and-arguments\n\n");
yodmsg("yod2 options are:\n\n");
yodmsg("-l [node-list] Allocate compute partition from these nodes only.  If not spec-\n");
yodmsg("               ified, nodes will be allocated from pool of all available nodes.\n");
yodmsg("-sz [n]       Number of nodes requested.  If not specified, assume all nodes in\n");
yodmsg("              the node-list, or 1 node from anywhere if there's no node-list.\n");
yodmsg("-alloc        After load, display allocated nodes and wait for my OK to start\n");
yodmsg("              the parallel code.\n");
yodmsg("-attach       After load, display allocated nodes and wait for my OK to start\n");
yodmsg("              processes. Intended for use with GDB proxy utility, cgdb,\n");
yodmsg("              although essentially the same as -alloc.\n");
yodmsg("-batch        yod2 is NOT running in interactive mode.\n");
yodmsg("-interactive  yod2 IS running in interactive mode (default).\n");
yodmsg("-kill         Kill my job if any processes terminate abnormally.\n");
yodmsg("-nice         Kill my job (with SIGTERM/wait 5 min./SIGKILL) if another job needs the nodes.\n");
yodmsg("-D            Display load debugging information.  Repeat to increase quantity.\n");
yodmsg("-bt           Notify yod2 that the parallel application was compiled with debugging.\n");
yodmsg("              symbols.  yod2 will provide a stack trace if the application faults.\n");
yodmsg("-file [name]  write final status messages to file [name] instead of stdout.\n");
yodmsg("-history      display job creation history when all jobs have completed.\n");
yodmsg("-show         yod2 will show link version nos. for both yod2 and executable\n");
yodmsg("-quiet        yod2 will not display any status or error messages.\n");
yodmsg("-help         display this message\n");
yodmsg("-vhelp        display more verbose help\n\n");
}
static void
debug_option_lines()
{
yodmsg("\n");
yodmsg("-d io          Display application IO requests to yod.\n");
yodmsg("-d iomore      Display details of dispatch of IO requests.\n");
yodmsg("-d job         Display details of job spawn/synch requests.\n");
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
yodmsg("-d bebopd      Display yod2 interactions with bebopd.\n");
yodmsg("-d comm        Display information about portals setup.\n");
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
