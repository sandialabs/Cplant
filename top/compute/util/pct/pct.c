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
** $Id: pct.c,v 1.130.2.2 2002/03/21 18:47:56 jsotto Exp $
*/

#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <limits.h>
#include <malloc.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/mman.h>
 
#include "pct.h"
#include "appload_msg.h"
#include "fyod_map.h"
#include "config.h"
#include "bebopd.h"
#include "srvr_err.h"
#include "srvr_comm.h"
#include "srvr_coll.h"
#include "ppid.h"
#include "portal_assignments.h"
#include "srvr_coll_fail.h"
#include "pct_ports.h"

#include "cplant_host.h"

static int get_pct_options(int argc, char *argv[]);
static void usage_message(void);

static void signal_handler(int sig);
static void sighup_handler(int sig);
static void updateTimeouts(void);
static void sigusr_handler(int sig);
static void setup_sig_handlers(void);
static void takedown_sig_handlers(void);

static int stow_executable(proc_list *plist);
static int create_exec(char *name, char *user_exec, int size,
		       uid_t uid, gid_t gid);
static void process_app_state_change(appData *reply);

static int process_yod_message(control_msg_handle *load_req);
static int process_bebopd_requests(control_msg_handle *req);
static int vote_on_completion(proc_list *pl);

static void clear_appHost(void);
static void reset_pct(void);
static void reset_pct2(void);
static void yod_send_error(void);

int DebugSpecial;
int fyod_map[FYOD_MAP_SZ];

const char *routine_name="uninitialized";
const char *routine_where="uninitialized";
const char *routine_name0="uninitialized";
const char *routine_where0="uninitialized";
const char *routine_name1="uninitialized";
const char *routine_where1="uninitialized";

load_data_buffer LoadData;
load_msg1 init_msg;

double td0, td1, td2, td3;

static int loading;
static int daemon_mode;
static int loopProgressCounter=0;

static int LocalExecStorage;
static int noBebopdAckYet;
static int bnidGiven;
static int nice_kill_seconds;

extern char *scratch_loc;

char *ename;
char *gdbProc = "PCT (gdb child)";
char *gwrapProc = "PCT (gwrap child)";
char *appProc = "PCT (application process child)";

/*
** PCT - the compute node daemon.  It forks/execs the application
** process and catches it's termination.  It can attach a debugger
** to the app process.  It reports status to the bebopd.
**
**      options:
**
**          -D turn on debugging output, "-D -D" is more verbose.
**         
**          -S 1    log to stderr
**          -S 0    don't log to stderr (the default)
**         
**          -L 1    log to cplant log file (the default)
**          -L 0    don't log to cplant log file
**
**          -d      fork and detach from tty, good for
**                    remotely (re-)starting using rsh
**
**   Normally PCT looks up bebopd location in a file.  If you
**   want it to talk to a different bebopd you can specify that
**   on the command line:
**
**          -nid    Node ID of bebopd
**          -pid    Portal ID of bebopd
**
**  Warning - some of PCT is written to assume it is only running one
**  process for each application.  Some of it is written more generally.
**  If we load more than one member of application on same node, some
**  fix up will be necessary.
*/

static int bptl;    /* how to contact bebopd */
static nid_type bnid;
static spid_type bpid;

static mgrData appHostReq;
static char *appHostData;
static int appHostDataLen;

int gwrap_pid=0;
int gwrap_status;
int collectiveWaitLimit=30;

extern pid_t tvdsvrpid;
 
#ifdef FORK_APP_HOST
static appData appHostReply;
#else
static appData *appHostReply;
#endif
int health_check = TRUE;

static char namebuf[16];

