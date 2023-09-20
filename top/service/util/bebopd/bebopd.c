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
** $Id: bebopd.c,v 1.116.2.9 2002/09/27 01:00:30 jjohnst Exp $
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sched.h>
#define __USE_GNU  /* force __USE_GNU to get sysv_signal from signal.h */
#include <signal.h>
#undef  __USE_GNU  /* it conflicts in other places, e.g. off64_t in qkdefs.h */

#ifdef __linux__
#include <getopt.h>
#endif
#ifdef __osf__
#include "linux-getopt.h"
#endif
#include <sys/time.h>
#include <sys/mman.h>

#include <puma.h>
 
#include "sys_limits.h"
#include "bebopd.h"
#include "bebopd_private.h"
#include "config.h"
#include "appload_msg.h"
#include "srvr_comm.h"
#include "srvr_err.h"
#include "srvr_async.h"
#include "pct_ports.h"
#include "ppid.h"
#include "portal_assignments.h"
#include "cplant.h"
#include "defines.h"


/*
** configuration daemon, keeps track of compute partition, allocates
** nodes to parallel applications.
**
** When a pct is started, it looks up the location of the bebopd
** in a configuration file.  The pct sends an update message to the
** bebopd to register itself. (update_pct_pid_array())
**
** When yod wants to launch an application, it requests a list of
** pcts from the bebopd. (answer_yod_requests())
**
** pingd can request compute nodes status, and certain compute node
** operations, from this daemon.  (answer_ping_requests())
**
** You can send bebopd a SIGUSR1 to see LOCATION information.
*/

int Dbglevel;
int Restart;
char RestartFile[256] = {0};
int minPct, maxPct;
int timing_data;
#ifdef REMAP
int Remap;
char RemapFile[256] = {0};
int minMap, maxMap;

nid_type carray[MAX_NODES]; /* This is remap to node # */
int marray[MAX_NODES]; /* This is node # to remap. Necessary because */
                       /* larray can sometimes be overwritten */
#endif

pct_ID larray[MAX_NODES];
int pct_spids[MAX_NODES];
char loadCandidate[MAX_NODES];
nodeStateTypes nodeState[MAX_NODES];
int nodeStateTotals[numNodeStates];

int totalLoadCandidates;
int totalFreeCandidates;
int interactiveClearingNodes;
int interactiveClearingCandidates;

int daemonWaitLimit=30; 
int clientWaitLimit=5;
int solicitPctsTmout;

int update_ptl = UPDATE_PTL;

static int overwrite_old_registry;
static int alternative;
static char *rname;
static int node_list[MAX_NODES];

static void init_pct_updates(void);
static void init_yod_requests(void);
static void init_ping_requests(void);
#ifdef DYN_ALLOC
static void init_dna_requests(void);
static void answer_dna_requests(void);
#endif
static void answer_yod_requests(void);
static void answer_ping_requests(void);
static void setup_sig_handlers(void);
static void signal_handler(int sig);
static void sighup_handler(int sig);
static void updateTimeouts(void);
static void query_signal(int sig);
static void toggle_debug_signal(int sig);
static void takedown_portals(void);
static int get_options(int argc, char *argv[]);
static void restart_bebopd(void);
static void write_out_pct_data(void);
#ifdef REMAP
int remap();
#endif

static int registry = 0;

static int prevJobID;

static int daemon_mode;

#define FIRSTJOBID    100
#define LASTJOBID   10000

static double td1, td2, td3, td4;

const char *routine_name;
const char *routine_where;

char *pbsUpdateProc = "bebopd (PBS update process)";
char *ename;
int niceKill;

int 
main(int argc, char *argv[])
{
int rc, nupdates;
    LOCATION("main","top");

    log_open("bebopd");

    ename = argv[0];

    rc = get_options(argc, argv);

    if (rc){
        log_quit("invalid usage");
    }

#ifdef REMAP
    if (Remap) {
      remap();
    }
#endif

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


    if (alternative){
	_my_ppid = register_ppid(&_my_taskInfo, PPID_AUTO, GID_BEBOPD, "bebopd");

	if (_my_ppid == 0){
            log_to_stderr(1);
	    log_msg("Can not get a slot in portals module\n");
	    exit(-1);
	}
    }
    else{
	_my_ppid = register_ppid(&_my_taskInfo, PPID_BEBOPD, GID_BEBOPD, "bebopd");

	if (_my_ppid != PPID_BEBOPD){

            log_to_stderr(1);
	    log_msg("Can not register myself as PPID=%d\n", PPID_BEBOPD);
	    log_msg("Perhaps there's another bebopd running?\n");
	    exit(-1);
	}
    }
    if (alternative || Dbglevel){
        log_msg("Node ID: %d Portals PID: %d \n",_my_pnid, _my_taskInfo.ppid);
    }

    rc = server_library_init();

    if (rc){
        log_error("initializing server library");
    }

    updateTimeouts();

    setup_sig_handlers();   /* want to clean up if I get killed */

    /*
    ** Set up a portal to receive pct status requests.
    */
    init_ping_requests();

    /*
    ** Set up a portal to receive updates from remote pcts.
    */
    init_pct_updates();

    /*
    ** Set up well known portal to which local yods can send requests
    ** for remote pct ids.  Register daemon's pid in /tmp.
    */
    init_yod_requests();

#ifdef DYN_ALLOC
    /*
    ** Set up well known portal for handling dynamic node allocation
    ** requests.
    */
    init_dna_requests();
#endif

    /*
    ** List bebopd's nid/pid and portal numbers in a well known place.
    */

    /*
    ** If we are restarting the bebopd, read in the list of active
    ** PCTs and send them a request for current status.  Set the prevJobID
    ** to exceed all the job IDs out there when the last bebopd ended.
    */
    if (Restart){
        restart_bebopd();
    }
    else{
        prevJobID = FIRSTJOBID;
    }
    niceKill = atoi(nice_kill_interval());

    /*
    ** Now alternately check for new pct updates, and check 
    ** for local yod requests.
    */
    LOCATION("main","before while loop");

    while (1){

        nupdates = update_pct_pid_array(COUNT_ALL);

	if (PBSsupport && (nupdates > 0)){
	    compute_active_nodes();
     
	    if (PBSupdate){
		update_PBS_server_with_change();
	    }
	}

        answer_yod_requests();

        answer_ping_requests();

#ifdef DYN_ALLOC
	answer_dna_requests();
#endif

        usleep(200000);
        //sched_yield();
    }
    return 0;
}
static void
usage(void)
{ 
printf("USAGE:\n");
printf(" bebopd options are:\n");
printf("\n");
printf("    -alternative   test mode where portal PID differs from bebopd fixed pid\n\n");
printf("\n");
printf("    -D  generate debugging output\n");
printf("    -S 1    log to stderr\n");
printf("    -S 0    don't log to stderr (the default)\n");
printf("    -L 1    log to cplant log file (the default)\n");
printf("    -L 0    don't log to cplant log file\n");
printf("    -d      deamon mode, bebopd puts itself in the background.\n");
printf("    -PBSsupport   causes bebopd to maintain a count of compute nodes available\n");
printf("                  to scheduled jobs.  bebopd will accept scheduled jobs and\n");
printf("                  enforce allocated node limits.\n");
printf("    -PBSupdate    causes bebopd to update the PBS server's resources_available.size\n");
printf("                  attribute whenever the count changes.  The count may change\n");
printf("                  when a node dies, when a non-PBS job completes, or when\n");
printf("                  a PCT is started on a formerly dead node.  Turning on\n");
printf("                  PBSupdate turns on PBSsupport.\n");
#ifndef STRIPDOWN
printf("    -PBSinteractive [n]  bebopd will reserve [n] nodes for use by interactive\n");
printf("                         jobs.  Default is 0.\n");
printf("    -PBSlist [node-list] bebopd will reserve these nodes for use by interactive jobs\n");
#else
printf("    -PBSlist [node-list|null] Required arg tells bebopd to use either [list|no nodes] for interactive jobs.\n");
#endif

printf("\n");
printf("    -r  {optional-file-name}\n");
printf("\n");
printf("        restart bebopd, read in list of active PCTs saved by\n");
printf("        previous invocation of bebopd.  Default file name is\n");
printf("        \"saved_pct_list\" in same directory as bebopd registry.\n");
printf("        If optional-file-name is specified, this PCT list is\n");
printf("        read instead.\n");
#ifdef REMAP
printf("\n");
printf("    -remap [file name] The required arg gives the name of remap file for \n");
printf("                       the bebopd to use.\n");
#endif
printf("\n");
printf("    -help  display this help message\n");
}

