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
** $Id: pct_app_host.c,v 1.76.2.1 2002/05/16 18:44:31 rklundt Exp $
*/
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <unistd.h>
#include <asm/unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <malloc.h>
#include <ctype.h>
#include <grp.h>
#include "pct.h"
#include "srvr_err.h"
#include "proc_id.h"
#include "config.h"
#include "appload_msg.h"
#include "ppid.h"
#include <sched.h>
#include "pct_ports.h"
#include "bt.h"
#include "cgdb.h"
#include "fyod_map.h"
#include "portal_assignments.h"
#ifdef KERNEL_ADDR_CACHE
#include "cache/cache.h"
#endif

extern int fyod_map[FYOD_MAP_SZ];
extern int cache_fd;

/* process id for gdb wrapper */
extern int gwrap_pid;
extern int gwrap_status;

extern int isblank(int c);

static int fndnm(char *c);
static server_id bebopd_id;

/*******************************************************************************
      The component of PCT that hosts the app process (APP_HOST)

        function:
                sets up and starts app process
                provides data to app process in pre main startup
                processes the termination of the app process, cleans up
                collects a debugging stack trace for the main pct if requested
                   by the yod user
                can attach a debugger to the app process

*******************************************************************************/

/********************************************************************************
** Cleaning up after application process:  
**
**    After the fork that creates the application child process, set its
**       process group ID to it's process ID.  Now the application process
**       and all it's children will have this process group ID.  A signal
**       can be sent to the application process and all it's children
**       with killpg().
**
**    Application child process terminates:  If any of it's children 
**       terminated before it, their zombies are cleared out when the app
**       process terminates.  If any of it's children are still out
**       there, they can all be terminated with one call to killpg().
**       No need to wait() to clear zombies, since their parent terminated
**       first no zombie is created on their termination.
**
**    Application child process terminated by PCT:  Send the specified signal
**       to the application child process only.  It may want to handle the
**       signal.  Only when we get to sending SIGKILL do we send it to the
**       entire process group.  Then wait() for the application child
**       process.  No need to wait for subprocesses since their parent processes
**       are gone.  But, at this point (when the application child process
**       has been sucessfully wait()'ed for) kill all processes in it's
**       process group ID in the event it left any out there.
**
**    PCT is terminating: Must kill application child process and its entire
**       process ID group.  No need to wait, any zombies are cleared when the
**       PCT terminates.
********************************************************************************/

static int start_proxy(void);

static int init_pct_app_host(void);
static void clear_state(void);

static void listen_app_process(void);

static void wait_for_app(void);
static void child_done(int wstatus);

static int start_user_process(int *appPid, int *user_status);
static void exec_with_strace(void);
static int unpack_strings(char *packed, char **unpacked, int maxu);

static void main_pct_request(int todo, char *buf, int len);

/*
** get these from main PCT
*/
static proc_list procData;
static int dbglevel, debugSpecial;
static int peerMap[MAX_NODES];
static int peerRank, numPeers;

/*
** my state
*/
static FILE *DbgIn = NULL;
static appData appStatus;
static int appHistory;
static control_msg_handle app_pid_req;

static int app_ctl_portal    = SRVR_INVAL_PTL;
static int child_go_main_ptl = SRVR_INVAL_PTL;

static void close_gdb(void);

/* for interactive debugging w/ gdb */
static int wait_bt_put_msg(int bufnum, int tmout, int howmany);
static int wait_cgdb_put_msg(int bufnum, int tmout, int howmany);
static int gdb_interactive(char** bt, int* btsize);

/* for bt w/ gdb */
static int get_bt;

/* this limits the size (in chars) of a line of gdb output */
#define GDB_LN_SIZE 1024*4

#define GDB_TOTAL_OUT_SIZE 1024*64
static char cout[GDB_TOTAL_OUT_SIZE];

static int getch(int fd);
static int gdb_pipes(void);
static void itoa(int n, char s[]);
static int readgdb(int fd, int* pos, char line[], int sz);

static void check_gdb_output(void);
static void get_gdb_prompt(void);

static char pid_str[21];
static int gdb_pos=0, gdb_result=0;
static char* argv_gdb[6];
static char* argv_gdbwrap[6];
static const char* gdb_binary = "/usr/bin/gdb";
static const char* gdbwrap_binary = "/cplant/sbin/wrap";

#if 0
/* don't send gdb output to yod
   if this is set
*/
static int got_abort=0;
#endif

/* wait this long before timing out on
   getting an initial gdb prompt
*/
static double gdb_timeout=30.0;                /* in seconds */
static double gt0;

/* holds a line of output from gdb */
static char line[GDB_LN_SIZE];

/* pipes for redirecting the debugger's io */
static int fd_w_gdb[2], fd_r_gdb[2];

/* process id for debugger (i.e. gdb) */
static int gpid=0;

/* gdb commands and their lengths */
static const char* gdbcmd[4] = { "cont\n", "where\n", "cont\n", "quit\n" };
static const int gdbcmdlen[4] = { 5, 6, 5, 5 };

/* gdb state vars */
static int app_faulted=0, bt_w_gdb=0, gdbcmd_ind=0, save_gdb_output=0; 
static int gdb_count=0;
enum{ CONT1, BACKTRACE, CONT2, QUIT };
enum{ GDB_ERR, GDB_PROMPT, GDB_PROG_RUNNING, RECEIVED_SIGNAL, GDB_VERSION, COPYRIGHT, CURR_LANG, INCOMPLETE_LN, GDB_OTHER };