int
main(int argc, char *argv[])
{
control_msg_handle load_req, bebopd_req, tvdsvr_req, bt_req, cgdb_req;
proc_list *pl;
int rc;
int list;
int count, i;
double pctStartTime, lastBebopdUpdate;

    sprintf(namebuf,"PCT-%d",_my_pnid);

    log_open(namebuf);

    LOCATION("main","top");

    Dbglevel = 0;

    ename = argv[0];

    bnid = SRVR_INVAL_NID;
    bpid = SRVR_INVAL_PID;
    bptl = SRVR_INVAL_PTL;
    bnidGiven = 0;

    LOCATION("get_pct_options","top");

    if (argc > 1){
	rc = get_pct_options(argc, argv);

	if (rc){
	    usage_message();
	    log_quit("command line args incomplete");
	}
    }

    /*
     * Detach from tty so that "rsh node pct" does not hang rsh.
     */
    if (daemon_mode) {
	int pid;

	if ((pid=fork()) < 0)
	    log_error("fork failed");
	if (pid > 0)
	    exit(0);
	mlockall( MCL_CURRENT | MCL_FUTURE );
	freopen("/dev/null", "r", stdin);
	freopen("/dev/null", "w", stdout);
	freopen("/dev/null", "w", stderr);
	if (setsid() < 0)
	    log_error("setsid failed");

        _my_pid = getpid();
    }

   /* Register my PPID */
    _my_ppid = register_ppid(&_my_taskInfo, PPID_PCT, GID_PCT, "pct");

    if (_my_ppid != PPID_PCT){
 
	fprintf(stderr, "Can not register myself as PPID=%d\n", PPID_PCT);
	fprintf(stderr, "Perhaps there's another PCT already out there?\n");
	log_msg(" Can not register myself with PPID_PCT");
	exit(-1);
    }
    updateTimeouts();

    rc = server_library_init();

    if (rc){
        log_error("initializing server library");
    }
    /* 
    ** Get nid of bebopd, nid and pid of bebopd can be overridden
    ** with command line arguments for testing.
    */
    if (bnid == SRVR_INVAL_NID){
	if ( cplantHost_getNid("bebopd", 1, &list, &count) != OK )
	{
	    fprintf(stderr, "Can not get nid from cplant-host file\n");
	    exit(-1);
	}

	if ( count != 1 )
	{
	    fprintf(stderr, "Expected one bebopd entry; got %d\n", count);
	    exit(-1);
	}

	bnid = list;
    }
    if (bpid == SRVR_INVAL_PID){
        bpid = PPID_BEBOPD;
    }
    bptl = UPDATE_PTL;

    set_bebopd_id(bnid, bpid, bptl);
    
    LOCATION("setup_sig_handlers","");
    setup_sig_handlers();

    LOCATION("init_app_structures","");
    init_app_structures();
    
    LOCATION("server_coll_init","");
    rc = server_coll_init();

    if (rc){
        log_error("failure to init pct group communications");
    }

    for (i=0; i<FYOD_MAP_SZ; i++) {
       fyod_map[i] = -1;
    }
    rc = getFyodMap( fyod_map );
    if (rc){
        log_warning("failure to get fyod map");
    }

    LOCATION("initialize_listen","");
    rc = initialize_listen(bnid, bpid, bptl);

    if (rc){
        log_warning("initializing listen");
        cleanup_pct();
        log_quit("DONE");
    }

    clear_appHost();

    noBebopdAckYet = 1;
    pctStartTime = dclock();
    lastBebopdUpdate = pctStartTime - BEBOPD_INTERVAL;

    nice_kill_seconds = atoi(nice_kill_interval());

    while (1){

#ifdef STRIPDOWN
        if (start_pending_clock){

	  if ((time(NULL) - start_pending_clock) > PENDING_STATE_TIME_OUT){
	    stop_pending_for();
            State.status = STATUS_FREE;
            send_to_bebopd(PCT_UPDATE_MSG);
          }
        }
#endif
        loopProgressCounter++;
	if (loopProgressCounter > 1000000) loopProgressCounter=0;

	if (noBebopdAckYet){

            if ((dclock() - pctStartTime) > BEBOPD_GIVEUP){
                 fprintf(stderr,
                   "%c%c%c (%d) No response from bebopd in %f seconds, giving up\n",
                    BELL,BELL,BELL, _my_pnid, BEBOPD_GIVEUP);

		 cleanup_pct();
		 log_quit("No response from bebopd in %f seconds, giving up",
                                 BEBOPD_GIVEUP);
            }
	    /*
	    ** register myself with the bebopd
	    */
	    if ((dclock() - lastBebopdUpdate) > BEBOPD_INTERVAL){
		rc = send_to_bebopd(PCT_INITIAL_MSG);

		if (rc){
		    log_warning("send initial status to bebopd");

		    if (CPerrno != ESENDTIMEOUT){
		        bnid = SRVR_INVAL_NID;
		        bpid = SRVR_INVAL_PID;
		        bptl = SRVR_INVAL_PTL;
		        cleanup_pct();
		        log_quit("DONE");
		    }
		}
		lastBebopdUpdate = dclock();
	    }
	}

        LOCATION("yod_request_check","");
        rc = yod_request_check(&load_req);

        if (rc < 0){
            log_warning("checking for yod requests");
            cleanup_pct();
            log_quit("DONE");
        }
        if (rc == 1){

            LOCATION("process_yod_message","");
            rc = process_yod_message(&load_req);

            if (rc < 0){
                log_warning("processing yod message");
                cleanup_pct();
                log_quit("DONE");
            }
            free_yod_message(&load_req);
        }

        LOCATION("tvdsvr_request_check","");
        rc = tvdsvr_request_check(&tvdsvr_req);

        if (rc < 0) {
            log_warning("main: checking for tvdsvr requests");
            cleanup_pct();
            log_quit("DONE");
        }
        if (rc == 1) {
            LOCATION("process_tvdsvr_req","");
            log_msg("process_tvdsvr_req()");
            rc = process_tvdsvr_req(&tvdsvr_req);
            if (rc < 0) {
                log_warning("main: processing tvdsvr request");
                cleanup_pct();
                log_quit("DONE");
            }
            free_tvdsvr_message(&tvdsvr_req);
        }
        if (tvdsvrpid > 0) {
          if ( tvdsvrpid == waitpid( tvdsvrpid, NULL, WNOHANG) ) {
            tvdsvrpid = 0;
          }
        }

        LOCATION("bt_request_check","");
        rc = bt_request_check(&bt_req);

        if (rc < 0) {
          log_warning("main: checking for bt requests");
          cleanup_pct();
          log_quit("DONE");
        }
        if (rc == 1) {
          
          LOCATION("process_bt_req","");
          rc = process_bt_req(&bt_req);

          if (rc < 0) {
            log_warning("main: processing bt request");
            cleanup_pct();
            log_quit("DONE");
          }
          free_bt_message(&bt_req);
        }

        /* check for gdb wrapper loitering */
        if (gwrap_pid > 0) {
          if ( gwrap_pid == waitpid(gwrap_pid, &gwrap_status, WNOHANG) ) {
            gwrap_pid = 0;
          }
        }

        LOCATION("cgdb_request_check","");
        rc = cgdb_request_check(&cgdb_req);

        if (rc < 0) {
          log_warning("main: checking for cgdb requests");
          cleanup_pct();
          log_quit("DONE");
        }
        if (rc == 1) {
          
          LOCATION("process_cgdb_req","");
          rc = process_cgdb_req(&cgdb_req);

          if (rc < 0) {
            log_warning("main: processing cgdb request");
            cleanup_pct();
            log_quit("DONE");
          }
          free_cgdb_message(&cgdb_req);
        }

	if ((State.status == STATUS_BUSY) && nice_kill_timeout()){
	    if (Dbglevel){
	        log_msg("Time to kill job that was sent SIGTERM awhile ago\n");
	    }
	    pl = current_proc_entry();
	    send_done_to_yod(pl, pl->pid,  0, SIGKILL, time(NULL));
	    reset_pct();
	}

        if ((State.status == STATUS_BUSY) || (appHostReq.todo != TODO_NULL)){

            LOCATION("go app host","");

            appHostReply = go_app_host(&appHostReq, appHostData, appHostDataLen);

            appHostReq.todo = TODO_NULL;
            appHostData = NULL;
            appHostDataLen = 0;
 
            if (appHostReply->rc != APP_HOST_OK){

                if (Dbglevel >= 1){
                    log_msg("exception returned by pct app host\n");
                }
                if (appHostReply->u_stat){
                    /*
                    ** note any interesting changes to app process 
                    ** prior to error
                    */
                    update_status(NOUPDATE, appHostReply->u_stat);
                }
                switch (appHostReply->rc){

                    case LAUNCH_ERR_EXEC:
                        log_warning("exec of app process failed");
                        send_failure_to_yod(current_proc_entry(),
                           appHostReply->pid, LAUNCH_ERR_EXEC);

                        reset_pct();
                        break;

                    case LAUNCH_ERR_FATAL:
                        log_warning("fatal error in application host");
                        cleanup_pct();
                        log_quit("DONE");
                        break;

                    default: 
                        log_warning("failure in application host, we'll reset pct");
                        send_failure_to_yod(current_proc_entry(),
                            appHostReply->pid, appHostReply->rc );

                        reset_pct();
                        break;
                }
                loading = FALSE;
            }
            else if (appHostReply->attn){
                process_app_state_change(appHostReply);
            }
        }

        LOCATION("bebopd_request_check","");
        rc = bebopd_request_check(&bebopd_req);

        if (rc < 0){
            log_warning("checking for bebopd requests");
            cleanup_pct();
            log_quit("DONE");        
        }
        if (rc == 1){

            LOCATION("process_bebopd_requests","");
            rc = process_bebopd_requests(&bebopd_req);

            if (rc < 0){
                log_warning("processing bebopd message");
                cleanup_pct();
                log_quit("DONE");
            }
            LOCATION("free_bebopd_message","");
            free_bebopd_message(&bebopd_req);
        }

        if (!loading){
            usleep(200000);
            //sched_yield();    
        }
    }
    return 0;
}
/********************************************************************************/
/*        command line arguments                                                */
/********************************************************************************/
static struct option pct_options[]=
{
   {"Debug", no_argument, 0, 'D'},
   {"nid", required_argument, 0, 'N'},
   {"pid", required_argument, 0, 'P'},
   {"S", required_argument, 0, 'S'},
   {"L", required_argument, 0, 'L'},
   {"X", no_argument, 0, 'X'},
	{"H", required_argument, 0, 'H'}, 
   {"daemon", no_argument, 0, 'd'},
   {0, 0, 0, 0}
};