static void
node_list_usage()
{
printf("A node-list is a comma separated list of nodes or node ranges.  A node\n");
printf("range is two node numbers separated by one or more dots.  No white space\n");
printf("in node-list please.  Example: 1,5,7,12..18,100..150,152\n\n");
}

static struct option bebopd_options[] =
{
  {"alternative", no_argument, 0,   'a'},
  {"daemon", no_argument, 0,        'd'},
  {"help", no_argument, 0,          'h'},
  {"PBSinteractive", required_argument, 0, 'i'},
  {"PBSlist", required_argument, 0, 'l'},
  {"PBSsupport", no_argument, 0,    'p'},
  {"restart", optional_argument, 0, 'r'},
  {"PBSupdate", no_argument, 0,     'u'},
  {"Debug", no_argument, 0,         'D'},
  {"L", required_argument, 0,       'L'},
  {"S", required_argument, 0,       'S'},
  {"timing", no_argument, 0,        'T'},
  {"remap", required_argument, 0,   'M'},
  {0,0,0,0}
};

static int 
get_options(int argc, char *argv[])
{
int opttype;
int nnodes=0;

    Dbglevel = 0;
    overwrite_old_registry = TRUE;
    timing_data = FALSE;
    alternative = FALSE;
#ifdef REMAP
    Remap = FALSE;
#endif
    Restart = FALSE;
    PBSsupport = PBSupdate = FALSE;
    daemon_mode = 0;

    LOCATION("get_options","top");

    while (1){
        opttype = getopt_long_only(argc, argv, "+", bebopd_options, 0);

        if (opttype == EOF){
            break;
        }

        switch (opttype){

            case 'a':
                
                alternative = TRUE;
                break;

            case 'd':
                daemon_mode = 1;
                break;

            case 'h':
                
                usage();
                exit(0);
                break;

            case 'i':            /* -PBSinteractive */
#ifndef STRIPDOWN /* just use "-PBSlist node-list|null" */
		if (interactive_list_req_size){
		    printf("Specify \"-PBSlist\" OR \"-PBSinteractive\", not both\n");
		    usage();
		    return -1;
		}

                PBSsupport = TRUE;

		new_interactive_size(atoi(optarg));
#endif
                break;

            case 'l':          /* -PBSlist */
#ifndef STRIPDOWN
		if (interactive_size_req){
		    printf("Specify \"-PBSlist\" OR \"-PBSinteractive\", not both\n");
		    usage();
		    return -1;
		}
#endif
                PBSsupport = TRUE;

                /* empty list */
                if ( strcmp("null",optarg) == 0 ) {
                  optarg = NULL;
                }

		nnodes = parse_node_list(optarg, node_list, MAX_NODES, 0, MAX_NODES-1);

#ifndef STRIPDOWN /* expect a non-void node list */
		if (nnodes <= 0){
		  printf("Invalid node list argument to \"-PBSlist\"\n");
		  node_list_usage();
		  return -1;
		}
		new_interactive_list(node_list, nnodes);

#else /* null list OK */
                if (nnodes < 0) {
                  printf("use \"-PBSlist null\" or\n");
                  printf("    \"-PBSlist node-list\"\n");
                  node_list_usage();
                  return -1;
                }
#endif
                break;

            case 'p':
 
                PBSsupport = TRUE;
                break;

            case 'r':
                
                Restart = TRUE;
                if (optarg){
                    strncpy(RestartFile, optarg, 255);
                }
                break;

            case 'u':
 
                PBSsupport = TRUE;
                PBSupdate  = TRUE;
                break;

            case 'D':
                Dbglevel++;
                break;

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

            case 'M':
#ifdef REMAP
                Remap = TRUE;
                if (optarg){
                    strncpy(RemapFile, optarg, 255);
                }
#endif
                break;

            case 'T':
                
                timing_data = TRUE;
                break;

            default:
                return -1;
                break;
        }
    }
#ifdef STRIPDOWN
    new_interactive_list(node_list, nnodes);
#endif
    return 0;
}

void
finish_bebopd()
{

    LOCATION("finish_bebopd","top");

    if (PBSupdate) {
        activeNodes=0;
	update_PBS_server_with_change();
    }

    if (registry){
        unlink(rname);
    }

    takedown_portals();

    write_out_pct_data();

    log_msg("DONE");
    
    exit(0);
}
static void
setup_sig_handlers(void)
{
int sig;
 
    LOCATION("setup_sig_handlers","top");

    for (sig = 0; sig < NSIG; sig++){
 
        if ( (sig == SIGKILL) ||
             (sig == SIGSTOP) ||
             (sig == SIGUSR1) ||
             (sig == SIGWINCH) ||
             (sig == SIGCONT) ||
             (sig == SIGSTOP) ||
             (sig == SIGTSTP) ||
	     (sig == SIGCHLD) ||
             (sig == SIGTRAP) ||
             (sig == SIGPOLL) ||
             (sig == SIGHUP) ||
             (sig == SIGUSR2) ||
             (sig == SIGFPE)
                                ){
 
            continue;
        }
 
        sysv_signal(sig, signal_handler);
    }

    sysv_signal(SIGUSR1, query_signal);
    sysv_signal(SIGUSR2, toggle_debug_signal);
    sysv_signal(SIGHUP, sighup_handler);

}
static void
signal_handler(int sig)
{
   log_to_stderr(TRUE);
   log_msg("%s received signal %d, cleaning up",ename, sig);

   if (ename == pbsUpdateProc){
      log_error("DONE");
   }

   finish_bebopd();
}
static void
sighup_handler(int sig
#ifdef __GNUC__
__attribute__ ((unused))
#endif
)
{
    log_reopen("bebopd");

    log_msg("bebopd (system PID %d, portal ID %d) : got a SIGHUP ",
                 getpid(),_my_ppid);

    refresh_config();

    updateTimeouts();

    niceKill = atoi(nice_kill_interval());
    sysv_signal(SIGHUP, sighup_handler);
}
static void
updateTimeouts()
{
const char *c;

    if ((c = daemon_timeout()))     daemonWaitLimit = atoi(c);
    if ((c = client_timeout()))     clientWaitLimit = atoi(c);
    if ((c = solicit_pcts_tmout())) solicitPctsTmout = atoi(c);
}
/****************************************************************************
**  Setup for remote pct updates, check for updates coming in and
**  store in local data structure.
****************************************************************************/

static pct_rec statusBuf[MAX_NODES];

#define MAX_PCT_MSGS     MAX_NODES*2


static void
clear_pct_id(pct_ID *rec)
{
#ifdef STRIPDOWN
    int nodeType;
 
    nodeType = rec->nodeType; /* this field needs to persist */
#endif
    memset(rec, 0, sizeof(pct_ID));

#ifdef STRIPDOWN
    rec->nodeType = nodeType; 
#endif

    rec->job_id     = INVAL;
    rec->session_id = INVAL;
    rec->parent_id  = INVAL;
    rec->job_pid    = INVAL;
    rec->euid       = INVAL;
    rec->final.terminator  = PCT_NO_TERMINATOR;
    rec->pending_jid = INVAL;
    rec->pending_sid = INVAL;
    rec->reserved    = INVAL;
}
static void
init_pct_updates()
{
int i;

    CLEAR_ERR;

    LOCATION("init_pct_updates","top");

    if (srvr_init_control_ptl_at(MAX_PCT_MSGS, update_ptl))
    {
        log_warning("init pct update portal");
        finish_bebopd();
    }

    for (i=0; i<MAX_NODES; i++){
        larray[i].status = STATUS_NO_STATUS;
    }
    minPct = MAX_NODES;
    maxPct = -1;
}
/*
** "kill -s SIGUSR1 bebopd-pid" will display the nid/pid/portal info
** required by pcts.
*/
static void
query_signal(int sig)
{
char *c;

   log_msg("BEBOPD STATE");
   log_msg("bebopd: node %d, system pid %d, portal pid %d",
                 _my_pnid, getpid(), _my_ppid);

   log_msg("bebopd portals: yod requests %d, pingd requests %d, pct updates %d\n",
         REQUEST_PTL, PING_PTL, update_ptl);

   log_msg("PBSsupport: %d, PBSupdate: %d, PBSinteractive: %d\n",
	    PBSsupport, PBSupdate, PBSinteractive);

   c = myLogFileName();

   if (c){
       log_msg("Currently logging Cplant jobs to %s\n",c);
   }
   else {
       log_msg("Problem logging Cplant jobs, can't resolve path name.\n");
   }

   log_msg("location: %s (%s)\n",routine_name,routine_where);

   if (maxPct - minPct > 0){
       log_node_state_info();
   }

   sysv_signal(sig, query_signal);
}
/*
** "kill -s SIGUSR2 bebopd-pid" will toggle extra debugging output
** to the log file.
*/
int debug_toggle=0;