appData *
go_app_host(mgrData *mdata, char *buf, int len)
{
int rc;

    appStatus.attn   = 0;  /* notifies main PCT something of interest happened */
    appStatus.u_stat = 0;  /* child status events since last call */
    appStatus.rc     = APP_HOST_OK;
    CPerrno = 0;

    LOCATION("go_app_host", "top");

    if (app_ctl_portal == SRVR_INVAL_PTL){  /* one time initialization */
        clear_state();

        rc = init_pct_app_host();

        if (rc != APP_HOST_OK){
            appStatus.rc = rc;
            return &appStatus;
        }
        dbglevel = mdata->dbglevel;
        debugSpecial = mdata->debugSpecial;
    }

    /*
    ** If app process has started, check for it's termination.
    */
    if (appStatus.started && !appStatus.done && !bt_w_gdb){

        wait_for_app();

        if (appStatus.rc != APP_HOST_OK){
            return &appStatus;
        }
    }
    
    /*
    ** execute main PCT's request, if any
    */

    if (mdata->todo != TODO_NULL){
        main_pct_request(mdata->todo, buf, len);

        if (appStatus.rc != APP_HOST_OK){
            return &appStatus;
        }
    }

    /*
    ** check for app process messages
    */
    if (appStatus.started && !appStatus.done){

        listen_app_process();

        if (appStatus.rc != APP_HOST_OK){
            return &appStatus;
        }

    }


    /*
    ** Debugging the app?  
    **    1. Check for debugger output.  
    **
    **    Interactive?  
    **        2. Check for user debugging requests.  
    **        3. If debugger command, fan out request and execute it.  If 
    **           display request, fan out request and send debugger output 
    **           to user.
    **
    **    Non interactive?
    **        2. If debugger indicates fault, collect and save a back trace.
    **           The main PCT may request it on behalf of yod.
    */
    if ( bt_w_gdb ) {
      check_gdb_output();
    }
    /*
    ** Check once more for app termination, maybe main PCT had
    ** asked us to kill it.
    */
    if (appStatus.started && !appStatus.done && !bt_w_gdb){

        wait_for_app();

        if (appStatus.rc != APP_HOST_OK){
            return &appStatus;
        }
    }

    /*
    ** Reply to main PCT with any state changes
    */
    if ((Dbglevel > 0) && appStatus.u_stat){
        log_msg("go_app_host: returning appStatus 0x%04x\n",appStatus.u_stat);
    }

    return &appStatus;
}
/***********************************************************************
**  Process the main PCT's requests:
**
**   TODO_NULL      no action required on behalf of main PCT
**   TODO_START     start app, buf contains pointer to process list entry
**   TODO_GETPIDS   buf contains user application pid list
**   TODO_GOMAIN    all app processes are synchronized, send app to main
**   TODO_SIGUSR    send signal to app process
**   TODO_ABORT     abort app process
**   TODO_RESET     things are a mess, reset everything even if there's an
**                      app process running
**   TODO_ALLDONE   all app processes have terminated, you're done
**
***********************************************************************/
static void
main_pct_request(int todo, char *buf, int len)
{
int rc;

  LOCATION("main_pct_request", "top");
  appStatus.rc = APP_HOST_OK;

  switch(todo){

    case TODO_NULL:     /* main pct has nothing for us to do */
      break;

    case TODO_START:    /* main pct says start app process */

      LOCATION("main_pct_request", "TODO_START");

      if (appStatus.started){
        log_msg("go_app_host: unexpected start request");
        appStatus.rc = LAUNCH_ERR_HOST_PROTOCOL;
        break;
      }
      if (dbglevel >= 1){
        log_msg("go_app_host: received request to start app");
      }
      appStatus.u_stat = NEW_JOB;

      if (len < (int)sizeof(proc_list)){
        log_msg("go_app_host: invalid length of start data");
        appStatus.rc = LAUNCH_ERR_HOST_PROTOCOL;
        break;
      }
      memcpy((char *)&procData, buf, sizeof(proc_list));

      /*
      ** If go_app_host is a subprocess, then we need to
      ** pull in args, environment and executable name here.
      */

#if 0
      /* clear this guy in case set previously */
      got_abort=0;
#endif

      rc = start_user_process(&appStatus.pid, &appStatus.u_stat);

      if (rc){
        appStatus.rc = rc;
      }
      else{
        appStatus.attn    = 1;
        appStatus.started = 1;
        appStatus.start_time = time(NULL);  /* more or less */

        if (dbglevel >= 1){
          log_msg("go_app_host: app process has started");
        }
      }

      /* in case there's a left over gdb wrapper... */

      if (gwrap_pid > 0) {
        log_msg("OPT_ATTACH: sending gdb proxy SIGTERM...");
        kill(gwrap_pid, SIGTERM);

        td0 = dclock();

	while((dclock() - td0) < 30.0){
            rc = waitpid(gwrap_pid, &gwrap_status, WNOHANG);
	    if (rc == 0) continue;
	    break;
	}

        gwrap_pid = 0;
      }

      break;

    case TODO_GETPEERS:  /* main pct has nid map of other app hosts */
      LOCATION("main_pct_request", "TODO_GETPEERS");

      if (len > (int)(sizeof(int) * MAX_NODES)){
        log_msg("go_app_host: invalid length of peer list");
        appStatus.rc = LAUNCH_ERR_HOST_PROTOCOL;
      }
      else{
        memcpy((char *)peerMap, buf, len);
        peerRank = procData.my_rank;
        numPeers = procData.nprocs;

        if (dbglevel >= 1){
          log_msg("go_app_host: got my list of peers");
        }
      }
      break;
    /*
    ** Main PCT has gathered map of portals IDs required by application
    */
    case TODO_GETPIDS:       /* main PCT says it has app global pid map */
            
      LOCATION("main_pct_request", "TODO_GETPIDS");
      if (!appStatus.started || (appHistory & SENT_PIDMAP)){
        log_msg("go_app_host: unexpected pid list");
        appStatus.rc = LAUNCH_ERR_HOST_PROTOCOL;
        break;
      }
      if (len != (int)(sizeof(ppid_type) * procData.nprocs)){
        log_msg("go_app_host: invalid length of pid list");
        appStatus.rc = LAUNCH_ERR_HOST_PROTOCOL;
        break;
      }
      memcpy((char *)procData.IDmap, buf, len);

      if (dbglevel >= 1){
        log_msg("go_app_host: got application pid map");
      }

      if (appHistory & REQUESTED_PIDMAP){

        rc = srvr_comm_get_reply(&(app_pid_req), (char *)procData.IDmap, len);

        if (rc){
          appStatus.rc = LAUNCH_ERR_CHILD_COMM;
        }
        else{
          appStatus.u_stat |= SENT_PIDMAP;
          appStatus.attn = 1;

          if (dbglevel >= 1){
            log_msg("go_app_host: sent pid map to app process");
          }
        }
	free_app_message(&(app_pid_req));
      }

      break;

    case TODO_GOMAIN:
      LOCATION("main_pct_request", "TODO_GOMAIN");

      get_bt = 0;

      if (!(appHistory & GOT_READY)){
        log_msg("go_app_host: go main before app is ready");
        appStatus.rc = LAUNCH_ERR_HOST_PROTOCOL;
        break;
      }

      if (dbglevel >= 1){
        log_msg("go_app_host: sending app process to main");
      }


      /* application debugging  -- attach gdb to app in case
         either noninteractive backtrace or interactive
         debugging is requested
      */

      if (procData.yod_msg.option_bits & OPT_BT) {

        get_bt = 1;

        rc = gdb_pipes();
        if (rc != APP_HOST_OK) {
          appStatus.rc = rc;
          log_msg("go_app_host: could not open gdb pipes");
          break;
        }

        if ( access(gdb_binary, F_OK) < 0 ) {
          appStatus.rc = LAUNCH_ERR_LOC_GDB_BIN; 
          log_msg("go_app_host: could not find gdb binary:");
          log_msg(gdb_binary);
          break;
        }

        if ( access(gdb_binary, X_OK) < 0 ) {
          appStatus.rc = LAUNCH_ERR_EXEC_GDB_BIN; 
          log_msg("go_app_host: do not have execute permission for gdb binary:");
          log_msg(gdb_binary);
          break;
        }

        /*
        ** before sending app along to user code, attach
        ** to it with debugger
        */

#ifdef KERNEL_ADDR_CACHE
        if ( syscall(__NR_ioctl, cache_fd, CACHE_INVALIDATE, 0) <0) {
          fprintf(stderr, "pct: CACHE_INVALIDATE failed 1\n");
          exit(-1);
        } 
#endif

        if ( (gpid = fork()) < 0 ) {
          appStatus.rc = LAUNCH_ERR_FORK_GDB; 
          log_msg("go_app_host: problem w/ fork for gdb");
          break;
        }

        if ( gpid > 0 ) {  /* parent -- get gdb prompt */

          if ( get_bt ) {
            bt_w_gdb++;
          }
          else {
            log_msg("go_app_host: starring gdb becuz why???");
            exit(-1);
          }

          /* write to fd_w_gdb[1] */
          close(fd_w_gdb[0]);
   
          /* read from fd_r_gdb[0] */
          close(fd_r_gdb[1]);

          LOCATION("go_app_host", "about to get gdb prompt");

          /* set flag on pipe we read gdb stuff from so that read()s
             will be non-blocking -- we want to avoid the following
             situation: some other process faults and the local process
             hangs waiting on a result from that process, thus hanging
             gdb, thus hanging the pct in readgdb(). instead, return
             immediately from readgdb() in case read()ing a char got
             nothing (and we know it's not EOF -- see getch()) 
          */
          fcntl(fd_r_gdb[0], F_SETFL, O_NONBLOCK);

          gdb_pos = 0;
          get_gdb_prompt();

        }
        else {              /* child -- exec gdb */

	  ename = gdbProc;

          itoa(appStatus.pid, pid_str);   /* pid to ascii for execv */

          /* child reads from this pipe -- parent writes */
          close(fd_w_gdb[1]); 

          /* child writes to this pipe  -- parent reads */
          close(fd_r_gdb[0]); 

          /* redirect child's (gdb's) io to parent (pct) */
          dup2( fd_r_gdb[1], STDOUT_FILENO );
          dup2( fd_w_gdb[0], STDIN_FILENO  );

          argv_gdb[0] = strdup("gdb");
          argv_gdb[1] = procData.pname;
          argv_gdb[2] = pid_str;
          argv_gdb[3] = 0;
          if ( execv(gdb_binary, argv_gdb) < 0 ) {
            /* what should be really do? -- note, before forking
               we test executableness of this file... 
            */
            //appStatus.rc = LAUNCH_ERR_???_GDB; 
            break;
          }
          free(argv_gdb[0]);
        }
      }

      rc = srvr_send_to_control_ptl(_my_pnid, appStatus.ppid,
               child_go_main_ptl, CHILD_GO_MAIN_TOKEN, NULL, 0);

      if (rc){
        appStatus.rc = LAUNCH_ERR_CHILD_COMM;
        break;
      }
      else{
        appStatus.u_stat |= SENT_TO_MAIN;
        appStatus.attn = 1;
      }

      break;

    case TODO_ABORT1:
      LOCATION("main_pct_request", "TODO_ABORT1");

#if 0
      /* in case we are debugging the app, remember that
         we got aborted intentionally -- this way we will
         know not to send gdb output to yod...
      */
      got_abort=1;
#endif

      if (!appStatus.started || appStatus.done){
	if (dbglevel){
	    log_msg("go_app_host: got ABORT but skip it, app not alive");
	}
        break;
      }
      if (dbglevel >= 1){
        log_msg("go_app_host: send SIGTERM to app process %d",
                            appStatus.pid);
      }
      kill(appStatus.pid, SIGTERM);

      appStatus.u_stat |= SENT_KILL_1;

      break;

    case TODO_ABORT2:
      if (!appStatus.started || appStatus.done){
        break;
      }
      if (appHistory & SENT_KILL_2){
        log_msg("go_app_host: abort2 request repeated, ignored");
        break;
      }
      if (dbglevel >= 1){
        log_msg("go_app_host: send SIGKILL to app process %d",
                            appStatus.pid);
      }
      kill(appStatus.pid, SIGKILL);

      appStatus.u_stat |= SENT_KILL_2;
      break;

    case TODO_SIGUSR1:
    case TODO_SIGUSR2:
      LOCATION("main_pct_request", "TODO_SIGUSR");

      if (!appStatus.started || appStatus.done){
        break;
      }

      if (dbglevel >= 1){
        log_msg("go_app_host: send SIGUSR%d to app process %d",
                    ((todo == TODO_SIGUSR1) ? 1 : 2),
                            appStatus.pid);
      }
            
      kill(appStatus.pid, ((todo == TODO_SIGUSR1) ? SIGUSR1 : SIGUSR2));

      break;


    case TODO_RESET:
      LOCATION("main_pct_request", "TODO_RESET");

      if (dbglevel >= 1){
        log_msg("go_app_host: reset app host at request of main pct");
      }

      rc = reset_app_host();

      if (rc){
	  log_warning("pct_app_host: reset app host failure");
	  appStatus.rc = LAUNCH_ERR_FATAL;
      }

      break;

    case TODO_ALLDONE:
      LOCATION("main_pct_request", "TODO_ALLDONE");
      /*
      ** clean up local state
      */
      if (!appStatus.done){
        log_msg("go_app_host: all done rec'd while app process alive");
        appStatus.rc = LAUNCH_ERR_HOST_PROTOCOL;
        break;
      }
      if (dbglevel >= 1){
        log_msg("go_app_host: main pct says app all done");
      }
      clear_state();

      break;

    default:
      log_msg("go_app_host: invalid request type %x",todo);
      appStatus.rc = LAUNCH_ERR_HOST_PROTOCOL;
  }
  appHistory |= appStatus.u_stat;

  if (appStatus.u_stat) appStatus.attn = 1;


  return;
}
/***********************************************************************
**   application process debugging
***********************************************************************/
char *
get_back_trace()
{
char *debugger_output;
    /*
    ** Main PCT wants the last nbytes of debugger output
    */
    debugger_output = cout;
    return debugger_output;
}