static int
get_pct_options(int argc, char *argv[])
{
int opttype;	


    CLEAR_ERR;

    optind = 0;
    daemon_mode = 0;

    LOCATION("get_pct_options","top");
 
    while(1){
 
        opttype = getopt_long_only(argc, argv, "+", pct_options, 0);
 
        if (opttype == EOF){
            break;
        }

        switch (opttype){

            case 'L':
                if (*optarg == '0'){
                    log_to_file(0);
                }        
                break;

            case 'S':
                if (*optarg == '1'){
                    log_to_stderr(1);
                }        
                break;

            case 'D':
                Dbglevel++;
                break;

            case 'N':

                bnid = atoi(optarg);
		bnidGiven = 1;
                break;

            case 'P':

                bpid = atoi(optarg);
                break;

		      case 'H':
				    
				    if (*optarg == '0') health_check = FALSE;
				    break;

	    case 'd':
		
		daemon_mode = 1;
		break;

            default:        

                CPerrno = EINVAL;
                return -1;
        }
    }
    return 0;
}
static void
usage_message()
{
fprintf(stderr,
"      pct  -nid {id}  -pid {id}  -d -D -S [0|1] -L [0|1] \n\n");
fprintf(stderr,
"        -nid   Node ID of bebopd daemon (default is value in cplant-host file)\n"
"        -pid   Portal process ID of bebopd daemon (default is value in cplant-host file)\n"
"        -d     daemon mode - PCT will background itself\n"
"        -D     log debugging information, repeat -D for more output\n"
"        -S [0|1]   0 turns off logging to stderr, 1 turns it on, default is DON'T log to stderr\n"
"        -L [0|1]   0 turns off logging to log file, 1 turns it on, default is DO log to log file\n"
"        -H [0|1]   0 turns off health check, 1 turns it on (default)\n\n"
"  Typical uses:\n"
"       pct -d             run the PCT in daemon mode\n"
"       pct -D -D -S 1     run the PCT in the foreground, lots of debugging output to stderr\n");

}
/********************************************************************************/
/*         pct signal handling                                                  */
/********************************************************************************/
static void
signal_handler(int sig)
{

    takedown_sig_handlers();
 
    switch (sig)   {
        case SIGINT:    log_msg( "PCT received SIGINT (%d)\n", sig);
                        break;
        case SIGQUIT:   log_msg( "PCT received SIGQUIT (%d)\n", sig);
                        break;
        case SIGILL:    log_msg( "PCT received SIGILL (%d)\n", sig);
                        break;
        case SIGTRAP:   log_msg( "PCT received SIGTRAP (%d)\n", sig);
                        break;
        case SIGABRT:   log_msg( "PCT received SIGABRT (%d)\n", sig);
                        break;
        case SIGBUS:    log_msg( "PCT received SIGBUS (%d)\n", sig);
                        break;
        case SIGFPE:    log_msg( "PCT received SIGFPE (%d)\n", sig);
                        break;
        case SIGSEGV:   log_msg( "PCT received SIGSEGV (%d)\n", sig);
                        break;
        case SIGUSR2:    log_msg( "PCT received SIGUSR2 (%d)\n", sig);
                        break;
        case SIGPIPE:   log_msg( "PCT received SIGPIPE (%d)\n", sig);
                        break;
        case SIGALRM:   log_msg( "PCT received SIGALRM (%d)\n", sig);
                        break;
        case SIGTERM:   log_msg( "PCT received SIGTERM (%d)\n", sig);
                        break;
        case SIGCONT:   log_msg( "PCT received SIGCONT (%d)\n", sig);
                        break;
        case SIGTSTP:   log_msg( "PCT received SIGTSTP (%d)\n", sig);
                        break;
        case SIGTTIN:   log_msg( "PCT received SIGTTIN (%d)\n", sig);
                        break;
        case SIGTTOU:   log_msg( "PCT received SIGTTOU (%d)\n", sig);
                        break;
        case SIGURG:     log_msg( "PCT received SIGURG (%d)\n", sig);
                        break;
        case SIGXCPU:   log_msg( "PCT received SIGXCPU (%d)\n", sig);
                        break;
        case SIGXFSZ:   log_msg( "PCT received SIGXFSZ (%d)\n", sig);
                        break;
        case SIGVTALRM: log_msg("PCT received SIGVTALRM (%d)\n", sig);
                        break;
        case SIGPROF:   log_msg( "PCT received SIGPROF (%d)\n", sig);
                        break;
        case SIGPWR:   log_msg( "PCT received SIGGPWR (%d)\n", sig);
                        break;
        default:        log_msg( "Unknown signal (%d) received\n", sig);
                        break;
    }
    log_msg("in %s (%s)\n",routine_name,routine_where);

    if ( (ename == gdbProc) ||
         (ename == gwrapProc) ||
	 (ename == appProc)   ||
	 (sig == SIGPWR )     ||
	 (sig == SIGSEGV)             ){

        log_error("(%s) signal handler - exiting",ename);
    }
    log_msg("signal handler - cleaning up before exit");

    cleanup_pct();

    exit(0);
}  /* end of signal_handler() */
static void
sigusr_handler(int sig)
{
    log_msg( "PCT received SIGUSR1 (%d)\n", sig);
    log_msg("was in %s (%s)\n",routine_name0,routine_where0);
    log_msg("then in %s (%s)\n",routine_name1,routine_where1);
    log_msg("now in %s (%s)\n",routine_name,routine_where);
    log_msg("loop counter %d\n",loopProgressCounter);
    log_msg("PCT status %d\n",State.status);
    log_msg("nice kill interval is %d seconds\n",nice_kill_seconds);
    display_app_state();

    Dbglevel = (Dbglevel + 1) % 5;
    log_msg("Dbglevel = %d", Dbglevel);

    signal(SIGUSR1, sigusr_handler);
}
/*
** A SIGHUP indicates we should re-read the configuration files.
*/
static void
sighup_handler(int sig
#ifdef __GNUC__
__attribute__ ((unused))
#endif
)
{
int count, list, i, rc;

    refresh_config();

    nice_kill_seconds = atoi(nice_kill_interval());
    updateTimeouts();

    sprintf(namebuf,"PCT-%d",_my_pnid);
    log_reopen(namebuf);
    log_msg("got SIGHUP, re-read site configuration file");

    if (!bnidGiven){
	/*
	** re-read the file listing the node on which bebopd is
	** running, unless bebopd node ID was provided on command
	** line.  Send IM ALIVE to this bebopd, but don't worry
	** about an ack, since if its the same bebopd we initialized
	** with it won't send an ack.
	*/
	if ( cplantHost_getNid("bebopd", 1, &list, &count) != OK )
	{
	    log_warning("Error reading Cplant host file on SIGHUP");
            cleanup_pct();
	    log_quit("DONE");
	}
        log_msg("got SIGHUP, re-read cplant host file (bebopd)");

        bnid = list;	

	set_bebopd_id(bnid, bpid, bptl);
        send_to_bebopd(PCT_UPDATE_MSG);
        log_msg("got SIGHUP, send status to bebopd");
    }

    log_msg("got SIGHUP, re-read cplant host file (fyod)");

    for (i=0; i<FYOD_MAP_SZ; i++) {
       fyod_map[i] = -1;
    }
    rc = getFyodMap( fyod_map );
    if (rc){
        log_msg("failure to get fyod map in sighup_handler()");
    }

    signal(SIGHUP, sighup_handler);
}
static void
updateTimeouts()
{
const char *c;

    if ((c = daemon_timeout())) collectiveWaitLimit = atoi(c);
}

static void
setup_sig_handlers(void)
{
int sig;

    for (sig = 0; sig < NSIG; sig++){

        if ( (sig == SIGKILL) ||
             (sig == SIGSTOP) ||
             (sig == SIGCHLD) ||
             (sig == SIGIO) ||
             (sig == SIGUSR1) ||
             (sig == SIGWINCH ) ||
             (sig == SIGHUP ) )    {
  
            continue;
        }

        signal(sig, signal_handler);
    }

    signal(SIGHUP, sighup_handler);
    signal(SIGUSR1, sigusr_handler);

    if (Dbglevel >= 2){
        log_msg("setup_sig_handlers done");
    }

}  /* end of setup_sig_handlers() */

static void
takedown_sig_handlers(void)
{
int sig;

    for (sig = 0; sig < NSIG; sig++){

        if ( (sig == SIGKILL) ||
             (sig == SIGSTOP) ||
             (sig == SIGCHLD) ||
             (sig == SIGIO) ||
             (sig == SIGWINCH ) ){
  
            continue;
        }

        signal(sig, SIG_DFL);
    }

}

/********************************************************************************/
/*    application process setup, maintain process status info                   */
/********************************************************************************/
#define CREATE_OK          0
#define CREATE_NO_SPACE    1
#define CREATE_ERROR       2