static void 
toggle_debug_signal(int sig)
{
   debug_toggle = (debug_toggle ? 0 : 1);

   print_bufs();

   Dbglevel = (Dbglevel + 1) % 5;
   log_msg("Dbglevel = %d", Dbglevel);

   sysv_signal(sig, toggle_debug_signal);
}

/*
** Spends a few seconds looking at incoming PCT messages.  Returns a
** count of messages.
**
** We need different update counts:
**
**   COUNT_NEW counts only updates from nodes who's previous
**             status was STATUS_UNREPORTED.
**
**   COUNT_ALL counts all updates received.
*/

int
update_pct_pid_array(int countType)
{
int rc, msgtype, xfer_len, pct_nid, pct_pid, nupdates;
control_msg_handle mhandle;
pct_ID *pct_data;
int status;
time_t t1;
#ifdef STRIPDOWN
int nodeType;
#endif

#ifdef RELIABLE
int target_nid;
int target_ppid;
int target_ptl;
#endif

    CLEAR_ERR;

    LOCATION("update_pct_pid_array","top");

    t1 = time(NULL);
    nupdates = 0;

    while ((time(NULL) - t1) < 3){

        SRVR_CLEAR_HANDLE(mhandle);

	rc = srvr_get_next_control_msg(update_ptl, &mhandle, &msgtype,
		      &xfer_len, (char **)&pct_data);

	if (rc == 0){         /* no more pct updates */
	    break;
	}

	if (rc == -1){
	    log_warning("get pct control messages");
	    finish_bebopd();
	}
	pct_nid = SRVR_HANDLE_NID(mhandle);
	pct_pid = SRVR_HANDLE_PID(mhandle);
	status = (int) (pct_data->status);

	if (Dbglevel){
	   log_msg("bebopd: update from %d/%d, status %s, job id %d/%d, status %x\n",
		pct_nid, pct_pid, pct_status_strings[status],
		pct_data->job_id, pct_data->job_pid, pct_data->user_status);
	}
	/*
	** For now, we'll accept any valid looking message from a
	** process that has succeeded in obtaining the PCT's fixed
	** portal pid on a node.
	*/

	if ( (pct_nid >= MAX_NODES) || (pct_nid < 0) ||
	     (pct_pid != PPID_PCT) ||
	     ( (msgtype != PCT_UPDATE_MSG) && (msgtype != PCT_INITIAL_MSG)) ){

	    log_msg("ignoring message %d from %d/%d at pct update portal\n",
		    msgtype, pct_nid, pct_pid);

	    srvr_free_control_msg(update_ptl, &mhandle);

	    continue;
	}

	if ((countType == COUNT_ALL) ||
	    (   (countType == COUNT_NEW) && 
	        (larray[pct_nid].status==STATUS_UNREPORTED))){

	    nupdates++;
	}

	/*
	** Ack a PCT's first message to us.
	*/
	if (msgtype == PCT_INITIAL_MSG){

	    if (Dbglevel){
		log_msg("New pct update, node %d, ack it\n",pct_nid);
	    }

#ifdef RELIABLE
            target_nid  = pct_nid;
            target_ppid = PPID_PCT;
            target_ptl  = PCT_STATUS_ACTION_PORTAL;

            /* use a srvr_send_to_control_ptls() because it is non-blocking */
            rc = srvr_send_to_control_ptls(1, &target_nid, &target_ppid, 
                                           &target_ptl, PCT_GOTCHA, NULL, 0, 0, NULL);
            if (rc < 0) {
                log_error("fatal error in srvr_send_to_control_ptls()\n");
            }

            if (rc != 1) continue;
#else
	    rc = srvr_send_to_control_ptl(pct_nid, PPID_PCT,
		PCT_STATUS_ACTION_PORTAL, PCT_GOTCHA,  NULL, 0);

	    if (rc){
		log_warning("Can not send GOTCHA to pct %d, will retry",pct_nid);

		if (retry_send(pct_nid, PPID_PCT,
			  PCT_STATUS_ACTION_PORTAL, PCT_GOTCHA,  NULL, 0)){

		    log_warning("Retry failed as well, we'll ack it's next one\n");
                }
            }
#endif

	} /* if PCT_INITIAL_MSG */

        /* nodeType is only on bebopd -- preserve it */
#ifdef STRIPDOWN
        nodeType = larray[pct_nid].nodeType; 
#endif
	memcpy((char *)(larray+pct_nid), pct_data, sizeof(pct_ID));

#ifdef STRIPDOWN
        larray[pct_nid].nodeType = nodeType; 
#endif

#ifdef STRIPDOWN
          if ( !Registered[pct_nid] ) {
            if ( larray[pct_nid].nodeType == IACTIVE_NODE ) {
              PBSinteractive =1; /* there are interactive nodes */
              interactive_list[pct_nid] = 1;
#ifdef REMAP
              if ( Remap ) {
                if (marray[pct_nid] > intMAX) {
                  intMAX=marray[pct_nid];
                }
                if ((marray[pct_nid] != -1) &&
                    (marray[pct_nid] < intMIN)) {
                  intMIN=marray[pct_nid];
                }
              }
              else {
#endif
                if (pct_nid > intMAX ) intMAX = pct_nid;
                if (pct_nid < intMIN ) intMIN = pct_nid;
#ifdef REMAP
              }
#endif
            }
            else {
              scheduled_list[pct_nid] = 1;
#ifdef REMAP
              if (Remap) {
                if (marray[pct_nid] > pbsMAX) {
                  pbsMAX = marray[pct_nid];
                }
                if (marray[pct_nid] < pbsMIN) {
                  pbsMIN = marray[pct_nid];
                }
                activeNodes++;
              }
              else {
#endif
                if (pct_nid > pbsMAX ) pbsMAX = pct_nid;
                if (pct_nid < pbsMIN ) pbsMIN = pct_nid;
                activeNodes++;
              }
#ifdef REMAP
            }
#endif
            Registered[pct_nid] = 1;   /* this just ORs the 2 lists for 
                                        convenience */
          }
          else { /* registered nodes -- unregister a DOWN node (but
                    maintain its status->nodeType field) */

            if (status == STATUS_DOWN) {
              Registered[pct_nid] = 0;

              /* if it's a PBS node, decrement pbs active count */
              if ( interactive_list[pct_nid] ) {
                interactive_list[pct_nid] = 0;
              }
              else {
                activeNodes--;
                scheduled_list[pct_nid] = 0;
              }
            }
          } 
#endif
	pct_spids[pct_nid] = pct_pid;

	srvr_free_control_msg(update_ptl, &mhandle);

	minPct = ((pct_nid < minPct) ? pct_nid : minPct);
	maxPct = ((pct_nid > maxPct) ? pct_nid : maxPct);
    }

    return nupdates;
}

/****************************************************************************
**  Setup for ping requests.  Establish well known portal and check
**  it for requests.
****************************************************************************/

#define MAX_PING_MSGS  (50)
#define MAX_PING_DATA  (2)

static int ping_ptl = PING_PTL;

static void
init_ping_requests()
{
    CLEAR_ERR;

    LOCATION("init_ping_requests","top");

    /*
    ** Create portal for ping requests
    */

    if (srvr_init_control_ptl_at(MAX_PING_MSGS, ping_ptl))
    {
        log_warning("init ping request portal");
        finish_bebopd();
    }
}