#if 0
/***********************************************************************
**   process incoming messages from application process
***********************************************************************/
static int
consume_app_messages(void)
{
control_msg_handle app_req;
int rc, status;
 
    status = 0;

    LOCATION("consume_app_messages", "top");
    while (1){
        rc = start_request_check(&app_req);
     
        if (rc == 1){
            rc = free_app_message(&app_req);
	    if (rc){
		log_msg("pct_app_host: free app message error");
		status = -1;
		break;
	    }
        }
	else if (rc < 0){
	    log_msg("consume_app_messages: start_request_check error");
	    status = -1;
	    break;
	}
        else{
           break;
        }
    }
    return status;
}
#endif

static void
listen_app_process(void)
{
int rc, okToFree, childPtl;
int src_nid, src_pid, msg_type, req_len;
char *user_data;
control_msg_handle app_msg;
out_of_band_msg *dump_msg;

    LOCATION("listen_app_process", "top");

    appStatus.rc = APP_HOST_OK;

    while ( 1 ){

	LOCATION("listen_app_process", "start_request_check");

	rc = start_request_check(&app_msg);

	LOCATION("listen_app_process", "process start request");

        if (rc == 0) break;

        if (rc < 0){
            log_warning("listen_app_process: error checking for msgs");
            appStatus.rc = LAUNCH_ERR_CHILD_COMM;
            break;
        }
	okToFree = 1;

        src_nid = SRVR_HANDLE_NID(app_msg);
        src_pid = SRVR_HANDLE_PID(app_msg);
        msg_type = SRVR_HANDLE_TYPE(app_msg);
        user_data = SRVR_HANDLE_USERDEF(app_msg);
        req_len = SRVR_HANDLE_TRANSFER_LEN(app_msg);

        if ((src_nid != _my_pnid) ||
	    ( (appHistory & GOT_PORTAL_ID) && (src_pid != appStatus.ppid))){

            log_msg("dropping message at app portal, unknown source %d/%d\n",
		                   src_nid, src_pid);

            rc = free_app_message(&app_msg);

	    if (rc){
		log_warning("free app message error");
		appStatus.rc = LAUNCH_ERR_FATAL;
		break;
	    }
            continue;
        }

        switch (msg_type){

            case CHILD_BEBOPD_ID_REQUEST:

                LOCATION("listen_app_process", "CHILD_BEBOPD_ID_REQUEST");
                
                if (dbglevel >= 1){
	            log_msg("go_app_host: child needs bebopd ID");
                }

                childPtl = *(int *)user_data;

		find_bebopd_id(&bebopd_id.nid, &bebopd_id.pid, &bebopd_id.ptl);

		bebopd_id.ptl = DNA_PTL;  /* apps use this */

		rc = srvr_send_to_control_ptl(src_nid, src_pid, childPtl,
		           0, (char *)&bebopd_id, sizeof(server_id));

                if (dbglevel >= 1){
	            log_msg("go_app_host: sent %d/%d/%d to child %d/%d/%d\n",
		          bebopd_id.nid, bebopd_id.pid, bebopd_id.ptl,
			  src_nid, src_pid, childPtl);
                }

		if (rc){
		    log_warning("Problem sending bebopd ID to child");
		}


                break;


            case CHILD_PORTAL_ID:

                LOCATION("listen_app_process", "CHILD_PORTAL_ID");
                
                if (dbglevel >= 1){
	            log_msg("go_app_host: child sent portal ID");
                }

                appStatus.ppid = *(ppid_type *)user_data;

	        appStatus.u_stat |= GOT_PORTAL_ID;
	        appStatus.attn = 1;
                break;

            case CHILD_PID_MAP_REQUEST:

                LOCATION("listen_app_process", "CHILD_PID_MAP_REQUEST");
                if (appHistory & REQUESTED_PIDMAP){
                    log_msg("app requested pid list again");
                    appStatus.rc = LAUNCH_ERR_APP_PROTOCOL;
                }
                else{
                    if (dbglevel >= 1){
                        log_msg("go_app_host: child wants pid map");
                    }
                    appStatus.attn = 1;
                    appStatus.u_stat |= REQUESTED_PIDMAP;

                    if (procData.IDmap[0] > 0){  /* we have the PIDs */

                        if (dbglevel >= 1){
                            log_msg("go_app_host: sending pid map");
                        }
                        rc = srvr_comm_get_reply(&app_msg, 
                                    (char *)&procData.IDmap, req_len);

                        if (rc){
                            appStatus.rc = LAUNCH_ERR_CHILD_COMM;
                            break;
                        }
                        appStatus.u_stat |= SENT_PIDMAP;
                    }
                    else{  /* send pid map later on when we get it */
                        memcpy((char *)&app_pid_req, (char *)&app_msg,
                                    sizeof(control_msg_handle));

                        okToFree = 0;  /* keep ctl msg around */
                    }
                }
                break;

            case CHILD_NID_MAP_REQUEST:

                LOCATION("listen_app_process", "CHILD_NID_MAP_REQUEST");
                if (appHistory & REQUESTED_NIDMAP){
                    log_msg("app requested nid list again");
                    appStatus.rc = LAUNCH_ERR_APP_PROTOCOL;
                }
                else{
                    if (dbglevel >= 1){
                        log_msg("go_app_host: child wants nid map");
                    }
                    appStatus.u_stat |= REQUESTED_NIDMAP;
                    appStatus.attn = 1;

                    if (dbglevel >= 1){
                        log_msg("go_app_host: sending nid map");
                    }
                    rc = srvr_comm_get_reply(&app_msg, (char *)&procData.nids,
                                sizeof(nid_type) * procData.nprocs);

                    if (rc){
                        appStatus.rc = LAUNCH_ERR_CHILD_COMM;
                        break;
                    }
                    appStatus.u_stat |= SENT_NIDMAP;
                }
                break;

            case CHILD_FYOD_MAP_REQUEST:

                if (dbglevel >= 1){
                  log_msg("go_app_host: child wants fyod map");
                }
                rc = srvr_comm_get_reply(&app_msg, (char *) fyod_map,
                              sizeof(int) * FYOD_MAP_SZ);
                if (rc){
                  appStatus.rc = LAUNCH_ERR_CHILD_COMM;
                  break;
                }
                appStatus.u_stat |= SENT_FYODMAP;
                break;

            case CHILD_FILEIO_REQUEST:

                LOCATION("listen_app_process", "CHILD_FILEIO_REQUEST");
                if (appHistory & REQUESTED_FILEIO){
                    log_msg("app requested file data again");
                    appStatus.rc = LAUNCH_ERR_APP_PROTOCOL;
                }
                else{
                    if (dbglevel >= 1){
                        log_msg("go_app_host: child wants std io info");
                    }
                    appStatus.u_stat |= REQUESTED_FILEIO;
                    appStatus.attn = 1;

                    if (dbglevel >= 1){
                        log_msg("go_app_host: sending std io info");
                    }
                    rc = srvr_comm_get_reply(&app_msg, 
                                (char *)procData.yod_msg.fstdio,
                                sizeof(procData.yod_msg.fstdio));

                    if (rc){
                        appStatus.rc = LAUNCH_ERR_CHILD_COMM;
                        if (dbglevel >= 1)
                            log_msg("go_app_host: With Error");
                        break;
                    }
                    appStatus.u_stat |= SENT_FILEIO;
                }
                break;

            case CHILD_IS_READY_TOKEN:

                LOCATION("listen_app_process", "CHILD_IS_READY_TOKEN");
                if (appHistory & GOT_READY){
                    log_msg("app sent ready again");
                    appStatus.rc = LAUNCH_ERR_APP_PROTOCOL;
                }
                else{
                    if (dbglevel >= 1){
                        log_msg("go_app_host: child sent ready token");
                    }
                    child_go_main_ptl = *(int *)(user_data);
                    appStatus.u_stat |= GOT_READY;
                    appStatus.attn = 1;
                }
                break;

            case CHILD_NACK_TOKEN:

                LOCATION("listen_app_process", "CHILD_NACK_TOKEN");
                memcpy((char *)&appStatus.nack, user_data,
                         sizeof(child_nack_msg));

                if (dbglevel >= 1){
                    log_msg("go_app_host: child sent nack token: %s",
                       start_error_string(appStatus.nack.start_log));
                }
                appStatus.u_stat |= CHILD_NACK;
                appStatus.attn   = 1;

                break;

            default:      /* app can request pct to log some info */

                LOCATION("listen_app_process", "child debug data");
                dump_msg = (out_of_band_msg *)user_data;
     
                log_msg("dumped from %d, type %d (0x%x): %d %d %d %d %p %p",
                    src_pid, msg_type,  msg_type,
                    dump_msg->int1, dump_msg->int2, dump_msg->int3, dump_msg->int4,
                    dump_msg->ptr1, dump_msg->ptr2);

                break;

        }
	if (okToFree){
	    rc = free_app_message(&app_msg);

	    if (rc){
		log_warning("free app message error");
		appStatus.rc = LAUNCH_ERR_FATAL;
	    }
	}

        appHistory |= appStatus.u_stat;

        if (appStatus.rc != APP_HOST_OK) break;
    }

    return;
}
/***********************************************************************
**   app process termination
***********************************************************************/
static void
wait_for_app(void)
{
pid_t wpid;
int wstatus, fd;
char fname[MAXPATHLEN];

    LOCATION("wait_for_app", "top");

    wpid = waitpid(appStatus.pid, &wstatus, WNOHANG);

    if (wpid == -1){
        if (errno == ECHILD){
            /*
            ** This could mean one of two things:
            **
            ** (1) The child process is no longer a child of the PCT.
            ** This may mean that someone has attached a debugger to it.
            ** We permit this wandering of the child and anticipate it
            ** will return someday.
            **
            ** (2) The child process no longer exists.  This could happen
            ** if someone attached a debugger to it and killed the process.
            ** We should behave in this case as if we had caught the
            ** child process' termination.
            */
            sprintf(fname,"/proc/%d",appStatus.pid);
 
            fd = open(fname, O_RDONLY);
 
            if (fd < 0){  /* case (2) child process is done */
                log_msg("app process done - though it's no longer mine");
 
                child_done(0);
 
                appStatus.attn = 1;
 
            }
            else{              /* case (1) child process must be adopted */
                close(fd);
            }
        }
        else{
            log_warning("waitpid: error waiting for app process termination");
            appStatus.rc = LAUNCH_ERR_WAIT;
        }
    }
    else if (wpid == appStatus.pid){

        if (dbglevel >= 1){
            log_msg("go_app_host: waitpid returned %d, app process done",
                     wpid);
        }
        child_done(wstatus);

        appStatus.attn = 1;
    }
    return;
}
static void
child_done(int wstatus)
{

    LOCATION("child_done", "top");
     
    /*
    ** Kill any subprocesses of the application child process.
    */
    killpg(appStatus.pid, SIGKILL);

    /*
    ** Remove application process from table of portals processes
    */

    appStatus.done = 1;
    appStatus.end_time = time(NULL);  /* more or less */
    appStatus.u_stat |= CHILD_DONE;

    /* kill a possible gdb */
    if (gpid > 0) {
      log_msg("sending gdb SIGKILL...");
      kill(gpid, SIGKILL);
      close_gdb();
    }

    /*
    ** If we wait()'ed for child process, record termination statuses.
    ** If a debugger told us child terminated, we don't have termination
    ** status info, it's in the text of the debugger output.
    */
    if (wstatus == 0){
        appStatus.rc = 0;
        appStatus.child_rc = 0;
        appStatus.child_term_sig = 0;
    }
    else{
        if (WIFEXITED(wstatus)){
            appStatus.child_rc = WEXITSTATUS(wstatus);

            if (dbglevel >= 1){
              log_msg("go_app_host: child exit code %d",appStatus.child_rc);
            }

            if (appStatus.child_rc == EXEC_FAILURE){
                appStatus.rc = LAUNCH_ERR_EXEC;
            }
            else if (appStatus.child_rc == PRE_EXEC_FAILURE){
                appStatus.rc = LAUNCH_ERR_PRE_EXEC;
            }

        }
        else {
            appStatus.child_rc = 0;
        }
        if (WIFSIGNALED(wstatus)){
            appStatus.child_term_sig = WTERMSIG(wstatus);

            if (dbglevel >= 1){
                log_msg("terminating signal %d",appStatus.child_term_sig);
            }
        }
        else{
            appStatus.child_term_sig = 0;
        }
    }

    if (Dbglevel >= 1 || 
        appStatus.child_rc != 0 ||
        appStatus.child_term_sig != 0) {  
        log_msg("app process done, exit code %d, terminating signal %d",
                 appStatus.child_rc, appStatus.child_term_sig);
    }
    return;
}
/***********************************************************************
**   set up and start app process
***********************************************************************/