static int
stow_executable(proc_list *plist)
{
int rc;
char *newfilenameptr,*lastslash;
char newfilename[128];

 
    LOCATION("stow_executable","0");

    CLEAR_ERR;

    /* if plist->pname contains, scratch_loc change it to argv[0] */
    if ( strstr( plist->pname, scratch_loc ) ) {
     
      /* replace the filename */
      lastslash = strrchr( plist->arglist, 47 );
      if ( lastslash ) {
          newfilenameptr = lastslash++;
      } else {
          newfilenameptr = plist->arglist;
      }
      sprintf(newfilename,"%s/%s",scratch_loc,newfilenameptr);
      if (Dbglevel >= 1) {
          log_msg( "calling rename( %s, %s )\n",plist->pname,newfilename);
      }
      if ( rename( plist->pname, newfilename ) == 0 ) {

          if (strlen(newfilename) > strlen(plist->pname)){

                  free(plist->pname);
                  plist->pname = malloc(strlen(newfilename)+1);

                  if (!plist->pname){
                      CPerrno = ENOMEM;
                      return -1;
                  }
          }

          strcpy(plist->pname, newfilename);
          if (Dbglevel >= 1) {  
              log_msg( "changed filename to %s\n",plist->pname);
          }
      }
    }

    rc = create_exec(plist->pname, plist->user_exec, LoadData.msg2.data.execlen,
		     plist->yod_msg.uid, plist->yod_msg.gid);
 
    LOCATION("stow_executable","1");

#ifdef HAVE_WRITABLE_DISK
    if (rc == CREATE_NO_SPACE){

        char *pathname;

        pathname = disk_disk();

        log_warning(
        "executable of size %d doesn't fit in ram disk, writing to %s\n", 
            LoadData.msg2.data.execlen, pathname);

        plist->pname = tempnam(pathname, "Child");

        if (plist->pname == NULL){
             log_msg("stow_executable: can't generate unique temporary name");
             return LAUNCH_ERR_TEMP_NAME;
        }

        rc = create_exec(plist->pname, plist->user_exec,
                            LoadData.msg2.data.execlen,
		     plist->yod_msg.uid, plist->yod_msg.gid);

        LOCATION("stow_executable","2");

    }
#endif

    if (rc != CREATE_OK) {
        log_warning( "can't write file %s of size %d",
            plist->pname, LoadData.msg2.data.execlen);
        return LAUNCH_ERR_TEMP_NAME;
    }

    return 0;
}
static int
create_exec(char *name, char *user_exec, int size, uid_t uid, gid_t gid)
{ 
int fd, rc, myrc;

    LOCATION("create_exec","open");

    fd = open(name, O_WRONLY, S_IRWXU);
 
    if (fd < 0){
         log_warning("can't open temp file %s",name);
         myrc = CREATE_ERROR;
    }
    else{
        LOCATION("create_exec","write");
         
        lseek(fd, 0, SEEK_SET);

        rc = write(fd, user_exec, size);

        if (rc < 0){
	    log_msg("can't create %s",name);
            myrc = CREATE_ERROR;
        }
        else if (rc < size){
            myrc = CREATE_NO_SPACE;
        }
        else{
            myrc = CREATE_OK;

	    rc = fchown(fd, uid, gid);

            if (rc < 0){
		log_msg("cant create %s for uid %d gid %d",
			    name, uid, gid);
                myrc = CREATE_ERROR;
            }
        }

        LOCATION("create_exec","close");

        close(fd);        

        if (myrc != CREATE_OK){
             unlink(name);
        }
    }
    return myrc;
}
static void
process_app_state_change(appData *reply)
{
proc_list *pl;
int rc;
appID appIDmsg;

    LOCATION("process_app_state_change", "top");

    pl = current_proc_entry();

    if (!pl) return;

    update_status(NOUPDATE, reply->u_stat);

    pl->status |= (int)reply->u_stat;

    if (reply->u_stat & CHILD_NACK){
        log_msg("received CHILD_NACK from job id %d, pid %d",
                          pl->job_id,  pl->pid);

        rc = send_child_nack_to_yod(pl, pl->pid, &(reply->nack));

        if (rc){
            yod_send_error();
        }

        return;
    }
    else if (reply->u_stat & CHILD_DONE){
        if (Dbglevel >= 1){
            log_msg("pct app host reports child done, send status to yod");
        }

        if (reply->bt_size){
             pl->bt_size = reply->bt_size;
        }

        if (!(pl->status & SENT_TO_MAIN)){
            rc = send_child_failure_to_yod(pl, pl->pid, LAUNCH_ERR_CHILD_EXIT,
                          reply->child_rc, reply->child_term_sig);
        }
        else{
            rc = send_done_to_yod(pl, pl->pid, 
                      reply->child_rc, reply->child_term_sig, reply->end_time);
        }
        if (rc){
           yod_send_error();
	   return;
        }

        /* dont wait for the MSG_ALL_DONE to fan in -- we need to get
           the pct's updated status to bebopd _before_ everyone reports
           to yod, not as a result of it. else under pbs yod could initiate 
           a new load assuming the availability of these nodes before they
           are allocatable... note, we cannot reset the pct if there is
           a backtrace pending -- wait for the MSG_GET_BT and then reset */
        if (!reply->bt_size){
          reset_pct();
        }
//        update_status(STATUS_FREE, NOUPDATE);
//        send_to_bebopd(PCT_UPDATE_MSG);

	/*
	** If child process was killed on behalf of a reset request, don't
	** wait for yod to give us an all done.  There may be no yod out
	** there.   Reset requests are the last resort for cleaning up
	** a compute node and returning it to FREE status.
	*/
	if (State.user_status & SENT_KILL_2){
	    reset_pct();
	}
        return;
    }

    if (reply->u_stat & STARTED){
	/*
	** App has started, it's in pre-main startup code.
	*/
        if (Dbglevel >= 1){
            log_msg("pct app host reports child started, pid %d",reply->pid);
        }
	pl->pid = reply->pid;

	init_child_pid(reply->pid, reply->start_time);

    }
    if (reply->u_stat & GOT_PORTAL_ID){

        if (Dbglevel >= 1){
            log_msg("pct app host reports child portal id %d",reply->ppid);
        }
	pl->ppid               = reply->ppid;
        appIDmsg.ppid          = reply->ppid;
	pl->IDmap[pl->my_rank] = reply->ppid;

        appIDmsg.pid = pl->pid;

        /*
        ** Each pct sends the application Portal ID to yod.  This serves
        ** to synchronize the PCTs after the fork.  yod broadcasts 
        ** the portal ID map to the PCTs for fanout.
        **
        ** If at some point in time we have better control over memory
        ** management, the PCTs can use global communication to collect
        ** the portal ID map and send it to yod.  But for now PCTs must not 
        ** exchange messages from the time they call fork() until they 
        ** get the portal ID map back from yod.
        */
        LOCATION("process_app_state_change", "send PORTAL ID msg");

        rc = srvr_send_to_control_ptl(pl->srvr.nid, pl->srvr.pid,
                pl->srvr.ptl, APP_PORTAL_ID,
                (char *)&appIDmsg, sizeof(appID));
 
        if (rc < 0){
            log_warning("error sending APP_PORTAL_ID to yod %d/%d/%d",
                pl->srvr.nid, pl->srvr.pid, pl->srvr.ptl);
            yod_send_error();
        }
    }

    if (reply->u_stat & GOT_READY){

        if (Dbglevel >= 1){
            log_msg("pct app host reports child ready, send READY to yod %d/%d/%d",
                    pl->srvr.nid, pl->srvr.pid, pl->srvr.ptl);
        }

        if (DebugSpecial){
            td1 = dclock();
        }
        LOCATION("process_app_state_change", "send READY msg");

        rc = dsrvr_barrier(collectiveWaitLimit, NULL, 0);

        if (rc != DSRVR_OK){
            log_warning("failure in collective barrier");
	    if ( (CPerrno == ERECVTIMEOUT) || (CPerrno == ESENDTIMEOUT)) { 
		log_msg("%s",dsrvr_who_failed());
		send_group_failure_to_yod(pl, pl->pid);
	    }
	    else {
		send_failure_to_yod(pl, pl->pid, LAUNCH_ERR_PORTAL_ERR);
	    }

            reset_pct();
        }

        /*
        ** all PCTs have received the READY message
        */
        if (dsrvrMyGroupRank == 0){
	    rc = srvr_send_to_control_ptl(pl->srvr.nid, pl->srvr.pid,
	   	    pl->srvr.ptl, APP_READY_MSG,
                    NULL, 0);
 
	    if (rc < 0){
	        log_warning("error sending APP_READY_MSG to yod %d/%d/%d",
		    pl->srvr.nid, pl->srvr.pid, pl->srvr.ptl);
	        yod_send_error();
	    }
        }
    }
    
    /*
    **
    */
    if ((reply->u_stat & SENT_TO_MAIN) ||
        (reply->u_stat & SENT_KILL_1)  ||
        (reply->u_stat & SENT_KILL_2)    ){  

        loading = FALSE;
    }
}
/********************************************************************************/
/*         pct yod communications                                               */
/********************************************************************************/

#define IGNORE_IT(type) \
        log_msg("ignoring %s from %d/%d, unknown job ID %d", \
                type, src_nid, src_pid, job_id); \
        return 0; 

#define YOD_CONTROL_MSG_START(type) \
    job_id = *(int *)(user_data); \
    if (Dbglevel >= 1){                         \
        log_msg("rec'd %s for job %d from %d/%d\n", \
             type, job_id, src_nid, src_pid); \
    } \
    plist = get_proc_list(job_id);    \
    if (plist == NULL){               \
        log_msg("ignoring %s from %d/%d, unknown job ID %d", \
                type, src_nid, src_pid, job_id); \
        return 0; \
    }
        

#define MALLOC_REPLY_BUF(type, len1, len2, buf) \
    if ((req_len < len1) ||           \
        (req_len > len2) ){           \
        log_msg("ignoring %s from %d/%d, len is %d", type,  \
                src_nid, src_pid, req_len);  \
        return 0;                                 \
    }                                             \
    if (req_len > 0){                   \
        buf = (char *)malloc(req_len);  \
        if (!buf){                                \
            CPerrno = ENOMEM;                     \
            log_warning("malloc %d for %s",req_len, type);  \
            return -1;  \
        }               \
    }               \
    else{           \
        buf = NULL; \
    }

#define PUT_REPLY(type, buf) \
    if (req_len > 0){                   \
        rc = srvr_comm_put_reply(load_req, buf, req_len); \
        if (rc < 0){                           \
            log_msg("Error in %s reply, to %d/%d",         \
                type, src_nid, src_pid); \
            reset_pct();     \
        }                                       \
    }                                      \
    if (Dbglevel >= 2){                    \
            log_msg("%s completed", type); \
    }