#ifdef RELIABLE

typedef struct _put_buf {
    struct _put_buf *next;
    char            *buf; 
    int             bufnum;
    time_t          t1;
    int             thresh;
} put_buf;

put_buf *pb_head = NULL;

void
enqueue_put_buf(char *buf, int len, int bufnum, int thresh)
{
    put_buf *new;

    new = malloc(sizeof(put_buf));
    if (new == NULL) 
        log_error("Out of memory in enqueue_put_buf() (trying to alloc %d bytes)", sizeof(put_buf));

    new->buf = malloc(len);
    if (new->buf == NULL) 
        log_error("Out of memory in enqueue_put_buf() (trying to alloc %d bytes)", len);

    memcpy(new->buf, buf, len);

    new->bufnum = bufnum;
    new->t1     = time(NULL);
    new->thresh = thresh;

    /* enqueue at head of list */
    new->next = pb_head;
    pb_head = new;
}

void
prune_put_bufs()
{
    int rc;
    put_buf *curr = pb_head;
    put_buf *prev = NULL;
    put_buf *next;

    while (curr != NULL) {

        rc = srvr_test_read_buf(curr->bufnum, curr->thresh);
        if (rc < 0) log_error("Error in prune_put_bufs(): srvr_test_read_buf returned rc=%d", rc);

        if ((rc == 1) || ((time(NULL) - curr->t1) > clientWaitLimit)) {
            if (rc != 1) log_msg("Freeing put_buf because of timeout");

            /* free put_buf */
            if (prev == NULL) {
                /* removing from head of list */
                pb_head = next = curr->next;
            }
            else {
                /* removing from middle or end of list */
                prev->next = next = curr->next;
            }

            free(curr->buf);
            srvr_delete_buf(curr->bufnum);
            free(curr);

            curr = next;
        }
        else {
            prev = curr;
            curr = curr->next;
        }
    }
}

#endif /* RELIABLE */