enum {yodnid, yodpid, yodptl, numprocs,
      groupid, u_mask, yourrank, yurname,
      fyodnid,
      launch_ptl, session_id, parent_handle, my_handle,
      mpilib, nxlib, log_startup_actions, sleep_before_main,
      myuid, myeuid, mygid, myegid, lastvar};
 
static char *envp[MAX_ENVP], *argv[MAX_ARGV];
static char envvars[lastvar][80];


static int
start_user_process(int *appPid, int *userStatus)
{
int i, en, an, rc; 
int runStrace;

    /*
    ** Create environment, fork and exec app process
    */
    LOCATION("start_user_process", "top");
 
    if (debugSpecial){
       td3 = dclock();
    }

    /*************************************************************
    ** Set up argument and environment lists for new app process
    *************************************************************/
    if (dbglevel >= 2){
        log_msg("Unpack environment variables");
    }
    en = unpack_strings(procData.envlist, envp, MAX_ENVP);
 
    if (en < 0){
        log_msg(
           "start_user_process: error unpacking new process environment, job %d",
            procData.job_id);
        return LAUNCH_ERR_LOAD_DATA;
    }
    /*
    ** We add some information to the new process' environment, specifically
    ** how to talk to the yod that loaded the group, how to talk to the
    ** local pct, and the size of the application group.
    **
    ** We also need to add initial UID information to environment, because
    ** getuid calls are made before our startup code.  Our getuid routines
    ** pull uid info from yod, which doesn't work prior to the execution
    ** of our startup code.  So if getuid calls are made before startup,
    ** we pull the uid from the environment.
    */
    LOCATION("start_user_process", "set up env vars");

    if ((en + lastvar) >= MAX_ENVP){
        log_msg( "ran out of environment variable slots");
        return LAUNCH_ERR_LOAD_DATA;
    }
    sprintf(envvars[yourrank], "__MYRANK=%d", procData.my_rank);
    sprintf(envvars[yurname], "__MYNAME=%s", procData.pname
                                      +fndnm(procData.pname));

    sprintf(envvars[yodnid], "__YODNID=%d", procData.srvr.nid);
    sprintf(envvars[yodpid], "__YODPID=%d", procData.srvr.pid);
    sprintf(envvars[yodptl], "__YODPTL=%d", procData.yod_msg.app_serv_ptl);
 
    sprintf(envvars[myuid], "__UID=%d", procData.yod_msg.uid);
    sprintf(envvars[myeuid], "__EUID=%d", procData.yod_msg.euid);
    sprintf(envvars[mygid], "__GID=%d", procData.yod_msg.gid);
    sprintf(envvars[myegid], "__EGID=%d", procData.yod_msg.egid);
 
    sprintf(envvars[numprocs], "__NPROCS=%d", procData.nprocs);
 
    sprintf(envvars[groupid], "__GROUPID=%d", procData.job_id);
 
    sprintf(envvars[launch_ptl], "__PCT_PORTAL=%d", app_ctl_portal );

    sprintf(envvars[session_id], "__PBS_JOB_ID=%d", procData.session_id);

    sprintf(envvars[parent_handle], "__MYPARENT=%d", init_msg.parent_handle);

    sprintf(envvars[my_handle], "__MYSELF=%d", init_msg.my_handle);
 
    sprintf(envvars[u_mask], "__UMASK=%d", procData.yod_msg.u_mask);

    sprintf(envvars[fyodnid], "__FYODNID=%d", procData.yod_msg.fyod_nid);

    if (Dbglevel > 0) {
      log_msg("fyod_nid= %d", procData.yod_msg.fyod_nid);
    }
 
    if (procData.yod_msg.option_bits & OPT_LOG){
        strcpy(envvars[log_startup_actions], "__LOG_CPLANT_STARTUP=1");
    }
    else{
        strcpy(envvars[log_startup_actions], "__DONT_LOG=1");
    }
 
    /*
    ** Four manual debugging entry points - two before exec (in this
    ** source file), two in startup code (after exec).
    */
    if (procData.yod_msg.option_bits & OPT_SLEEP_3){
        strcpy(envvars[sleep_before_main], "__SLEEP_AT_STARTUP=1");
    }
    else if (procData.yod_msg.option_bits & OPT_SLEEP_4){
        strcpy(envvars[sleep_before_main], "__SLEEP_BEFORE_MAIN=1");
    }
    else{
        strcpy(envvars[sleep_before_main], "__DONT_SLEEP=1");
    }

    for (i=0; i < lastvar; i++){
        envp[en++] = envvars[i];
    }
 
    envp[en]  = (char *)0;

    /*
    ** Set up argument list for new process
    */
    if (dbglevel >= 2){
        log_msg("Unpack argument list");
    }
    an = unpack_strings(procData.arglist, argv, MAX_ARGV);
 
    if (an < 0){
        log_msg("start_user_process: error unpacking new process arg list\n");
        return LAUNCH_ERR_LOAD_DATA;
    }
    /*************************************************************
    ** Now fork and exec the process(es)
    *************************************************************/
 
    if (dbglevel >= 1){
        log_msg("app host: fork/exec %s",procData.pname);
    }

    /*
    ** PCT must be written such that messages will not be arriving in
    ** it's portals during a fork() call.  Just prior to fork'ing/exec'ing
    ** the user process, the PCTs entered into a broadcast of the executable,
    ** which was followed by a barrier.  After the PCT leaves this barrier
    ** it comes over here and calls fork.  
    ** 
    ** So:
    ** 
    ** o  PCTs come out of barrier
    ** o  PCTs call fork(), then exec the app process

    ** o  each PCT sends PCT a PORTAL_ID message (obviously the fork() is done)
    ** o  yod waits to receive all portal IDs, and then sends the root PCT
    **       a message to come get the portal ID map and fan it out.

    ** o  PCTs resume group communication and yod resumes sending messages to
    **            the PCTs
    */
    if (Dbglevel >= 1) {
        log_msg("fork child application %d",procData.job_id);
    }

#ifdef KERNEL_ADDR_CACHE
    if ( syscall(__NR_ioctl, cache_fd, CACHE_INVALIDATE, 0) <0) {
      log_error("pct: CACHE_INVALIDATE failed 0");
    } 
#endif

    *appPid = fork();

    if (*appPid == 0){   /* child */

        ename = appProc;

        LOCATION("app proc just forked","top");

        if (procData.yod_msg.option_bits & OPT_SLEEP_1){
	    sleep(60);
	}
	/* 
	** Set process group ID of child and descendants to process ID 
	** of child, now we can signal the whole bunch with killpg().
	*/
        LOCATION("app proc just forked","setup id");

	rc = setpgrp();

        if (rc == 0){
            rc = setregid(procData.yod_msg.gid, procData.yod_msg.egid);
        }

        if (rc == 0){
	    if (procData.yod_msg.ngroups > FEW_GROUPS){
	        rc = setgroups(procData.yod_msg.ngroups, procData.groupList);
	    }
	    else{
   	        rc = setgroups(procData.yod_msg.ngroups, procData.yod_msg.groups);
	    }
	}

        if (rc == 0){
            rc = setreuid(procData.yod_msg.uid, procData.yod_msg.euid);
        }


        if (rc < 0){
	    exit(PRE_EXEC_FAILURE);     /* exec setup failed */
	}

        if (procData.yod_msg.option_bits & OPT_SLEEP_2){
	    sleep(60);
	}

	if (procData.strace){

            LOCATION("app proc just forked","strace ranks");

	    if (procData.strace->listlen){
	        char *c;
		int me;

		c = (char *)procData.strace + 
		    sizeof(straceInfo) + 
		    procData.strace->dirlen + 1 +
		    procData.strace->optlen + 1;

                me  = findInList(c, procData.my_rank);

		if (me >= 0){
		    runStrace = 1;
		}
		else{
		    runStrace = 0;
		}
	    }
	    else{    /* no rank list means only rank 0 runs strace on the process */

	        if (procData.my_rank == 0){
		    runStrace = 1;
		}
		else{
		    runStrace = 0;
		}
	    }

	}
	else{
	    runStrace = 0;
	}

	if (runStrace){
            LOCATION("app proc just forked","exec with strace");

	    exec_with_strace();
	}
	else{
            LOCATION("app proc just forked","exec");

 	    execve(procData.pname, argv, envp);
        } 

	exit(EXEC_FAILURE);     /* exec failed */
    }
    if (*appPid < 0){
         log_warning("start_user_process: fork failed");
         return LAUNCH_ERR_FORK;
    }
    /*
    ** Both PCT and child app process set process group of app process
    ** to equal it's process id.  This is to avoid a race condition.
    */
    setpgid(*appPid, 0);

    *userStatus |= STARTED;

    if (debugSpecial){
        log_msg("  TIMING: %f to setup, fork/exec job",dclock()-td3);
    }
    return APP_HOST_OK;
}
/*
** strace info is in procData.strace, arranged like so:
**
**     int job_ID                   -|
**     int dirlen                    |  this is straceInfo typedef
**     int optlen                    |
**     int listlen                  -|
**     null terminated directory name string
**     null terminated option string, or null if no options specified
**     null terminated rank list, or null if no ranks listed (default is rank 0 only)
*/
static void
exec_with_strace()
{
char *newargv[128];
char *dir, *opt, *optend, *c;
int i, nargs;
char outpath[MAXPATHLEN];

    dir = (char *)procData.strace + sizeof(straceInfo);
    opt = dir + procData.strace->dirlen + 1;  /* first char in option string */
    optend = opt + procData.strace->optlen;   /* last char in option string */

    sprintf(outpath,"%s/strace.%d.%d", dir, procData.strace->job_ID, procData.my_rank);

    newargv[0] = "/usr/bin/strace";
    newargv[1] = "-o";
    newargv[2] = outpath;

    nargs = 3;

    if (opt){                       /* strace options */
        /*
	** replace all spaces in the options string with NULL bytes
	*/
	c = opt;

	while (*c){
	    while (*c && !isblank((int)*c)) c++;
	    while (*c && isblank((int)*c)) *c++ = 0;
	}
	/*
	** set strace options to those in option string
	*/
	c = opt;

	while (c <= optend){
	    
	    if (*c){
	        newargv[nargs++] = c;
		if (nargs >= 126) break;
		while (*c++);
	    }
	    else{
	        while ((c <= optend) && (*c++ == 0));
	    }
	}
    }

    newargv[nargs++] = procData.pname;   /* program to trace */

    for (i=1; argv[i]; i++){             /* program options */

        if (nargs >= 126){
	     log_msg("error in getting strace ready");
	     exit(PRE_EXEC_FAILURE);
        }

	newargv[nargs++] = argv[i];
    }
    newargv[nargs] = (char *)0;


    if (dbglevel >= 1){
        log_msg("Running strace:\n");
	for (i=0; i<nargs; i++){
	    log_msg("    %s\n",newargv[i]);
	} 
    }

    execve(newargv[0], newargv, envp);
}
static int
unpack_strings(char *packed, char **unpacked, int maxu)
{
char *p;
int num, slen;
 
    LOCATION("unpack_strings", "");

    if (!packed){
        unpacked[0] = 0;
        return 0;
    }
 
    p = packed;
    num = 0;
 
    while (*p && (num < maxu)){
 
        unpacked[num++] = p;
 
        if (dbglevel >= 3){
            fprintf(stderr, "\t%s\n",p);
        }
 
        slen = strlen(p) + 1;
 
        p += slen;
    }
    if (num == maxu){
        log_msg("unpack_strings: ran out of slots\n");
        return -1;
    }
    unpacked[num] = NULL;
 
    return num;
}