#define BROADCAST_LOAD_DATA(buf, len, udata, udatalen) \
    if (Dbglevel >= 2){                    \
            log_msg("fanout before bcast"); \
    }                                       \
    rc = fanout_control_message(msg_type, (char *)udata, \
                udatalen, 2, 0, plist->nprocs);  \
    if (rc){                                               \
        reset_pct();                                       \
    }                                                      \
    if (Dbglevel >= 2){                    \
        log_msg("bcast %x %d",buf,len); \
    }                                       \
    rc = dsrvr_bcast(buf, len, collectiveWaitLimit, 0, NULL, 0); \
    if (rc){                                                   \
        log_warning("failure in collective broadcast 0");        \
	if ( (CPerrno == ERECVTIMEOUT) || (CPerrno == ESENDTIMEOUT)) { \
            log_msg("%s",dsrvr_who_failed());                      \
            send_group_failure_to_yod(plist, plist->pid); \
	}                                             \
	else {                                        \
            send_failure_to_yod(plist, 0, LAUNCH_ERR_PORTAL_ERR); \
	}                                             \
        reset_pct();                                  \
     }

#define BROADCAST_SUBGROUP_LOAD_DATA(buf, len, udata, udatalen) \
    if (Dbglevel >= 2){                    \
            log_msg("fanout before subgroup bcast"); \
    }                                       \
    rc = fanout_control_message(msg_type, (char *)udata, \
        udatalen, 2, plist->subgroupRanks[0], plist->subgroupSize);  \
    if (rc){                                               \
        reset_pct();                                       \
    }                                                      \
    if (Dbglevel >= 2){                    \
        log_msg("subgroup bcast %x %d",buf,len); \
    }                                       \
    rc = dsrvr_bcast(buf, len, collectiveWaitLimit, 0,  \
             plist->subgroupRanks, plist->subgroupSize);   \
    if (rc){                                                   \
        log_warning("failure in collective broadcast 1");        \
	if ( (CPerrno == ERECVTIMEOUT) || (CPerrno == ESENDTIMEOUT)) { \
            log_msg("%s",dsrvr_who_failed());                      \
            send_group_failure_to_yod(plist, plist->pid); \
	}                                             \
	else {                                        \
            send_failure_to_yod(plist, 0, LAUNCH_ERR_PORTAL_ERR); \
	}                                             \
        reset_pct();                                          \
    }