static void
answer_ping_requests()
{
int rc, msgtype, xfer_len, i, nid, nid1, nid2;
int elts, status, nnodes, adminRequest, reqtype;
int pingNid, pingPid, pingPtl, bufnum, nreq;
control_msg_handle mhandle;
ping_req *ping_data;
time_t t1;
pingd_summary ps;

#ifdef RELIABLE
int *_nids;
int _ppid, _ptl;
int num_targets;
#endif

    CLEAR_ERR;

    LOCATION("answer_ping_requests","top");

    SRVR_CLEAR_HANDLE(mhandle);

    rc = srvr_get_next_control_msg(ping_ptl, &mhandle, &msgtype,
                  &xfer_len, (char **)&ping_data);
 
    if (rc == 0){         /* no new requests */
        return;
    }
 
    if (rc == -1){
        log_warning("get pingd request messages");
        finish_bebopd();
    }

    if (Dbglevel){
        log_msg(
        "bebopd: got ping request %d..%d, job %d, euid %d, ping ptl %d\n",
        ping_data->args.nid1, ping_data->args.nid2,
        ping_data->args.jobID, ping_data->args.euid, ping_data->pingPtl);
    }
    nid1 = ping_data->args.nid1;
    nid2 = ping_data->args.nid2;

    pingNid = SRVR_HANDLE_NID(mhandle);
    pingPid = SRVR_HANDLE_PID(mhandle);
    pingPtl = ping_data->pingPtl;

    /*
    ** some simple administrative requests
    **
    **   Note that    PBSupdate==TRUE  ->  PBSsupport==TRUE
    **
    **   since PBSsupport is required if updates are to be provided to PBS
    */
    adminRequest = TRUE;

    if (msgtype == BEBOPD_PBS_SUPPORT_ON){
	PBSsupport = TRUE;
    }
    else if (msgtype == BEBOPD_PBS_SUPPORT_OFF){
	PBSsupport = FALSE;
	PBSupdate  = FALSE;
    }
    else if (msgtype == BEBOPD_PBS_UPDATE_ON){
	PBSsupport = TRUE;
	PBSupdate  = TRUE;
    }
    else if (msgtype == BEBOPD_PBS_UPDATE_OFF){
	PBSupdate  = FALSE;
    }
#ifndef STRIPDOWN
    else if (msgtype == BEBOPD_PBS_INTERACTIVE){
	new_interactive_size(nid1);
    }
#endif
    else if (msgtype == BEBOPD_PBS_LIST){

	if (nid1 == -1){             /* get node list from pingd */

	    if (Dbglevel){
		log_msg("Request pingd list of %d PBS interactive nodes\n",nid2); 
	    }
	    rc = srvr_comm_get_req((char *)node_list, nid2*sizeof(int),
		      0, NULL, 0,
		      pingNid, pingPid, pingPtl, 1, clientWaitLimit);

	    if (rc < 0){
		log_warning("can not get interactive node list from ping source\n");
		srvr_free_control_msg(ping_ptl, &mhandle);
		return;
	    }
	    new_interactive_list(node_list, nid2);

	    if (Dbglevel){
		log_msg("pingd requests interactive nodes from list of size %d\n",
		     nid2);
	    }
	}
	else {

/* STRIPDOWN -- support for empty node list */
            if (nid1 == -2){
              nreq = 0;
            }
            else {
	      if (nid1 < 0){
		nid1 = 0;
	      }
	      if (nid2 >= MAX_NODES){
		nid2 = MAX_NODES-1;
	      }
	      nreq = nid2 - nid1 + 1;

	      for (i=0; i<nreq; i++){
	        node_list[i] = nid1 + i;
	      }
            }

	    new_interactive_list(node_list, nreq);

	    if (Dbglevel){
		log_msg("pingd requests interactive nodes to be %d - %d\n",
		     nid1, nid2);
	    }
        }
    }
    else{
	adminRequest = FALSE;
    }

    if (adminRequest){

        if (PBSsupport){
	    solicit_pct_updates(NULL, 0, REQ_FOR_PING, 0, 0, INVAL);
	    compute_active_nodes();
	}
	if (PBSupdate){
	    update_PBS_server();
	}
        srvr_free_control_msg(ping_ptl, &mhandle);
	return;
    }

    if (maxPct < minPct){
	/*
	** We know of no compute nodes out there.
	*/
        if ((msgtype == PCT_STATUS_REQUEST) || 
	    (msgtype == PCT_FAST_STATUS)    ||
	    (msgtype == PCT_SCAN)){

            memset((char *)&ps, 0, sizeof(pingd_summary));

#ifdef RELIABLE
	    srvr_send_to_control_ptls(1, &pingNid, &pingPid, &pingPtl,
				   0, (char *)&ps, sizeof(pingd_summary), 0, NULL);
#else
	    srvr_send_to_control_ptl(pingNid, pingPid, pingPtl,
				   0, (char *)&ps, sizeof(pingd_summary));
#endif
	}
        srvr_free_control_msg(ping_ptl, &mhandle);
	return;
    }

    /*
    ** requests requiring compute partition action
    */
    if (((msgtype != PCT_RESERVE_REQUEST) && 
         (msgtype != PCT_UNRESERVE_REQUEST) &&
         (msgtype != PCT_SCAN) &&
         (msgtype != PCT_DIE_REQUEST) && (msgtype != PCT_STATUS_REQUEST) &&
         (msgtype != PCT_FAST_STATUS) && (msgtype != PCT_RESET_REQUEST)  &&
         (msgtype != PCT_NICE_KILL_JOB_REQUEST) &&
         (msgtype != PCT_GONE_UPDATE) && (msgtype != PCT_INTERRUPT_REQUEST) )  ||

         ( (nid1 == -1) && ((nid2 <= 0)||(nid2 > MAX_NODES)) ) ||

         ( (nid1 != -1) && ( (nid1 < 0)||(nid2 >= MAX_NODES)||(nid2 < nid1)))){

        log_msg("ignoring message type %d (%d %d) at ping request portal\n",
                msgtype, nid1, nid2);

        srvr_free_control_msg(ping_ptl, &mhandle);

        return;
    }

    if (timing_data){
        td1 = dclock();
    }

    LOCATION("answer_ping_requests","get node list");

    /*
    ** Build node list.
    */
    if (nid1 == -1){             /* get node list from pingd */

        nnodes = nid2;

        if (Dbglevel){
            log_msg("Request pingd node_list of size %d\n",nnodes); 
        }
        /*
        ** wait for pingd to send the node list
        */
        rc = srvr_comm_get_req((char *)node_list, nnodes*sizeof(int),
                  0, NULL, 0,
                  pingNid, pingPid, pingPtl, 1, clientWaitLimit);

        if (rc < 0){
            log_warning("can not get node list from ping source\n");
            srvr_free_control_msg(ping_ptl, &mhandle);
            return;
        }
    }
    else { /* node list is a simple range, arrived in control msg */

	if (nid1 < minPct){
	    nid1 = minPct;
	}
        if (nid2 > maxPct){
	    nid2 = maxPct;
	}

        nnodes = nid2 - nid1 + 1;

        if (Dbglevel){
            log_msg("pingd requests %d nodes %d ... %d\n",nnodes,
                 nid1, nid2);
        }
    }
    
    if (timing_data){
        td2 = dclock();
    }
    
    LOCATION("answer_ping_requests","process message type");

    if (msgtype == PCT_GONE_UPDATE){   /* these pcts are dead */
        if (Dbglevel){
            log_msg("bebopd: got a PCT_GONE_UPDATE, %d pcts\n",nnodes);
        }
        for (i=0; i<nnodes; i++){

	    if (nid1 >= 0){
		nid = nid1 + i;
	    }
	    else{
		if (((nid = node_list[i]) < minPct) || (nid > maxPct)){
		    continue;
		}
	    }
            if (larray[nid].status != STATUS_NO_STATUS) {
                clear_pct_id(larray + nid);
                larray[nid].status = STATUS_DOWN;
#ifdef STRIPDOWN
                /* unregister the node -- it will re-register
                   if it comes up again */
                Registered[nid] = 0;
                if ( interactive_list[nid] ) {
                  interactive_list[nid] = 0;
                }
                else {
                  scheduled_list[nid] = 0;
                  activeNodes--;            
                }
#endif
            }
        }
        if (timing_data){
            td4 = td3 = dclock();
        }
    }
    else if ((msgtype == PCT_DIE_REQUEST)   || 
	     (msgtype == PCT_RESET_REQUEST) ||
	     (msgtype == PCT_RESERVE_REQUEST) ||
	     (msgtype == PCT_UNRESERVE_REQUEST) ||
	     (msgtype == PCT_NICE_KILL_JOB_REQUEST) ||
	     (msgtype == PCT_INTERRUPT_REQUEST)    ){

        if (Dbglevel){
            log_msg("bebopd: got a %d request, %d pcts\n", msgtype, nnodes);
        }

#ifdef RELIABLE
        if (nnodes > 0) {
            _nids = malloc(nnodes * sizeof (int));
            if (_nids == NULL) {
                log_warning("out of memory in answer pingd requests\n");
                finish_bebopd();
            }
            _ppid = PPID_PCT;
            _ptl  = PCT_STATUS_ACTION_PORTAL;
        }

        num_targets = 0;
#endif

        for (i=0; i < nnodes; i++){

            if (nid1 >= 0){
                nid = nid1 + i;
            }
            else{
                if (((nid = node_list[i]) < minPct) || (nid > maxPct)){
                    continue;
                }
            }

            if (STATUS_UP(larray[nid].status)){
#ifdef RELIABLE
                if (larray[nid].status != STATUS_UNREPORTED) {
                    _nids[num_targets] = nid;
                    ++num_targets;
                }
#else
                rc = srvr_send_to_control_ptl(nid, PPID_PCT,
                    PCT_STATUS_ACTION_PORTAL, msgtype, 
                    (CHAR *)&(ping_data->args), sizeof(pingPct_req));

                if (rc){
                    log_warning(
		      "Can not send pingd request (%d) to pct %d, will try again",
			     msgtype, nid);

                    if (retry_send(nid, PPID_PCT,
				 PCT_STATUS_ACTION_PORTAL, msgtype,
				(CHAR *)&(ping_data->args), sizeof(pingPct_req))){

			log_warning("Retry failed as well\n");
                    }
                }
#endif
            }
        }

#ifdef RELIABLE
        if (num_targets > 0) {
            rc = srvr_send_to_control_ptls(num_targets, _nids, &_ppid, &_ptl, msgtype,
                                           (char *)&(ping_data->args), sizeof(pingPct_req),
                                            1, NULL);
            if (rc < 0) {
                log_error("Fatal error sending pingd req (%d) to compute nodes", msgtype);
            }
            free(_nids);
        }
#endif

        if (timing_data){
            td4 = td3 = dclock();
        }
    }
    else if ((msgtype == PCT_STATUS_REQUEST) || 
	     (msgtype == PCT_FAST_STATUS)    ||
	     (msgtype == PCT_SCAN)){

        if (Dbglevel){
            log_msg("bebopd: got a STATUS/SCAN request");
        }

	if (msgtype == PCT_SCAN){
	   reqtype = REQ_FOR_SCAN;
	}
	else{
	   reqtype = REQ_FOR_PING;
	}

        if (msgtype != PCT_FAST_STATUS){

            if (nid1 >= 0){
                node_list[0] = nid1;
                node_list[1] = nid2;
		nreq = solicit_pct_updates(node_list, -1, reqtype, 10, 0, INVAL);
            }
	    else{
	        nreq = solicit_pct_updates(node_list, nnodes, reqtype, 10, 0, INVAL);
	    }
            
            if (timing_data){
                td4 = td3 = dclock();
            }
        }      /* end of pct status query */
        else{
            if (timing_data){
                td4 = td3 = td2;
            }
        }
        /*
        ** The status information sent back is for those nodes
        ** requested for which we have received status updates.
        */
        elts = 0;

	memset((char *)&ps, 0, sizeof(pingd_summary));

        for (i=0; i<nnodes; i++){

	    if (nid1 >= 0){
		nid = nid1 + i;
	    }
	    else{
                nid = node_list[i];
	    }
            if ((nid >= minPct) && 
                (nid <= maxPct) &&
                (larray[nid].status != STATUS_NO_STATUS) ){

		statusBuf[elts].nid = nid;
		statusBuf[elts].pid = pct_spids[nid];

                memcpy((void *)&(statusBuf[elts].status), 
		       (void *)&(larray[nid]), sizeof(pct_ID));

                elts++;
            }
        }
	ps.nodes = elts;

        LOCATION("answer_ping_requests","send back pct info");

	if (!PBSsupport){
	    ps.pbsSupportLevel = 0;
#ifndef STRIPDOWN
	    ps.pbsInteractive  = 0;
#endif
        } else {
	    ps.pbsSupportLevel = 1;
#ifndef STRIPDOWN
	    ps.pbsInteractive  = PBSinteractive;
#endif
	    if (PBSupdate){
	        ps.pbsSupportLevel = 2;
	    }
	    ps.pbsOutliers = nodeStateTotals[pbsOutlier];
	    ps.intOutliers = nodeStateTotals[interactiveOutlier];
	}
	ps.reserved = reservedNodes;

        if (Dbglevel){
           log_msg("Reply to ping with %d nodes.\n",elts); 
        }

        if (PBSinteractive) {
          strncpy(ps.ilist, make_interactive_node_string(interactive_list),
                                       SUMMARY_ILIST_SIZE);
          ps.ilist[SUMMARY_ILIST_SIZE-1] = '0';
        }

        bufnum = srvr_comm_put_req((void *)statusBuf, elts*sizeof(pct_rec),
	            0, (char *)&ps, sizeof(pingd_summary),
		    1, &pingNid, &pingPid, &pingPtl);

        if (bufnum < 0) { 
            log_warning("can not send put req to pingd");
            srvr_free_control_msg(ping_ptl, &mhandle);
            return;
        }
        else {
            /* only want to wait for pingd to pickup data if put_req succeeded */
            status = 0;
            t1 = time(NULL);

#ifdef RELIABLE
            enqueue_put_buf((char *)statusBuf, elts*sizeof(pct_rec), bufnum, 1);
            prune_put_bufs();
#else
            while (1) {
                rc = srvr_test_read_buf(bufnum, 1);
     
                if (rc < 0) {
                    log_warning("data buf for pingd data not functioning, sorry\n");
                    finish_bebopd();
                }
                if (rc == 1) {
                    break;
                }
     
                if ((time(NULL) - t1) > clientWaitLimit) {
                    log_msg("Can not wait anymore for pingd to pick up data\n");
                    break;
                }
            }

            if (timing_data) {
                td4 = dclock();
            }
            srvr_delete_buf(bufnum);
#endif
        }

    }

    LOCATION("answer_ping_requests","end");

    rc = srvr_free_control_msg(ping_ptl, &mhandle);

    if (rc){
        log_warning("ping request control message portal (free)");
        finish_bebopd();
    }
    if (timing_data){
        log_msg("\nbebopd pingd request timing\n");
        log_msg("     get node list from ping: %f\n",td2-td1);
        log_msg("     send %d status request(s) to nodes : %f\n",nreq,td3-td2);
        log_msg("     reply to pingd with status data: %f\n",td4-td3);
    }
}