/***********************************************************************
**   initialize portals required by go_app_host
***********************************************************************/

static int
init_pct_app_host()
{
int rc, status;

    LOCATION("init_pct_app_host", "");

    status = APP_HOST_OK;

    if (dbglevel >= 1){
        log_msg("go_app_host initializing portals");
    }

    rc = initialize_app_portals(&app_ctl_portal);

    if (rc){
        status = LAUNCH_ERR_FATAL;
    }

    return status;
}

/***********************************************************************
**   clean up, reset
***********************************************************************/
void
display_app_state()
{
int mask,i;

    log_msg("attn %d started %d done %d \n",
	  appStatus.attn, appStatus.started,
	  appStatus.done);
    log_msg("pid %d ppid %d child_rc %d child_term_sig %d\n",
	 appStatus.pid, appStatus.ppid, appStatus.child_rc,
	  appStatus.child_term_sig );
     log_msg("start_time %d  end_time %d  bt_size %d\n",
	 appStatus.start_time, appStatus.end_time, appStatus.bt_size);

     log_msg("status 0x%04x\n",appStatus.u_stat);

     if (appStatus.u_stat){
        for (i=1, mask = 1; i <=16 ; i++, mask <<= 1){
 
            if (appStatus.u_stat & mask){
                log_msg("child status: %s",child_status_strings[i]);
            }
         }
     }
}
static void
clear_state()
{
    LOCATION("clear_state", "");

    DbgIn = NULL;


    memset((char *)&appStatus, 0, sizeof(appData));
    memset((char *)&procData, 0, sizeof(proc_list));

    appHistory = 0;

    child_go_main_ptl = SRVR_INVAL_PTL;

    numPeers = 0;
}