static int
process_yod_message(control_msg_handle *load_req)
{
int job_id, rc, retPtl, timeleft;
char *user_data;
int src_nid, src_pid, req_len;
proc_list *plist;
int status, msg_type, len;
send_sig *sigType;
struct stat sbuf;
char *btbuf;

    CLEAR_ERR;

    LOCATION("process_yod_message", "top");

    status = 0;   /* set non-zero only on pct fatal error */

    msg_type = SRVR_HANDLE_TYPE(*load_req);
    src_nid  = SRVR_HANDLE_NID(*load_req);
    src_pid  = SRVR_HANDLE_PID(*load_req);
    req_len = SRVR_HANDLE_TRANSFER_LEN(*load_req);
    user_data = SRVR_HANDLE_USERDEF(*load_req);

    /*
    ** These "yod" messages are load messages arriving from
    ** yod or from fellow PCTs fanning in/out to us.
    **
    ** On error, the PCT just drops out and resets itself.
    ** Yod will timeout and send reset messages to all PCTs.
    */

    switch (msg_type){

#ifndef STRIPDOWN /* no longer done */
        case MSG_REQUEST_TO_LOAD:

	    job_id = ((load_msg1 *)user_data)->job_id;
	    retPtl = ((load_msg1 *)user_data)->yod_id.ptl;

            if (Dbglevel >= 1){
                log_msg("REQUEST_TO_LOAD from %d/%d, job ID %d",
                         src_nid, src_pid, job_id);
            }

	    if (pending_for() != job_id){
	        srvr_send_to_control_ptl(src_nid, src_pid, retPtl,
		        REJECT_LOAD_MSG, NULL, 0);

                log_msg("rejecting request to load job %d - I'm not allocated to it\n",
		          job_id);
	    }
	    else{

	        restart_pending_timeout();

		if (State.status == STATUS_FREE){
		    srvr_send_to_control_ptl(src_nid, src_pid, retPtl,
			    OK_TO_LOAD_MSG, NULL, 0);
		}

		else if (State.status == STATUS_BUSY){

		    timeleft = nice_kill_time_left();

		    srvr_send_to_control_ptl(src_nid, src_pid, retPtl,
			    TRY_AGAIN_MSG, (char *)&timeleft, sizeof(int));
		}
	    }

	    break;
	
        case MSG_CANCEL_REQUEST_TO_LOAD:

            job_id = *(int *)(user_data);

            if (Dbglevel >= 1){
                log_msg("CANCEL_REQUEST_TO_LOAD job %d from %d/%d", 
                             job_id, src_nid, src_pid);
	    }

            if (job_id == pending_for()){
                stop_pending_for();
            }
	    break;
#endif /* !STRIPDOWN */

        case MSG_INIT_LOAD:    /* yod sends this to all pcts */

            if (Dbglevel >= 1){
                log_msg("INIT_LOAD from %d/%d", src_nid, src_pid);
	    }

	    job_id = ((load_msg1 *)user_data)->job_id;

#ifdef STRIPDOWN
            /* when we were allocated we marked state "allocated" and send to
               bebopd */
	    if ((job_id != State.pending_jid) || 
                           (State.status != STATUS_ALLOCATED)){
#else
	    if ((pending_for() != job_id) || (State.status != STATUS_FREE)){
#endif
	       /*
	       ** should never happen
	       */
	       IGNORE_IT("INIT_LOAD");
	    }

	    stop_pending_for();

            LOCATION("process_yod_message", "memcpy");

            memcpy(&init_msg, user_data, sizeof(load_msg1));

            LOCATION("process_yod_message", "init_new_job");

            rc = init_new_job();
         
            if (rc){
                LOCATION("process_yod_message", "reset_pct");
                reset_pct();
                break;     /* drop this job */
            }

            /*
            ** pull in load data and pct list
            */
            LOCATION("process_yod_message", "pull load data");

            YOD_CONTROL_MSG_START("INIT_LOAD");

            PUT_REPLY("PUT_DATA", &LoadData);

            if (Dbglevel >= 1){
                log_msg("Pulled in pct map from yod");
            }

            LOCATION("process_yod_message", "allocate load data");

            rc = allocate_load_data(plist);

            if (rc){
                LOCATION("process_yod_message", "reset pct 2");
                reset_pct();
                break;
            }
            LOCATION("process_yod_message", "init_child_owner");

            /*
            ** set child status fields exported by pct_comm 
            ** in response to bebopd queries
            */
            init_child_job_id(plist->job_id, plist->session_id, plist->parent_id);
            init_child_rank(plist->my_rank);
            init_child_owner(LoadData.msg2.euid);
            init_child_priority(LoadData.msg2.priority);
 
            if (LoadData.msg2.option_bits & OPT_SPECIAL) {
                DebugSpecial = 1;
            }
            else{
                DebugSpecial = 0;
            }

            /*
            ** Initialize PCT group - Sucessful return means all
            ** PCTs agree on membership
            */
            LOCATION("process_yod_message", "initialize_pct_group");

            rc = initialize_pct_group(LoadData.thePcts, init_msg.nprocs,
                       init_msg.job_id);

            if (rc < 0){
                log_warning("failure to init pct group for job %d",
                    init_msg.job_id);
                LOCATION("process_yod_message", "reset pct 3");
                reset_pct();
                break;
            }

            if (Dbglevel >= 1){
                log_msg("pct group successfully initialized");
            }

            /*
            ** Do all PCTs hosting the same executable as I have space
            ** in RAM disk to store it?
            */
            LOCATION("process_yod_message", "determine_exec_location");

            LocalExecStorage = determine_executable_location(plist);

            if (LocalExecStorage < 0){
                log_warning("failure to vote on exec location");
                LOCATION("process_yod_message", "reset pct 4");
                reset_pct();
                break;
            }

            /*
            ** Root PCT for each executable sends OK to yod.
            */
            if (dsrvrMyGroupRank == (plist->subgroupRanks[0])){
                LOCATION("process_yod_message", "send OK to yod");
                rc = srvr_send_to_control_ptl(init_msg.yod_id.nid, 
                    init_msg.yod_id.pid,
                    init_msg.yod_id.ptl, 
                    (LocalExecStorage ? SEND_EXEC_MSG : COPY_EXEC_MSG) ,

                    (char *)&(plist->bt_size), /* <- which nid has a RAM disk problem */

		    sizeof(int));

                if (rc < 0){
                    log_warning("error sending OK to yod %d/%d/%d",
                    init_msg.yod_id.nid, init_msg.yod_id.pid,
                    init_msg.yod_id.ptl);

                    status = -1;
                    break;
                }
            }
            loading = TRUE;

#ifndef STRIPDOWN /* notify bebopd when we finish running a job */
            send_to_bebopd(PCT_UPDATE_MSG);
#endif
            break;

        case MSG_PUT_ARGS:

            YOD_CONTROL_MSG_START("PUT_ARGS");

            if (dsrvrMyGroupRank == plist->subgroupRanks[0]){
                LOCATION("process_yod_message", "pull args from yod");
                PUT_REPLY("PUT_ARGS", plist->arglist);
            }
            LOCATION("process_yod_message", "broadcast arg list");

            BROADCAST_SUBGROUP_LOAD_DATA(plist->arglist, 
                            LoadData.msg2.data.argbuflen, user_data, sizeof(int));

            status = 0;

            break;

        case MSG_PUT_ENV:

            YOD_CONTROL_MSG_START("PUT_ENV");

            if (dsrvrMyGroupRank == 0){
                LOCATION("process_yod_message", "pull env from yod");
                PUT_REPLY("PUT_ENV", plist->envlist);
            }
            LOCATION("process_yod_message", "broadcast env list");

            BROADCAST_LOAD_DATA(plist->envlist, LoadData.msg2.envbuflen, user_data, sizeof(int));

            status = 0;

            break;

        case MSG_PUT_GROUPS:

            YOD_CONTROL_MSG_START("PUT_GROUPS");

            if (dsrvrMyGroupRank == 0){
                LOCATION("process_yod_message", "pull groups from yod");
                PUT_REPLY("PUT_GROUPS", plist->groupList);
            }
            LOCATION("process_yod_message", "broadcast group list");

            BROADCAST_LOAD_DATA((char *)(plist->groupList),
		     LoadData.msg2.ngroups * sizeof(gid_t), user_data, sizeof(int));

            status = 0;

            break;

        case MSG_PUT_STRACE:

            YOD_CONTROL_MSG_START("PUT_STRACE");

            if (dsrvrMyGroupRank == 0){
                LOCATION("process_yod_message", "pull strace request from yod");
                PUT_REPLY("PUT_STRACE", plist->strace);
            }
            LOCATION("process_yod_message", "broadcast strace request");

            BROADCAST_LOAD_DATA((char *)(plist->strace),
		     LoadData.msg2.straceMsgLen, user_data, sizeof(int));

            status = 0;

            break;


        case MSG_PUT_EXEC:

            YOD_CONTROL_MSG_START("PUT_EXEC");

            if (plist->user_exec == NULL){   /* was not expecting this */
                IGNORE_IT("PUT_EXEC");
            }

            if (DebugSpecial){
                td1 = dclock();
            }
            if (dsrvrMyGroupRank == plist->subgroupRanks[0]){
                LOCATION("process_yod_message", "pull exec from yod");
                PUT_REPLY("PUT_EXEC", plist->user_exec);
                if (DebugSpecial){
                    td2 = dclock();
                    log_msg("TIMING: %f to pull in executable",td2-td1);
                }
            }
            if (DebugSpecial){
                td1 = dclock();
            }

            LOCATION("process_yod_message", "broadcast exec");

            /*
	    ** Bad hardware often first displays itself by corrupting
	    ** the executable file as it is sent to the node.  Let's
	    ** detect this with a check sum and abort the broadcast 
	    ** and tell yod.
	    */
            dsrvr_bcast_cksum = ((sendExec *)user_data)->cksum;
	    dsrvr_do_cksum    = 1;

            BROADCAST_SUBGROUP_LOAD_DATA(plist->user_exec, 
                        LoadData.msg2.data.execlen, user_data, sizeof(sendExec));
     
            if (DebugSpecial){
                log_msg("TIMING: %f in executable broadcast",dclock()-td1);
            }
	    dsrvr_do_cksum    = 0;
            /*
            ** If broadcast failed, pct was reset and loading == FALSE
            */
            if (!loading){
                break;
            }
            LOCATION("process_yod_message", "stow_executable");
 
            if (DebugSpecial){
               td3 = dclock();
            }

            rc = stow_executable(plist);
 
            if (rc){
                log_warning("failure to stow executable");
                send_failure_to_yod(plist, 0, rc);
                LOCATION("process_yod_message", "reset pct");
                reset_pct();
                break;
            }
            if (DebugSpecial){
                log_msg("  TIMING: %f to stow executable",dclock()-td3);
            }
 
            appHostReq.todo     = TODO_START;
            appHostData         = (char *)current_proc_entry();
            appHostDataLen      = sizeof(proc_list);

            break;

        case MSG_PUT_EXEC_PATH:

            YOD_CONTROL_MSG_START("PUT_EXEC_PATH");

            if (DebugSpecial){
                td1 = dclock();
            }
            if (plist->user_exec != NULL){   /* was not expecting this */
                IGNORE_IT("PUT_EXEC_PATH");
            }

            if (dsrvrMyGroupRank == plist->subgroupRanks[0]){
                LOCATION("process_yod_message", "pull exec path from yod");
                PUT_REPLY("PUT_EXEC_PATH", plist->pname);
                if (DebugSpecial){
                    td2 = dclock();
                    log_msg("TIMING: %f to pull in executable name",td2-td1);
                }
            }
            if (DebugSpecial){
                td1 = dclock();
            }

            LOCATION("process_yod_message", "broadcast exec path");

            BROADCAST_SUBGROUP_LOAD_DATA(plist->pname,  MAXPATHLEN, user_data, sizeof(int));
 
            if (DebugSpecial){
                td2 = dclock();
                log_msg("TIMING: %f in executable name broadcast",td2-td1);
            }
 
            /*
            ** If broadcast failed, pct was reset and loading == FALSE
            */
            if (!loading){
                break;
            }
            /*
            ** Verify we can execute this file
            */
            rc = stat(plist->pname, &sbuf);
 
            if (rc || !(sbuf.st_mode & S_IXUSR)){
                log_msg("stat %s, rc %d  mode 0x%x, %d\n",
                        plist->pname, rc, sbuf.st_mode,
                        sbuf.st_mode & S_IXUSR);
                send_failure_to_yod(plist, 0, LAUNCH_ERR_EXEC_PATH);
                LOCATION("process_yod_message", "reset pct");
                reset_pct();
                break;
            }
 
            appHostReq.todo = TODO_START;
            appHostData         = (char *)current_proc_entry();
            appHostDataLen      = sizeof(proc_list);
            
            break;

        case MSG_PUT_PORTAL_IDS:

            YOD_CONTROL_MSG_START("PUT_PORTAL_IDS");

            if (dsrvrMyGroupRank == 0){
                LOCATION("process_yod_message", "pull portal IDs from yod");
                PUT_REPLY("PUT_PORTAL_IDS", plist->IDmap);
            }
            LOCATION("process_yod_message", "broadcast portal ID list");

            len = sizeof(ppid_type) * plist->nprocs;
            BROADCAST_LOAD_DATA((char *)(plist->IDmap), len, user_data, sizeof(int));

            appHostReq.todo     = TODO_GETPIDS;
            appHostData         = (char *)(plist->IDmap);
            appHostDataLen      = len;

            status = 0;

            break;

        case MSG_GO_MAIN:

            YOD_CONTROL_MSG_START("GO_MAIN");

            LOCATION("process_yod_message", "fanout GOMAIN");

            rc = fanout_control_message(MSG_GO_MAIN, (char *)&job_id,
                       sizeof(int), 2, 0, plist->nprocs);
            if (rc){
                LOCATION("process_yod_message", "reset pct 5");
                reset_pct();
                break;
            }
            if (!(plist->status & GOT_READY)){
                CPerrno = EPROTOCOL;
                send_failure_to_yod(plist, plist->pid, LAUNCH_ERR_MISC);
                LOCATION("process_yod_message", "reset pct 6");
                reset_pct();
                break;
            }

            appHostReq.todo = TODO_GOMAIN;
            
            break;

        case MSG_GET_BT:

            YOD_CONTROL_MSG_START("GET_STACK_TRACE");

            update_status(STATUS_FREE, NOUPDATE);
            send_to_bebopd(PCT_UPDATE_MSG);

            if (plist->bt_size > 0){

                btbuf = get_back_trace();

                LOCATION("process_yod_message", "send back trace to yod");

                if (btbuf){
                    rc = srvr_comm_get_reply(load_req, btbuf, plist->bt_size);
                }
                else{
                    log_msg("unable to access back trace");
                }

                if (!btbuf || rc){
                    send_failure_to_yod(plist, plist->pid, LAUNCH_ERR_YOD_REPLY);
                    LOCATION("process_yod_message", "reset pct 6");
                    reset_pct();
                }
            }
            else{
                log_msg("unexpected back trace requested by yod, ignored");
            }

            reset_pct2();

            break;

#if 0
        case MSG_ALL_DONE:

            YOD_CONTROL_MSG_START("ALL_DONE");

            LOCATION("process_yod_message", "fanout ALL DONE");

            rc = fanout_control_message(MSG_ALL_DONE, (char *)&job_id,
                    sizeof(int), 2, 0, plist->nprocs);
            if (rc){
                log_warning("fanout MSG_ALL_DONE to other pcts");
            }

            if (!(plist->status & CHILD_DONE)){

                CPerrno = EPROTOCOL;
                log_msg("ignoring ALL_DONE from yod on jobid %d\n",
                        job_id);

                return 0;
            }
            appHostReq.todo = TODO_ALLDONE;

            LOCATION("process_yod_message", "reset pct 8");

            reset_pct();

            status = 0;

            break;
#endif

        case MSG_SEND_SIGUSR:

            YOD_CONTROL_MSG_START("SIGUSR");

            LOCATION("process_yod_message", "fanout MSG_SEND_SIGUSR");

            rc = fanout_control_message(MSG_SEND_SIGUSR, user_data,
                    sizeof(send_sig), 2, 0, plist->nprocs);
            if (rc){
                log_warning("fanout MSG_SEND_SIGUSR to other pcts");
            }

            sigType = (send_sig *)user_data;

            if (sigType->type == 1){
               appHostReq.todo = TODO_SIGUSR1;
            }
            else if (sigType->type == 2) {
               appHostReq.todo = TODO_SIGUSR2;
            }

            status = 0;

            break;

        case MSG_ABORT_LOAD_1:

            YOD_CONTROL_MSG_START("ABORT_LOAD FIRST TRY");

            LOCATION("process_yod_message", "fanout MSG_ABORT_LOAD_1");

            rc = fanout_control_message(MSG_ABORT_LOAD_1, (char *)&job_id,
                    sizeof(int), 2, 0, plist->nprocs);

            if (rc){
                log_warning("fanout MSG_ABORT_LOAD_1 to other pcts");
            }

            update_terminator(LoadData.msg2.euid);

            appHostReq.todo = TODO_ABORT1;

            status = 0;

            break;

        case MSG_ABORT_LOAD_2:

            YOD_CONTROL_MSG_START("ABORT_LOAD SECOND TRY");

            LOCATION("process_yod_message", "fanout MSG_ABORT_LOAD_2");

            rc = fanout_control_message(MSG_ABORT_LOAD_2, (char *)&job_id,
                        sizeof(int), 2, 0, plist->nprocs);
            if (rc){
                log_warning("fanout MSG_ABORT_LOAD_2 to other pcts");
            }

            update_terminator(LoadData.msg2.euid);

            appHostReq.todo = TODO_ABORT2;

            status = 0;

            break;

        /*
        ** yod sends this directly to each pct since things are really
        ** screwed up at this point and maybe even pcts are not talking
        ** to each other.
        */
        case MSG_ABORT_RESET:

            YOD_CONTROL_MSG_START("ABORT_RESET");

            LOCATION("process_yod_message", "reset pct 9");

            reset_pct();

            status = 0;

            break;

        /*
	** End game goes like this:  Each PCT notifies yod when the
	** process it is hosting terminates.  When all PCTs have notified
	** yod, yod sends a MSG_ALL_DONE to the PCTs and they reset themselves
	** to STATUS_FREE.  If yod exits before completing the end game, PCTs 
	** are left hanging until someone manually resets them with pingd.  
	**
	** To handle this we have the root PCT, once the application process
	** has terminated, check from time to time to see if all processes in
	** the application have terminated.  If so, the PCTs reset themselves 
        ** to STATUS_FREE.
	*/
        case MSG_YODLESS_COMPLETION:

            YOD_CONTROL_MSG_START("YODLESS_COMPLETION");

	    rc = fanout_control_message(MSG_YODLESS_COMPLETION, (char *)&(plist->job_id),
			sizeof(int), 2, 0, plist->nprocs);

	    if (rc){
		log_warning("can't fanout MSG_YODLESS_COMPLETION to other pcts");
	    }
            else if (vote_on_completion(plist)){

		if (Dbglevel){
		    log_msg("yodless completion protocol indicates a reset");
		}

		reset_pct();
	    }

            status = 0;

            break;


        default:

            log_msg("ignoring msg type %d on yod msg portal from %d/%d\n",
                      msg_type, src_nid, src_pid);

            status = 0;

            break;
    }

    return status;
}

static int
vote_on_completion(proc_list *pl)
{
int i, alldone, rc;


    if (pl->status & CHILD_DONE){
        alldone = 1;
    }
    else{
        alldone = 0;
    }

    rc = dsrvr_vote(alldone, collectiveWaitLimit, 1111, NULL, 0);

    if (rc == DSRVR_EXTERNAL_ERROR){
        log_warning("voting on application completion: failure of %s",dsrvr_who_failed());
        dsrvr_clear_fail_info();

        return alldone;            /* go ahead and reset if I'm done */
    }
    else if (rc != DSRVR_OK){
        log_error("Serious error when voting on application completion - job %d",
                    pl->job_id);
    }

    if (alldone){
	for (i=0; i<pl->nprocs; i++){
	    if (DSRVR_VOTE_VALUE(i) == 0){
		alldone = 0;       /* someone else is not done */
		break;
	    }
	}
    }

    return alldone;
}
/********************************************************************************/
/*         pct - bebopd requests                                                */
/*                                                                              */
/********************************************************************************/


int
process_bebopd_requests(control_msg_handle *req)
{
int msg_type, src_nid, src_pid;
int status, runningJob;
char req_src; 
char *user_data;
pingPct_req *preq;
pctStatus_req *pstat;
proc_list *plist;

    CLEAR_ERR;

    LOCATION("process_bebopd_requests", "top");

    status = 0;      /* set non-zero only on PCT fatal error */

    msg_type = SRVR_HANDLE_TYPE(*req);
    user_data = SRVR_HANDLE_USERDEF(*req);
    src_nid = SRVR_HANDLE_NID(*req);
    src_pid = SRVR_HANDLE_PID(*req);

    noBebopdAckYet = 0;    /* any bebopd message is in effect an ack */

    if (msg_type == PCT_GOTCHA){
        if (Dbglevel){
            log_msg("bebopd sent PCT_GOTCHA");
        }
        return status;
    }
    plist = NULL;
    runningJob = 0;

    if (State.status == STATUS_BUSY){

	plist = current_proc_entry();
    }

    if (plist && (plist->status & STARTED) && !(plist->status & CHILD_DONE)) {

        runningJob = 1;
    }
    /*
    ** PCT_STATUS_REQUEST
    **     request for PCT's status, if request is on behalf of yod, 
    **     and PCT is available, PCT temporarily is allocated to the job
    **
    ** PCT_NOT_ALLOCATED
    **     PCT will not be needed for the job
    **
    ** PCT_INTERRUPT_REQUEST
    **     request to send a SIGTERM to the job
    **
    ** PCT_NICE_KILL_JOB_REQUEST
    **     request to send a SIGTERM to job, and if it doesn't exit
    **     send a SIGKILL some time later
    **
    ** PCT_RESET_REQUEST
    **     Kill off job, if any, and reset PCT back to FREE.  Doesn't
    **     change reservation or pending status.
    **
    ** PCT_DIE_REQUEST
    **     bebopd tells the PCT to exit.
    **
    ** PCT_RESERVE_REQUEST 
    **     PCT will only run jobs owned by holder of reservation.  Present
    **     job will be allowed to complete regardless of owner.
    **
    ** PCT_UNRESERVE_REQUEST
    **     Guess.
    **
    ** PCT_GOTCHA
    **     bebopd acknowledges PCT's initial "I'm alive" message.
    */


    if (msg_type == PCT_STATUS_REQUEST){

        pstat = (pctStatus_req *)user_data;

	DEBUG_STALE_NODES("Received status request");

        req_src = pstat->opsrc;

	if (!check_bebopd_id(src_nid, src_pid)){
	    /*
	    ** A new bebopd has started, replacing the one we knew about.
	    */
	    LOCATION("process_bebopd_requests", "set_bebopd_id");
	    set_bebopd_id( pstat->bnid, pstat->bpid, pstat->bptl);
            log_msg("bebopd has changed - new bebopd is %d/%d/%d\n",
                            pstat->bnid, pstat->bpid, pstat->bptl);
        }
    }

    if ((msg_type == PCT_INTERRUPT_REQUEST) && !runningJob){

	   return status;
    }

    if ((msg_type == PCT_NICE_KILL_JOB_REQUEST) && (State.status != STATUS_BUSY)){

	   return status;
    }

    if ((msg_type == PCT_DIE_REQUEST)       ||
        (msg_type == PCT_RESET_REQUEST)     ||
        (msg_type == PCT_INTERRUPT_REQUEST) ||
        (msg_type == PCT_NICE_KILL_JOB_REQUEST) ||
        (msg_type == PCT_UNRESERVE_REQUEST) ||
        (msg_type == PCT_RESERVE_REQUEST)  ){

        preq = (pingPct_req *)user_data;

        if (preq->jobID != INVAL){
            /*
            ** Only signal, reset or reserve PCTs hosting this Cplant job
            */ 
            if ((State.status != STATUS_BUSY) ||
                (init_msg.job_id != preq->jobID)   ){
            
                return status;
            }
        }
        if (preq->sessionID != INVAL){
            /*
            ** Only signal, reset or reserve PCTs hosting this PBS job
            */ 
            if ((State.status != STATUS_BUSY) ||
                (init_msg.session_id != preq->sessionID)   ){
            
                return status;
            }
        }
        if (preq->euid != -1){

            /*
            ** Only signal, reset or reserve PCTs hosting a job owned by euid
            */     
            if ((State.status != STATUS_BUSY) ||
                ((INT32) LoadData.msg2.euid != preq->euid) ){

                return status;
            }
        }
    }

    if ((msg_type == PCT_NICE_KILL_JOB_REQUEST) && (plist->nice_kill_1)){

       log_msg("ignore NICE_KILL_JOB_REQUEST, we already started a nice kill");

       return status;
    }

    if (msg_type == PCT_RESERVE_REQUEST){
	
	if (!reservation_violation(preq->reserve_uid)){
	    update_reservation(preq->reserve_uid);
	}
	else{
	    log_msg(
	    "Ignoring request to reserve node for %d, since already reserved for someone else.",
	                 preq->reserve_uid);
	}

	return status;
    }
    else if (msg_type == PCT_UNRESERVE_REQUEST){
        clear_reservation();

	return status;
    }

    if ((msg_type == PCT_DIE_REQUEST)       ||
        (msg_type == PCT_RESET_REQUEST)     || 
        (msg_type == PCT_NICE_KILL_JOB_REQUEST)     || 
        (msg_type == PCT_INTERRUPT_REQUEST)     ){

        /* 
        ** accountants need to know if job was killed
        ** by owner or by system administration
        */
        if (runningJob){
	    update_terminator(preq->euid);
	}
    }

    if (msg_type == PCT_DIE_REQUEST){

        if (Dbglevel){
            log_msg("processing bebopd DIE request");
        }

        log_msg("bebopd %d/%d ordered KILL",bnid,bpid);
        LOCATION("process_bebopd_requests", "cleanup_pct");
        cleanup_pct();
        log_quit("DONE");
    }
    else if (msg_type == PCT_RESET_REQUEST){

        if (Dbglevel){
            log_msg("processing bebopd RESET request");
        }

        if (runningJob){

            /*
	    ** We'll send app the SIGKILL in reset_pct()
	    */
	    update_status(NOUPDATE, SENT_KILL_2);

	    send_done_to_yod(plist, plist->pid,  0, SIGKILL, time(NULL));
        }
        LOCATION("process_bebopd_requests", "update_status 0");
        reset_pct();
    }
    else if (msg_type == PCT_INTERRUPT_REQUEST) {

	if (Dbglevel){
	    log_msg("processing bebopd INTERRUPT request");
	}

	appHostReq.todo = TODO_ABORT1;  /* send app proc a SIGTERM */
    }
    else if (msg_type == PCT_NICE_KILL_JOB_REQUEST){

        /*
	** Warning - never allow a nice-kill operation that kills
	** only part of a job.  pingd requires that the entire
	** job be killed.  bebopd only kills entire jobs.  Once
	** a nice-kill starts, the PCT figures it is available
	** to be allocated to a new job.  If the other members
	** of the application have not also been nice-killed,
	** this is not the case.
	**
	** If you write a utility that sends a NICE_KILL_JOB_REQUEST
	** to the PCT, make sure you never nice-kill only part
	** of a job.
	*/

	if (Dbglevel){
	    log_msg("processing bebopd NICE_KILL request");
	}

        if (runningJob){
	    appHostReq.todo = TODO_ABORT1; 
        }

	plist->nice_kill_1 = time(NULL);
	plist->nice_kill_2 = plist->nice_kill_1 + nice_kill_seconds;

        update_status(NOUPDATE, NICE_KILL_JOB_STARTED);
    }
    else if (msg_type == PCT_STATUS_REQUEST){

        if (Dbglevel){
            log_msg("processing bebopd STATUS request");
        }

        /* 
        ** Check for stable conditions on the host, only if not
        ** running a job or pending. Any non-zero return means
        ** problems were found.
        */
        if (health_check) {

            LOCATION("process_bebopd_requests", "check_host_health");

            if ( (State.status != STATUS_BUSY) && !pct_pending()){
                if ( check_host_health() ) 
                    update_status(STATUS_TROUBLE, NOUPDATE);
                else if ( State.status == STATUS_TROUBLE )
                    update_status(STATUS_FREE, NOUPDATE);
            }
        }

        LOCATION("process_bebopd_requests", "send_to_bebopd(PCT_UPDATE_MSG)");

#ifndef STRIPDOWN
        send_to_bebopd(PCT_UPDATE_MSG);

        if ((req_src == REQ_FOR_YOD) && 
            !pct_pending()           &&
	    (   (State.status == STATUS_FREE) || 
	        (State.user_status & NICE_KILL_JOB_STARTED))  ){

                /*
		** Note that we are pending allocation to this new job.  If bebopd
		** doesn't need us, it will send us a NOT_ALLOCATED message.
		*/

		start_pending_for(pstat->jobID, pstat->sessionID);
	    }
    }
#else
        if (req_src == REQ_FOR_YOD) {
	  if (State.status != STATUS_FREE) {
            log_msg("pct: Allocation request from bebopd, but we're not free...\n");
            /* respond to solicit, but dont change state */
            send_to_bebopd(PCT_UPDATE_MSG);
          }
          else {
            /* send bebopd free status -- it will be updated to allocated */
            start_pending_for(pstat->jobID, pstat->sessionID);
            send_to_bebopd(PCT_UPDATE_MSG);
            State.status = STATUS_ALLOCATED;
          }
        }
        else {
          send_to_bebopd(PCT_UPDATE_MSG);
        }
    }
#endif


#ifndef STRIPDOWN /* this is no longer supported */
    else if (msg_type == PCT_NOT_ALLOCATED){

        if (Dbglevel){
            log_msg("processing bebopd NOT_ALLOCATED advisory");
        }

	stop_pending_for();
    }
    else{
        log_msg("ignoring bebopd request %d",msg_type);
    }
#endif

    return status;
}
/********************************************************************************/
/*     resetting and cleaning up                                                */
/********************************************************************************/
static void
clear_appHost()
{

    LOCATION("clear_appHost", "top");
    appHostReq.todo = TODO_NULL;
    appHostReq.dbglevel = Dbglevel;
    appHostReq.debugSpecial = DebugSpecial;

    appHostData = NULL;
    appHostDataLen = 0;
}
static void
yod_send_error()
{
    log_msg("error sending to yod, resetting PCT");
    reset_pct();
}
static void
reset_pct()
{
    LOCATION("reset_main_pct", "recover_proc_list_entry");

    recover_proc_list_entry(current_proc_entry());

    loading = FALSE;

    LOCATION("reset_main_pct", "takedown_pct_group");

    takedown_pct_group();
    srvr_reset_coll();

    clear_appHost(); 
    appHostReq.todo = TODO_RESET;

    LOCATION("reset_main_pct", "go_app_host");

    go_app_host(&appHostReq, appHostData, appHostDataLen);

    update_status(STATUS_FREE, NOUPDATE);
    send_to_bebopd(PCT_UPDATE_MSG);
}
static void
reset_pct2()
{
    LOCATION("reset_main_pct", "recover_proc_list_entry");

    recover_proc_list_entry(current_proc_entry());

    loading = FALSE;

    LOCATION("reset_main_pct", "takedown_pct_group");

    takedown_pct_group();
    srvr_reset_coll();

    clear_appHost(); 
    appHostReq.todo = TODO_RESET;

    LOCATION("reset_main_pct", "go_app_host");

    go_app_host(&appHostReq, appHostData, appHostDataLen);
}