#ifdef DYN_ALLOC
/****************************************************************************
** Setup for dynamic allocation requests. Create a well known portal and check
** it for requests.
*****************************************************************************/
#define MAX_DNA_MSGS (50)
#define MAX_DNA_DATA (2)

static int dna_ptl = DNA_PTL;

static void
init_dna_requests()
{
    CLEAR_ERR;

    LOCATION("init_ping_requests","top");

    /*
    ** Create portal for dynamic node allocation requests
    */


    if (srvr_init_control_ptl_at(MAX_DNA_MSGS, dna_ptl))
    {
	log_warning("init dynamic node allocation request portal");
        finish_bebopd();
    }
else printf("dynamic node allocation request portal created.\n");
}

static void
answer_dna_requests()
{
int rc, msgtype, xfer_len, mle, nid1, nid2;
int replyNid, replyPid, replyPtl, jobID;
int count, nnodes, i, send_val;
time_t t1;

char * res;

control_msg_handle mhandle;
ping_req *dna_data;
int *dna_info;


    CLEAR_ERR;

    LOCATION("answer_dna_requests","top");

    SRVR_CLEAR_HANDLE(mhandle);

    rc = srvr_get_next_control_msg(dna_ptl, &mhandle, &msgtype,
		  &xfer_len, (char**)&dna_data);

    if (rc == 0){	/* no new requests */ 
	return;
    }

    if (rc == -1) {
       log_warning("get dynamic node allocation request message");
       finish_bebopd();
    }

    if (Dbglevel){
	log_msg(
	"bebopd : got dynamic node request %d..%d, job %d, euid %d, dna ptl %d\n",
	dna_data->args.nid1, dna_data->args.nid2, dna_data->args.jobID,
	dna_data->args.euid, dna_data->pingPtl);

     }

     jobID = dna_data->args.jobID;

     replyNid = SRVR_HANDLE_NID(mhandle);
     replyPid = SRVR_HANDLE_PID(mhandle);
     replyPtl = dna_data->pingPtl;

     if (msgtype == BEBOPD_INFO_GET_JOB_SIZE) {

	jobID = dna_data->args.jobID;

	replyPtl = dna_data->pingPtl;

	nid1 = minPct;
	nid2 = maxPct;

	count = 0;

	for (i = nid1; i < nid2; i++) {
	    if (larray[i].job_id == jobID) {
		
		if (larray[i].euid != dna_data->args.euid) {
		    count = -1;
		    break;
		}
		count++;
	    }
	}

        srvr_send_to_control_ptl(replyNid, replyPid, replyPtl, 1, 
			(char *) &count, sizeof(int));

     }

     if (msgtype == BEBOPD_JOB_GET_PBS_ID) {

        jobID = dna_data->args.jobID;

        replyPtl = dna_data->pingPtl;

        nid1 = minPct;
        nid2 = maxPct;

        nnodes = nid2-nid1 +1;

        for (i=0; i < nnodes && larray[i].job_id != jobID; i++);

        if ( i < nnodes &&
             larray[i].job_id == jobID &&
             larray[i].euid == dna_data->args.euid) {

            send_val=larray[i].session_id;
        }
        else {
            send_val = -1;
        }

        srvr_send_to_control_ptl(replyNid, replyPid, replyPtl, 1,
                        (char *) &send_val, sizeof(int));
    }

    if (msgtype == BEBOPD_DO_QSTAT) {

        res = NULL;
        replyPtl=dna_info[0];
        rc = query_pbs(&res, 15, "qstat -a");
        if (rc) {
            mle=srvr_comm_put_req(res, rc, 1,NULL, 0, 1, &replyNid, &replyPid,
                                  &replyPtl);
            t1 = time(NULL);

            LOCATION("answer_dyn_alloc_req", "wait on put qstat info to yod");

            while (1){
                rc = srvr_test_read_buf(mle, 1);

                if (rc < 0){
                    log_msg("data buf for dyn. alloc data not functioning.\n");
                    finish_bebopd();
                }
                if (rc == 1) break;

                if ((time(NULL) - t1) > clientWaitLimit){
                    log_msg("Can not wait anymore for pct to pick up data\n");
                    break;
                }
            }
        }
        else {
            srvr_send_to_control_ptl(replyNid, replyPid, replyPtl,
                REPLY_NO_DATA, NULL, 0);
        }
    }

    if (msgtype == BEBOPD_LIST_QUEUES) {

        res = NULL;
        replyPtl=dna_info[0];
        rc = query_pbs(&res,15,"qstat -q");
        if (rc) {
            mle=srvr_comm_put_req(res, rc, rc,NULL, 0, 1, &replyNid, &replyPid,
                                  &replyPtl);
            t1 = time(NULL);

            LOCATION("answer_dyn_alloc_req", "wait on put qstat info to yod");
            while (1){
                rc = srvr_test_read_buf(mle, 1);

                if (rc < 0){
                    log_msg("data buf for dyn. alloc qstat not functioning.\n");
                    finish_bebopd();
                }
                if (rc == 1) break;

                if ((time(NULL) - t1) > clientWaitLimit){
                    log_msg("Can not wait anymore for pct to pick up data\n");
                    break;
                }
            }
        }
        else {
            srvr_send_to_control_ptl(replyNid, replyPid, replyPtl,
                REPLY_NO_DATA, NULL, 0);
        }
    }

    if (msgtype == BEBOPD_QUERY_SERVER) {
        res = NULL;
        replyPtl=dna_info[0];
        rc = query_pbs(&res,15,"qmgr -c \"l s\"");
        if (rc) {
            mle=srvr_comm_put_req(res, rc, rc,NULL, 0, 1, &replyNid, &replyPid,
                                  &replyPtl);
            t1 = time(NULL);

            LOCATION("answer_dyn_alloc_req", "wait on put qstat info to yod");
            while (1){
                rc = srvr_test_read_buf(mle, 1);

                if (rc < 0){
                    log_msg("data buf for dyn. alloc qstat not functioning.\n");
                    finish_bebopd();
                }
                if (rc == 1) break;

                if ((time(NULL) - t1) > clientWaitLimit){
                    log_msg("Can not wait anymore for pct to pick up data\n");
                    break;
                }
            }
        }
        else {
            srvr_send_to_control_ptl(replyNid, replyPid, replyPtl,
                REPLY_NO_DATA, NULL, 0);
        }
    }


    if (msgtype == BEBOPD_QUERY_QMGR_Q) {

        res = NULL;
        replyPtl=dna_info[0];
        rc = qmgr_q_query(&res, 3000, (char*)(dna_info+2));
        mle=srvr_comm_put_req(res, rc, 1,NULL, 0, 1, &replyNid, &replyPid,
                                  &replyPtl);

        t1 = time(NULL);

        LOCATION("answer_dyn_alloc_req", "wait on put q info to yod");

        while (1){
            rc = srvr_test_read_buf(mle, 1);

            if (rc < 0){
                log_msg("data buf for dyn. alloc data not functioning, sorry\n");
                finish_bebopd();
            }
            if (rc == 1) break;

            if ((time(NULL) - t1) > clientWaitLimit){
                log_msg("Can not wait anymore for pct to pick up data\n");
                break;
            }
        }
    }

     rc = srvr_free_control_msg(dna_ptl, &mhandle);

     if (rc){
	log_warning("dynamic node request control message portal (free)");
        finish_bebopd();
     }
}
#endif /* DYN_ALLOC */