int
reset_app_host()
{
int rc;
double reset_timeout=5.0;                /* in seconds */

    LOCATION("reset_app_host", "");

    if (gpid > 0) {                      /* kill debugger */
      log_msg("reset_app_host: sending gdb SIGKILL...");
      kill(gpid, SIGKILL);
      close_gdb();
    }

    if (app_ctl_portal == SRVR_INVAL_PTL){
        return 0;                        /* nothing to reset */
    }
    if (dbglevel){
	log_msg("reset_app_host");
	display_app_state();
    }
/*
    rc = consume_app_messages();

    if (rc){
	return -1;
    }

    we sometimes get errors in consume app messages - there should be
    no messages, but they are found and freeing them we get timeout
    errors, pct subsequently seg faults.
*/
    clear_app_messages();


    if ((appStatus.pid != 0)    && 
	(appStatus.started)     &&
	(!appStatus.done)   ){

        LOCATION("reset_app_host", "killpg");

        rc = killpg(appStatus.pid, SIGKILL);

	if (rc){
	    log_warning("reset_app_host: killpg(%d)",appStatus.pid);
	}
	else{

	    errno = 0;

            LOCATION("reset_app_host", "waitpid");
	    td2 = dclock();

	    while (1){
		rc = waitpid(appStatus.pid, NULL, WNOHANG);

		if (rc == appStatus.pid){    /* got it */
		    child_done(0);           /* release resources used by app */
		    break;
		}
		if (rc == 0){
		    if (errno == ECHILD){   /* child's not out there */
		        log_msg("reset_app_host: waitpid returned ECHILD for pid %d",
				  appStatus.pid);

		        break;
		    }
		    else{
	 	        if ((dclock() - td2) < reset_timeout){
			     continue;   /* child's there, keep waiting */
		        }
		        else{
			    log_msg(
			    "reset_app_host: waitpid(%d) timed out at %d seconds",
				  appStatus.pid, reset_timeout);
                            return -1;
		        }
		    }
		}
		log_warning("reset_app_host: waitpid(%d) error",appStatus.pid); 
		break;
	    }
	}
    }

    clear_state();

    return 0;
}

/* 
   open some pipes to use in redirecting gdb's io
*/
int
gdb_pipes()
{
  if ( pipe(fd_w_gdb) < 0 ) {
    log_msg("go_app_host: in gdb_pipes(), couldn't create first pipe");
    return LAUNCH_ERR_CRT_GDB_PIPE;
  }

  if ( pipe(fd_r_gdb) < 0 ) {
    log_msg("go_app_host: in gdb_pipes(), couldn't create second pipe");
    return LAUNCH_ERR_CRT_GDB_PIPE;
  }

  return APP_HOST_OK;
}


/* 
   integer to ascii 
*/
void itoa(int n, char s[]) {
  int i, j, c, sign;
  if ((sign = n) < 0)
    n = -n;
  j = 0;
  do {
    s[j++] = n % 10 + '0';
  } while ((n /= 10) > 0);
  if (sign < 0)
    s[j++] = '-';
  s[j--] = '\0';
  for (i=0; i<j; i++, j--) {
    c = s[i];
    s[i] = s[j];
    s[j] = c;
  }
}

/* 
   do character input from arbitrary
   file descriptor   
*/
int getch(int fd) 
{
  char c;
  int n;
  n = read(fd, &c, 1);

  /* error */
  if ( n == -1 ) {
    /* no bytes available right now -- assumes above read
       was made NON_BLOCKING w/ fcntl call 
    */
    if (errno == EAGAIN) {
      return 0;
    }
    /* there was a real error */
    return EOF;
  }

  if ( n == 1 ) {
    return c;
  }
  /* else, n == 0 */
  return EOF;
}

/*
   this routine does parsing "lines" of gdb output,
   where a line means either a string ending in a
   newline, or something that is recognized as a
   prompt for input -- these seem to be the two kinds
   of responses that one gets from gdb. they are distinct
   because prompts do not end in newlines, so we cannot
   generally sit around waiting for one.

   also, we do not want to hang waiting for a "line"
   in case something bad happens to gdb. therefore,
   this routine does nonblocking reads of a character
   at a time, and if it accumulates a significant no.
   of zero reads, then it returns with a status of
   INCOMPLETE_LN (see below). however, it maintains
   state so that getting of the current line can be
   continued on a subsequent call. 

   thus, the caller should be prepared to come away 
   empty-handed occasionally; it may need to make a 
   number of calls in order to get a complete line.

   return the type of gdb string read or EOF 
      EOF                   for EOF
      GDB_ERR               for error
      GDB_PROMPT            for gdb prompt
      GDB_PROG_RUNNING      for gdb long prompt
      RECEIVED_SIGNAL       for "Program received signal..." 
			     or "Program terminated with signal..."
      GDB_VERSION           for gdb version
      COPYRIGHT             for copyright info
      CURR_LANG             for curr lang msg (???)
      INCOMPLETE_LN         for return, but input incomplete
      GDB_OTHER             for other non-EOF
   return length of string read in *pos
   sz is the size of line[]
*/
int readgdb(int fd, int* inpos, char line[], int sz) 
{
  static int pos=0;
  static int did_prompt_check=0;
  static int did_long_prompt_check=0;
  int ch, result=0, starting=1, num_zeros=0;
  const char* gdb_signal_str = "Program received signal";
  const char* gdb_prompt_str = "(gdb) ";
  const char* gdb_running = "The program is running";
  const char* gdb_terminated_str = "Program terminated with signal";
  const char* gdb_exited_str = "Program exited with code";
  const char* gdb_version_str = "GNU gdb";
  const char* gdb_copy_str = "Copyright";
  const char* gdb_curl_str = "Current language:";

  if ( sz <= (int)strlen(gdb_prompt_str) ) {
    return GDB_ERR;
  } 

  /* the gdb prompt requires special handling since it
     doesn't supply a newline -- that's why we check 
     pos < len(gdb_prompt)
  */

  while ( (pos < (int)strlen(gdb_prompt_str) || did_prompt_check) && 
          (pos < (int)strlen(gdb_running) || did_long_prompt_check) 
               && (ch=getch(fd)) != EOF && ch != '\n' ) {
    /* if on the first attempt we get a 0, then no bytes available
       temporarily, so come back later 
    */
    if ( starting && ch == 0 ) {
      return INCOMPLETE_LN;
    }
    starting=0;

    if (ch != 0) {
      line[pos++] = ch;
    }
    else { /* implement a "timeout", so pct doesn't hang on a hung gdb */
      num_zeros++;
      if (num_zeros > 100) return INCOMPLETE_LN;
    }
  }

  /* check for gdb prompt */
  if ( pos == (int)strlen(gdb_prompt_str) ) {
    if ( !strncmp(gdb_prompt_str, line, strlen(gdb_prompt_str)) ) {
      pos = 0;
      did_prompt_check = 0;
      return GDB_PROMPT;
    }

    /* not prompt, record the fact that we did the check, i.e. that
       we got enough bytes for a prompt already, and come back
       for more later
    */
    did_prompt_check = 1;
  }

  /* gdb_running is also a prompt string */
  if ( pos == (int)strlen(gdb_running) ) {
    if ( !strncmp(gdb_running, line, strlen(gdb_running)) ) {
      pos = 0;
      did_long_prompt_check = 0;
      return GDB_PROG_RUNNING;
    }

    /* not prompt, record the fact that we did the check, i.e. that
       we got enough bytes for a prompt already, and come back
       for more later
    */
    did_long_prompt_check = 1;
  }

  /* return pos or EOF */
  if ( ch != '\n' ) {
    if ( ch != EOF ) {
      return INCOMPLETE_LN;
    }
    else {
      return EOF;
    }
  }
  else {
    line[pos] = ch;

    /* test for signal string */
    if ( !strncmp(gdb_signal_str,line,strlen(gdb_signal_str)) ) {
      result = RECEIVED_SIGNAL;
      goto end;
    }

    /* test for terminated string */
    if ( !strncmp(gdb_terminated_str,line,strlen(gdb_terminated_str)) ) {
      result = RECEIVED_SIGNAL;
      goto end; 
    }

    /* test for exited string */
    if ( !strncmp(gdb_exited_str,line,strlen(gdb_exited_str)) ) {
      result = RECEIVED_SIGNAL;
      goto end; 
    }

    /* test for version string */
    if ( !strncmp(gdb_version_str,line,strlen(gdb_version_str)) ) {
      result = GDB_VERSION;
      goto end;
    }

    /* test for copyright string */
    if ( !strncmp(gdb_copy_str,line,strlen(gdb_copy_str)) ) {
      result = COPYRIGHT;
      goto end;
    }

    /* test for current language (???) string */
    if ( !strncmp(gdb_curl_str,line,strlen(gdb_curl_str)) ) {
      result = CURR_LANG;
      goto end;
    }

    result = GDB_OTHER;

end:
    /* add 1 to pos to make it a length rather 
       than a position... 
    */
   *inpos = pos+1;
    pos = 0;
    did_prompt_check=0;
    return result; 
  }
}