/****************************************************************************
**  Setup for local yod requests.  Establish well known portal and check
**  it for requests.
**
****************************************************************************/

#define MAX_YOD_MSGS  (50)
#define MAX_YOD_DATA    1

static int request_ptl = REQUEST_PTL;

static void
init_yod_requests()
{
    CLEAR_ERR;

    /*
    ** Create portal for yod requests
    */
    LOCATION("init_yod_requests", "top");

    if (srvr_init_control_ptl_at(MAX_YOD_MSGS, request_ptl))
    {
        log_warning("init yod request portal");
        finish_bebopd();
    }

}


static void
display_yod_request(control_msg_handle mhandle, yod_request *yod_data,
                     int msgtype)
{
nodeSpec *spec;

    spec = &(yod_data->spec);

    log_msg("From yod %d/%d, msg type %s\n",
        SRVR_HANDLE_NID(mhandle), SRVR_HANDLE_PID(mhandle),
        (msgtype==YOD_NODE_REQ_ANY?"YOD_NODE_REQ_ANY":
          (msgtype==YOD_NODE_REQ_RANGE?"YOD_NODE_REQ_RANGE":
             (msgtype==YOD_NODE_REQ_LIST?"YOD_NODE_REQ_LIST":
                 (msgtype==YOD_NODE_REQ_COMPOUND?"YOD_NODE_REQ_COMPOUND":
                  "UNKNOWN"))))        );

    log_msg("%d nodes, my ptl %d\n",yod_data->nnodes, yod_data->myptl);
    if (msgtype == YOD_NODE_REQ_RANGE){
        log_msg("range: %d - %d\n",spec->range.from_node,
                                  spec->range.to_node);
    }
    else if (msgtype == YOD_NODE_REQ_COMPOUND){
        log_msg("compound request composed of %d requests\n",
                   spec->req.numRequests);
    }
    else if (msgtype == YOD_NODE_REQ_LIST){
        log_msg("node list of %d entries\n", spec->list.listsize);
    }
    return;
}
int 
check_yod_request(yod_request *yod_data)
{
int status, msgtype;
nodeSpec *spec;

    msgtype = yod_data->specType;

    if (((msgtype != YOD_NODE_REQ_ANY)    &&
         (msgtype != YOD_NODE_REQ_RANGE)  &&
         (msgtype != YOD_NODE_REQ_COMPOUND)  &&
         (msgtype != YOD_NODE_REQ_LIST)      ) ||
        (yod_data->nnodes <= 0)          ||
        (yod_data->nnodes > MAX_NODES )  ||
        (yod_data->myptl > SRVR_MAX_PORTAL)     ){

        log_msg("ignoring message type %x (%d %d) at yod request portal\n",
                msgtype, yod_data->nnodes, yod_data->myptl);

        return -1;
    }
    status = 0;

    spec = &(yod_data->spec);

    if (msgtype == YOD_NODE_REQ_RANGE){
        if ( (spec->range.from_node < 0) || 
             (spec->range.from_node > MAX_NODES) || 
             (spec->range.to_node < 0) || 
             (spec->range.to_node > MAX_NODES) ){

             log_msg("ignoring YOD_NODE_REQ_RANGE with range %d - %d\n",
                    spec->range.from_node,
                    spec->range.to_node);

             status = -1;
         }
    }
    else if (msgtype == YOD_NODE_REQ_LIST){

        if ((spec->list.listsize <=0) || (spec->list.listsize > MAX_NODES)){

             log_msg("ignoring YOD_NODE_REQ_LIST with list size %d\n",
                    spec->list.listsize);

             status = -1;
        }
    }
    else if (msgtype == YOD_NODE_REQ_COMPOUND){
        if (spec->req.numRequests < 2 ){

             log_msg("ignoring YOD_NODE_REQ_COMPOUND with %d requests\n",
                    spec->req.numRequests);

             status = -1;
        }
    }
    return status;
}
static char log_record[USERLOG_MAX];

static void
log_user(control_msg_handle *mhandle, yod_request *yod_data)
{
int yodnid;
int yodpid, reclen, rc;
int yodptl;
bebopd_status status;

    yodnid = SRVR_HANDLE_NID(*mhandle);
    yodpid = SRVR_HANDLE_PID(*mhandle);

    yodptl = yod_data->myptl;
    reclen = yod_data->nnodes;

    status.rc = BEBOPD_OK;   /* bebopd gets are initiated by a yod */
	     /* request.  rc to let yod know we can do the get request */

    /*
    ** Send a request to yod for the log record
    */
    rc = srvr_comm_get_req(log_record, 
	     (reclen < USERLOG_MAX) ? reclen : USERLOG_MAX,
	     BEBOPD_GET_USERLOG_RECORD, 
	     (char *)&status, sizeof(bebopd_status),
	     yodnid, yodpid, yodptl,
	     1, clientWaitLimit);

    if (rc < 0){
        log_warning("log_user - srvr_comm_get_req failure");
        return; 
    }

    write_log_record(log_record);  
}
static void  
answer_yod_requests()
{
int rc, msgtype, xfer_len, mle, jobID, reqnodes;
int i, fail_load;
control_msg_handle mhandle;
yod_request *yod_data;
int *pctList;
int yodnid;
int yodpid;
int yodptl;
time_t t1;
bebopd_status status;

    CLEAR_ERR;

    LOCATION("answer_yod_requests", "top");

    if (timing_data)
        td1 = dclock();

    SRVR_CLEAR_HANDLE(mhandle);

    rc = srvr_get_next_control_msg(request_ptl, &mhandle, &msgtype,
                  &xfer_len, (char **)&yod_data);
 
    if (rc == 0){         /* no new yod requests */
        return;
    }
 
    if (rc == -1){
        log_warning("get yod request messages");
        finish_bebopd();
    }

    if (msgtype == YOD_USERLOG_RECORD){
	log_user(&mhandle, yod_data);
        srvr_free_control_msg(request_ptl, &mhandle);
        return;
    }

    if (Dbglevel){
        display_yod_request(mhandle, yod_data, msgtype);
    }

    rc = check_yod_request(yod_data);
 
    if (rc < 0){
        srvr_free_control_msg(request_ptl, &mhandle);
        return;
    }
    reqnodes = yod_data->nnodes;
    yodnid = SRVR_HANDLE_NID(mhandle);
    yodpid = SRVR_HANDLE_PID(mhandle);
    yodptl = yod_data->myptl;

    if (maxPct < minPct){
	/*
	** We're not aware of any compute nodes out there.
	*/
	status.job_id = -1;
	status.rc     = BEBOPD_ERR_FREE_NODES;

#ifdef RELIABLE
	srvr_send_to_control_ptls(1, &yodnid, &yodpid, &yodptl,
		    status.rc, (char *)&status, sizeof(bebopd_status), 0, NULL);
#else
	srvr_send_to_control_ptl(yodnid, yodpid, yodptl,
		status.rc, (char *)&status, sizeof(bebopd_status));
#endif

        srvr_free_control_msg(request_ptl, &mhandle);
        return;
    }
    td1 = dclock();

    jobID = ((prevJobID > LASTJOBID) ? FIRSTJOBID : prevJobID+1);

    /* adding this to ensure that bebopd is updated on statuses
       of pcts corresponding to the previous job run in a pbs
       session */
    update_pct_pid_array(COUNT_ALL);
    rc = find_free_nodes(yod_data, &mhandle, &pctList, jobID);

    srvr_free_control_msg(request_ptl, &mhandle); 

    if (rc != BEBOPD_OK){

        status.job_id = -1;
        status.rc     = rc;

        if (rc == BEBOPD_ERR_INTERNAL){
	    log_msg("Failure to process yod request from %d/%d",
		      yodnid,yodpid);
	}
	if (Dbglevel){
	    log_msg("Send allocation failure to yod, status %d\n",status.rc);
	}

#ifdef RELIABLE
	srvr_send_to_control_ptls(1, &yodnid, &yodpid, &yodptl,
		    status.rc, (char *)&status, sizeof(bebopd_status), 0, NULL);
#else
	srvr_send_to_control_ptl(yodnid, yodpid, yodptl,
		    status.rc, (char *)&status, sizeof(bebopd_status));
#endif

        return;
    }

#ifdef STRIPDOWN   
/* get pct updates from nodes in allocation list 
   these nodes allocate themselves to the job and start a 
   timeout on getting the INIT_LOAD from yod */
    rc = solicit_pct_updates(pctList, reqnodes, REQ_FOR_YOD, 0, jobID,
                                   yod_data->session_id);
    fail_load = 0;
    if (rc != reqnodes ) {
      /* allocation failed -- we've got some STATUS_UNREPORTED (stale)
         nodes now */
      status.job_id = -1;
      status.rc = BEBOPD_ERR_CONTACT_NODES;
      fail_load = 1;
    }
    else { /* all nodes reported back, but it's possible that some
              might have unexpected status... */
      for (i=0; i<reqnodes; i++) {
        if ( larray[pctList[i]].status != STATUS_FREE ) { 
          /* fail load -- but continue marking nodes as allocated, 
             since that's what each pct does -- when pcts time out on
             allocation for this job, we should get updated w/ free
             status */
          fail_load = 1;
          status.job_id = -1;
          status.rc = BEBOPD_ERR_UNEXPECTED_NODE_STATE;
        }
        else {
          larray[pctList[i]].status = STATUS_ALLOCATED;
        }
      }
    }
    if (fail_load) { /* report failure to yod */

#ifdef RELIABLE
      srvr_send_to_control_ptls(1, &yodnid, &yodpid, &yodptl, status.rc,
                   (char *)&status, sizeof(bebopd_status), 0, NULL);
#else
      srvr_send_to_control_ptl(yodnid, yodpid, yodptl, status.rc,
                               (char *)&status, sizeof(bebopd_status));
#endif
      return;
    }
#endif

    td2 = dclock();

    prevJobID = jobID;

    status.rc     = BEBOPD_OK;
    status.job_id = jobID;

    /*
    ** send allocated PCTs to yod
    */

    if (Dbglevel){
        log_msg("Send pct list of length %d to yod, job ID %d\n",
                     reqnodes, jobID);
    }

    mle = srvr_comm_put_req((void *)pctList, reqnodes*sizeof(int),
                           BEBOPD_PUT_PCT_LIST,
	                    (char*)&status, sizeof(bebopd_status),
                           1, &yodnid, &yodpid, &yodptl);

    if (mle < 0){
        log_warning("error in send pct list to yod %d/%d/%d",
            SRVR_HANDLE_NID(mhandle),
            SRVR_HANDLE_PID(mhandle),
            yod_data->myptl);
    }
    else {
        /* only want wait for yod to pickup data if put_req succeeded */
        t1 = time(NULL);

//#ifdef RELIABLE
#ifdef RELIABLE_TODO
        enqueue_put_buf((char *)pctList, reqnodes*sizeof(int), mle, 1);
        prune_put_bufs();
#else
        LOCATION("answer_yod_requests", "wait on put pct list to yod");

        while (1){
            rc = srvr_test_read_buf(mle, 1);
 
            if (rc < 0){
                log_msg("data buf for yod data not functioning, sorry\n");
                finish_bebopd();
            }
            if (rc == 1) break;
 
            if ((time(NULL) - t1) > clientWaitLimit){
                log_msg("Can not wait anymore for yod to pick up data\n");
                break;
            }
        }
        td4 = dclock();

        LOCATION("answer_yod_requests", "done");
    
        rc = srvr_delete_buf(mle);
        if (rc) {
            log_warning("fatal error cleaning up put buffers\n");
            finish_bebopd();
        }
#endif

    }


    if (timing_data){
        td4 = dclock();
    }
    if (timing_data){
        log_msg("\nbebopd node allocation timing (%d nodes):\n",reqnodes);
	log_msg("     queried %d nodes for status, updated %d to deallocate\n",
			   timing_queries, timing_updates);
        log_msg("     find free pcts: %f\n",td2-td1);
        log_msg("     reply to yod with node list: %f\n",td4-td2);
    }
}

static void
write_out_pct_data()
{
FILE *savfd;
int i, logged;
const char *fname;
pct_rec rrec;

    LOCATION("write_out_pct_data", "top");

    logged = 0;

    fname = bebopd_restart_file();

    if (!fname){
        return;
    }

    savfd = fopen(fname, "w");

    if (!savfd){
        log_warning("can not open %s to log active pcts",fname);
        return;
    }
    for (i=minPct; i<=maxPct; i++){
        if (larray[i].status != STATUS_NO_STATUS){
	    rrec.nid = i;
	    rrec.pid = pct_spids[i];
	    memcpy((void *)&(rrec.status), (void *)(larray+i), sizeof(pct_ID));

            fwrite((void *)&rrec, sizeof(pct_rec), 1, savfd);
            if (!logged){
                log_msg("pct data saved to %s\n",fname);
                logged=1;
            }
        }
    }
    fclose(savfd);

    return;
}
static void
restart_bebopd()
{
const char *fname;
FILE *savfd;
int rc, i, fsize, npcts, maxJobID;
pct_rec *savedPcts;
struct stat statbuf;

    LOCATION("restart_bebopd", "top");

    fsize = -1;

    if (RestartFile[0]){
        fname = RestartFile;
    }
    else{
        fname = bebopd_restart_file();
    }

    if (fname){
        rc = stat(fname, &statbuf);

        if (rc == 0){
            fsize = statbuf.st_size;
        }
    }

    if (fsize == 0){
        log_msg("Restart file has no records.\n");
        return;
    }

    if (fsize < 0){
        log_msg("Unable to locate active pct restart file\n");
        return;
    }

    savedPcts = (pct_rec *)malloc(fsize);

    if (!savedPcts){
        log_error("malloc space for savedPcts array");
    }

    savfd = fopen(fname, "r");

    if (!savfd){
        log_warning("Unable to open active pct restart file %s\n",fname);
        return;
    }

    rc = fread((void *)savedPcts, fsize, 1, savfd);

    if (rc != 1){
        log_warning("Unable to read active pct restart file %s\n",fname);
        return;
    }
    rc = fclose(savfd);

    npcts = fsize / sizeof(pct_rec);

    for (i=0, maxJobID=0; i<npcts; i++){

        node_list[i] = savedPcts[i].nid;

        if ((savedPcts[i].status.job_id >= FIRSTJOBID) &&
            (savedPcts[i].status.job_id <= LASTJOBID)  &&
            (savedPcts[i].status.job_id > maxJobID)          ){

            maxJobID = savedPcts[i].status.job_id;
        }

	minPct = ((minPct > node_list[i]) ? node_list[i] : minPct);
	maxPct = ((maxPct < node_list[i]) ? node_list[i] : maxPct);

	larray[node_list[i]].status = STATUS_FREE;

    }

    free(savedPcts);

    solicit_pct_updates(node_list, npcts, REQ_FOR_BEBOPD, 0, 0, INVAL);
    
    /*
    ** Select a starting job ID number that is greater than the job ID
    ** of any job that was executing when the bebopd last exited.
    */
    if (maxJobID){
        prevJobID = maxJobID+1;
    }
    else{
        prevJobID = FIRSTJOBID;
    }
}
static void
takedown_portals()
{
    LOCATION("takedown_portals", "top");

    if (request_ptl != SRVR_INVAL_PTL){
        srvr_release_control_ptl(request_ptl);
        request_ptl = SRVR_INVAL_PTL;
    }
    if (update_ptl != SRVR_INVAL_PTL){
        srvr_release_control_ptl(update_ptl);
        update_ptl = SRVR_INVAL_PTL;
    }
    if (ping_ptl != SRVR_INVAL_PTL){
        srvr_release_control_ptl(ping_ptl);
        ping_ptl = SRVR_INVAL_PTL;
    }
}
/*
** Currently, a send may fail because MCP can't handle the volume
** of sends.  We'll wait 10 seconds and try again.
*/
int
retry_send(INT32 nid, INT32 pid, int idx, INT32 tag,
	     CHAR *udata, INT32 len)
{
int rc;

   sleep(10);

   rc = srvr_send_to_control_ptl(nid, pid, idx, tag, udata, len);

   return rc;
}