int process_bt_req(control_msg_handle *bt_req)
{
int rc, wrc, fail;
int status, msg_type, src_nid, src_pid;
char *user_data;
int bt_snid, bt_spid, bt_ptl;
int tmout, nsent, bufnum;     /* no of secs to timeout on for put request */
static int count=0;

char *bt = NULL;
int bt_size=0;

char bad_jid[] = "ERROR(pct): request for debug info on bad job id\n";
int bad_jid_sz=strlen(bad_jid)+1;

char no_gdb[] = "ERROR(pct): gdb is not running (use yod w/ -bt option?)\n";
int no_gdb_sz=strlen(no_gdb)+1;

char cant_signal_gdb[] = "ERROR(pct): cant send SIGINT to gdb?!\n";
int cant_signal_gdb_sz=strlen(cant_signal_gdb)+1;

char get_bt_from_gdb[] = "ERROR(pct): trouble getting bt from gdb?!\n";
int get_bt_from_gdb_sz=strlen(get_bt_from_gdb)+1;

char* bt_err[25];
int    bt_sz[25];

int job_id;  /* get this out of the user data bits; it's the job id
                for the debugging request -- compare it w/ our current
                job's to rule out the possibility of an old or mistaken
                request
             */


    bt_err[PCT_BT_BAD_JID] = bad_jid;
    bt_err[PCT_BT_NO_GDB] = no_gdb;
    bt_err[PCT_BT_CANT_SIGNAL_GDB] = cant_signal_gdb;
    bt_err[PCT_BT_GET_BT_FROM_GDB] = get_bt_from_gdb;

    bt_sz[PCT_BT_BAD_JID] = bad_jid_sz;
    bt_sz[PCT_BT_NO_GDB] = no_gdb_sz;
    bt_sz[PCT_BT_CANT_SIGNAL_GDB] = cant_signal_gdb_sz;
    bt_sz[PCT_BT_GET_BT_FROM_GDB] = get_bt_from_gdb_sz;

    CLEAR_ERR;

    LOCATION("process_bt_req", "top");

    status = 0;   /* set non-zero only on pct fatal error */

    msg_type = SRVR_HANDLE_TYPE(*bt_req);
    user_data = SRVR_HANDLE_USERDEF(*bt_req);
    src_nid = SRVR_HANDLE_NID(*bt_req);
    src_pid = SRVR_HANDLE_PID(*bt_req);
    
    job_id = *( (int*) user_data);

    /*
    ** These "bt" messages are debug messages arriving from
    ** bt or from fellow PCTs fanning in/out to us.
    */

    switch (msg_type) {

      case BT_REQUEST_BT:    /* bt sends this to select pcts */

        if (Dbglevel > 0){
          log_msg("process_bt_req: BT_REQUEST_BT from %d/%d (%d)",
                       src_nid, src_pid, count++);
        }

        bt_snid = src_nid;
        bt_spid = src_pid;
        bt_ptl = BT_PCT_PTL;


        /* get back trace and send bt_size back to bt in ctl portal
           or
           make bt_xxxx global and set a flag that bt is waiting
           for a bt. then send SIGINT to gdb here, and handle gdb
           prompt elsewhere...
        */

        fail = -1;

        /* log msg but don't quit */
        if (job_id != procData.job_id) {
          log_msg("process_bt_req: bt request wrt job_id %d\n", job_id);
          log_msg("process_bt_req: but we're job_id %d\n", procData.job_id);

          fail = PCT_BT_BAD_JID;
          goto failcheck;
        }

        /* make sure gdb is running, else why bother */
        if (Dbglevel > 0) {
          log_msg("process_bt_req: check for gdb...");
        }
        if (gpid == 0) {
          log_msg("process_bt_req: gdb not running...");

          fail = PCT_BT_NO_GDB;
          goto failcheck;

        }

        if (Dbglevel > 0) {
          log_msg("process_bt_req: send gdb signal...");
        }
        /* try to send signal to gdb */
        if ( kill(gpid,SIGINT) < 0 ) {
          log_msg("process_bt_req: cant signal gdb...");

          fail = PCT_BT_CANT_SIGNAL_GDB;
          goto failcheck;
        }

        if (Dbglevel > 0) {
          log_msg("process_bt_req: get bt from gdb...");
        }
        /* try to get prompt from gdb */
        rc = gdb_interactive(&bt, &bt_size);

        if (rc < 0) {
          log_msg("process_bt_req: cant get bt from gdb...");

          fail = PCT_BT_GET_BT_FROM_GDB;
          goto failcheck;
        }

failcheck:
        if (fail >= 0) {

          bufnum = srvr_comm_put_req( bt_err[fail], bt_sz[fail],
	                          fail,
                                  (CHAR*) &bt_sz[fail], sizeof(int),
                                  1, &bt_snid, &bt_spid, &bt_ptl);

          if (Dbglevel > 0) {
            log_msg("process_bt_req: did srvr_comm_put_req()");
          }
        }
        else {

          bufnum = srvr_comm_put_req( bt, bt_size, PCT_BT_SEND_BT,
                                  (CHAR*) &bt_size, sizeof(int),
                                  1, &bt_snid, &bt_spid, &bt_ptl);

          if (Dbglevel > 0) {
            log_msg("process_bt_req: did srvr_comm_put_req()");
          }
        }

        tmout=10;
        nsent = 1;
        wrc = wait_bt_put_msg(bufnum, tmout, nsent);
        if (Dbglevel > 0) {
          log_msg("process_bt_req: did wait_bt_put_msg()");
        }

        srvr_delete_buf(bufnum);
        if (Dbglevel > 0) {
          log_msg("process_bt_req: did srvr_delete_buf()");
        }

        return wrc;

        break;

      default:
        log_msg("process_bt_req: ignoring msg type %d on bt msg portal from %d/%d\n", msg_type, src_nid, src_pid);

        status = 0;
        break;
    }

    return status;
}



int process_cgdb_req(control_msg_handle *cgdb_req)
{
int rc, wrc, fail, succeed;
int status, msg_type;
char *user_data;
int cgdb_snid, cgdb_spid, cgdb_ptl;
int tmout, nsent, bufnum;     /* no of secs to timeout on for put request */
static int count=0;

char serror[] = "ERROR(pct): start up of gdb wrapper failed\n";
int serror_sz=strlen(serror)+1;

char sok[] = "pct: start up of gdb wrapper ok\n";
int sok_sz=strlen(sok)+1;


    CLEAR_ERR;

    LOCATION("process_cgdb_req", "top");

    status = 0;   /* set non-zero only on pct fatal error */

    msg_type  = SRVR_HANDLE_TYPE(*cgdb_req);
    user_data = SRVR_HANDLE_USERDEF(*cgdb_req);
    cgdb_snid = SRVR_HANDLE_NID(*cgdb_req);
    cgdb_spid = SRVR_HANDLE_PID(*cgdb_req);
    cgdb_ptl  = CGDB_PCT_PTL;

    switch (msg_type) {

      case CGDB_REQUEST_PROXY:  /* cgdb sends this to select pcts */

        if (Dbglevel > 0){
          log_msg("process_cgdb_req: CGDB_REQUEST_START from %d/%d (%d)",
                       cgdb_snid, cgdb_spid, count++);
        }

        rc = start_proxy();

        fail = -1;

        if (rc < 0) {
          fail = PCT_CGDB_START_FAILED;
          log_msg("process_cgdb_req: could not start gdb proxy");
        }
        else {
          succeed = PCT_CGDB_START_OK;
        }

        if (fail >= 0) {

          bufnum = srvr_comm_put_req( serror, serror_sz, fail,
                                  (CHAR*) &serror_sz, sizeof(int) ,
                                  1, &cgdb_snid, &cgdb_spid, &cgdb_ptl);

          if (Dbglevel > 0) {
            log_msg("process_cgdb_req: did srvr_comm_put_req()");
          }
        }
        else {
          /* put just send success msg */

          bufnum = srvr_comm_put_req(sok, sok_sz, succeed,
                                  (CHAR*) &sok_sz, sizeof(int) ,
                                  1, &cgdb_snid, &cgdb_spid, &cgdb_ptl);

          if (Dbglevel > 0) {
            log_msg("process_cgdb_req: did srvr_comm_put_req()");
          }
        }

        tmout=10;
        nsent = 1;

        wrc = wait_cgdb_put_msg(bufnum, tmout, nsent);
        if (Dbglevel > 0) {
          log_msg("process_cgdb_req: did wait_cgdb_put_msg()");
        }

        srvr_delete_buf(bufnum);
        if (Dbglevel > 0) {
          log_msg("process_cgdb_req: did srvr_delete_buf()");
        }

        return wrc;

        break;

      default:
        log_msg("process_cgdb_req: ignoring msg type %d on cgdb msg portal from %d/%d\n", msg_type, cgdb_snid, cgdb_spid);

        status = 0;
        break;
    }

    return status;
}


/* basically wait_pcts_put_message() from yod */
static int wait_bt_put_msg(int bufnum, int tmout, int howmany)
{
  int rc;
  int count=0;

  while( count < tmout ) {
    rc = srvr_test_read_buf(bufnum, howmany);

    if (rc < 0) {
      log_msg("wait_bt_put_msg: srvr_test_read_buf failed");
      return -1;
    }
    if (rc == 1) return 0;

    sleep(1);
    count++;
  }
  log_msg("wait_bt_put_msg: timed out after %d seconds", tmout);

  return 0;   /* OK: pct doesn't exit just because yod or bt is gone */
}


static int wait_cgdb_put_msg(int bufnum, int tmout, int howmany)
{
  int rc;
  int count=0;

  while( count < tmout ) {
    rc = srvr_test_read_buf( bufnum, howmany);
    if (rc < 0) {
      log_msg("wait_cgdb_put_msg: srvr_test_read_buf failed");
      return -1;
    }
    if (rc == 1) return 0;

    sleep(1);
    count++;
  }
  log_msg("wait_cgdb_put_msg: timed out after %d seconds", tmout);

  return 0;   /* ok - this is not an error for this pct - it's external */
}


static int gdb_interactive(char** bt, int* btsize) {

  int i, prompts=0, gpos=0;

  if (gdb_result == EOF) {
    gdb_result = 0;
    return -1;
  }

  while(1) {

  if ( (prompts < 2) &&
       (gdb_result = readgdb(fd_r_gdb[0], &gpos, line, GDB_LN_SIZE)) != EOF ) {

    if ( gdb_result != INCOMPLETE_LN ) { /* else we read empty pipe; try later */

      line[gpos] = 0;
      log_msg("line= %s", line);

      /* handle prompt */

      if ( gdb_result == GDB_PROMPT ) {
        log_msg("gdb_interactive: got GDB_PROMPT....");

        prompts++;

        if (prompts == 1) {
          log_msg("gdbcmd= %s", "where\n");
          write( fd_w_gdb[1], "where\n", 6 );
          save_gdb_output = 1;
        }
        else {
          log_msg("gdbcmd= %s", "cont\n");
          write( fd_w_gdb[1], "cont\n", 5 );
          save_gdb_output = 0;
        }
      }
      else {
        /* line was not a gdb prompt, maybe some useful info... */

        if ( gdb_result == RECEIVED_SIGNAL ) {   /* "Program received signal" */
          log_msg("gdb_interactive: got prog RECEIVED SIGNAL...");
        }

        /* useful info about program fault */

        if ( save_gdb_output && (gdb_result != CURR_LANG) ) {
          for (i=0; i<gpos; i++) {
            cout[gdb_count++] = line[i];
          }
        }
        /* useless info prior to prompt */
        else {
          log_msg("gdb_interactive: useless info...");
        }
      }
    }
  }
  else {  /* got EOF?? -- probably just prompts == 2
             save back trace size, wait for gdb to exit, 
             and reset flags
          */
    log_msg("gdb_interactive: out of while...");

   *btsize = gdb_count;
   *bt = cout;
    gdb_count = 0;

    if ( gdb_result == EOF ) {
      log_msg("gdb_interactive: got EOF...");
      close_gdb();
    }
    break;
  }
} /* while */
return 0;  
}


void get_gdb_prompt()
{
  int i;

  /* read gdb output until we get the first prompt...
     reply to prompt with the 1st command (i.e. "cont") and
     break out -- then later, in each pct loop, readgdb() once
     and send the next command in response to a gdb prompt...
  */

  gt0 = dclock();
  while ( (gdb_result = readgdb(fd_r_gdb[0], &gdb_pos, line, GDB_LN_SIZE)) != EOF ) {
    /* only wait so long for prompt */
    if ( (dclock() - gt0) > gdb_timeout ) {
      log_msg("go_app_host: timeout on initial gdb prompt");

      /* the following is used by the main readgdb() test -- if
         it sees that result is already EOF, then it does its
         waitpid(gpid) thing
      */
      gdb_result = EOF;
      break;
    }

    if ( gdb_result != INCOMPLETE_LN ) {

      if ( gdb_result == GDB_PROMPT ) {      /* gdb prompt */
        write( fd_w_gdb[1], gdbcmd[gdbcmd_ind], gdbcmdlen[gdbcmd_ind] );
        gdbcmd_ind++;
        break;             /* out of while */
      }
      else {               /* line was not a prompt; could be ver */
        if ( gdb_result == GDB_VERSION || gdb_result == COPYRIGHT ) {
          for (i=0; i<gdb_pos; i++) {
            cout[gdb_count++] = line[i];
          }
        }
        else {
          /* useless info prior to program fault */ 
#if 0
          line[pos] = 0;
          printf("pct: %s", line);
#endif
        }
      }
    }
  }
  if ( gdb_result == EOF ) {    /* probably means gdb died -- normally
                               we get here via the above "break" 
                            */
    log_msg("go_app_host: gdb pipe hit unexpected EOF");
    bt_w_gdb = 0;
  }
}


void check_gdb_output(void) 
{
  int i;

  LOCATION("go_app_host", "check_gdb_output()");

  /* "result" might have got set to EOF because of a gdb
     timeout -- so we include logic here to fall through
     to waitpid() in that case
  */

  if ( (gdb_result != EOF) &&
    (gdb_result = readgdb(fd_r_gdb[0], &gdb_pos, line, GDB_LN_SIZE)) != EOF ) {

    if ( gdb_result != INCOMPLETE_LN ) { /* else we read empty pipe; 
                                        try later 
                                     */
      /* handle prompt */

      if ( gdb_result == GDB_PROMPT ) { /* gdb prompt */
        write( fd_w_gdb[1], gdbcmd[gdbcmd_ind], gdbcmdlen[gdbcmd_ind] );
        gdbcmd_ind++;

        /* if we have issued the second "continue" command then we are
           done w/ the backtrace; i.e., we issued the backtrace and later
           got a gdb prompt -- so stop saving gdb output...
        */
        if (gdbcmd_ind > CONT2) {
          save_gdb_output = 0;
        }
      }
      else {
        /* line was not a gdb prompt, maybe some useful info.
           if it begins w/ "Program received signal", record the fact
           and begin saving lines of output -- even before issuing the
           backtrace
        */
        if ( gdb_result == RECEIVED_SIGNAL ) {   /* "Program received signal" */
          app_faulted++;
          save_gdb_output++;
        }

        /* useful info about program fault */

        if ( save_gdb_output && (gdb_result != CURR_LANG) ) {
          for (i=0; i<gdb_pos; i++) {
            cout[gdb_count++] = line[i];
          }
        }
        /* useless info prior to program fault */
        else {
#if 0
          line[gdb_pos] = 0;
          printf("pct: %s", line);
#endif
        }
      }
    }
  }
  else {  /* got EOF -- save back trace size, wait for gdb to
             exit, and reset flags
          */

    if ( gdb_count &&
#if 0
          !got_abort &&
#endif
            app_faulted ) {
      appStatus.bt_size = gdb_count;
    }
    else {
      appStatus.bt_size = 0;
    }

    close_gdb();
  }
}


static void close_gdb(void) 
{
int status, rc; 

   td0 = dclock();

   while((dclock() - td0) < 30.0){
       rc = waitpid(gpid, &status, WNOHANG);
       if (rc == 0) continue;
       break;
   }
   gpid = 0;
   gdb_count = 0; 
   gdbcmd_ind = 0;
   gdb_result = 0;
   bt_w_gdb = 0;
   save_gdb_output = 0;
   app_faulted = 0;
   close(fd_w_gdb[1]);
   close(fd_r_gdb[0]);
}


static int start_proxy(void)
{
  if (appStatus.pid == 0) {
    log_msg("start_proxy: no application to attach to...\n");
    return -1;
  }

  if ( !(procData.yod_msg.option_bits & OPT_ATTACH) ) {
    log_msg("start_proxy: app not run under the attach option...\n");
    return -1;
  }

  if (gwrap_pid > 0) {
    log_msg("start_proxy: proxy already running with pid= %d", gwrap_pid);
    return -1;
  }


  if ( access(gdbwrap_binary, F_OK) < 0 ) {
    appStatus.rc = LAUNCH_ERR_LOC_GDBWRAP_BIN; 
    log_msg("start_proxy: could not find gdbWRAP binary:");
    log_msg(gdbwrap_binary);
    return -1;
  }

  if ( access(gdbwrap_binary, X_OK) < 0 ) {
    appStatus.rc = LAUNCH_ERR_EXEC_GDBWRAP_BIN; 
    log_msg("start_proxy: do not have execute permission for gdbWRAP binary:");
    log_msg(gdbwrap_binary);
    return -1;
  }

  /* before sending app along to user code, start up wrapper for gdb */

#ifdef KERNEL_ADDR_CACHE
  if ( syscall(__NR_ioctl, cache_fd, CACHE_INVALIDATE, 0) <0) {
    fprintf(stderr, "pct: CACHE_INVALIDATE failed 2\n");
    exit(-1);
  } 
#endif

  if ( (gwrap_pid = fork()) < 0 ) {
    appStatus.rc = LAUNCH_ERR_FORK_GDBWRAP; 
    log_msg("start_proxy: problem w/ fork for gdbWRAP");
    return -1;
  }

  if ( gwrap_pid > 0 ) {  /* parent */

    LOCATION("start_proxy", "just started gdbWRAP");

    if (dbglevel > 0) {
      log_msg("start_proxy: started gdbWRAP");
    }
    return 0;
  }
  else {   /* child -- exec gdbwrap */

    ename = gwrapProc;

    itoa(appStatus.pid, pid_str);   /* pid to ascii for execv */

    argv_gdbwrap[0] = strdup("wrap");
    argv_gdbwrap[1] = procData.pname;
    argv_gdbwrap[2] = pid_str;
    argv_gdbwrap[3] = 0;
    if ( execv(gdbwrap_binary, argv_gdbwrap) < 0 ) {
      log_msg("start_proxy: could not exec proxy binary\n");
      exit(-1);
    }
    free(argv_gdbwrap[0]);
  }
  return 0;
}
 
/* find the beginning of the last component of the given
   filename */
static int fndnm(char *c) 
{
  int i;
  char* a;

  i = strlen(c)-1;
  a = c+i;
  while ( *a != '/' && i>0 ) {
    a--;
    i--;
  }
  if (*a == '/') {
    return i+1;
  }
  return i;
}
