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
/* wrap.c */

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
 
#include "wrap.h"
#include "appload_msg.h"
#include "cgdb.h"
#include "config.h"
#include "bebopd.h"
#include "srvr_err.h"
#include "srvr_comm.h"
#include "srvr_coll.h"
#include "ppid.h"
#include "portal_assignments.h"
#include "srvr_coll_fail.h"
#include "pct_ports.h"
#include "defines.h"

/* this limits the size of a line of gdb output */
#define GDB_LN_SIZE 2001

/* this limits the total amount of output
   we can collect from gdb for a process */
#define GDB_TOTAL_OUT_SIZE 64*1024

#if 0
extern char* optarg;
static struct option opts[]=
{
  {"pid",     required_argument,  0, 'p'},  /* app system pid */
  {"name",    required_argument,  0, 'n'},  /* app executable name */
  {0, 0, 0, 0}
};
#endif

const char *routine_name="uninitialized";
const char *routine_where="uninitialized";
const char *routine_name0="uninitialized";
const char *routine_where0="uninitialized";
const char *routine_name1="uninitialized";
const char *routine_where1="uninitialized";

static int Dbglevel=0;
#if 0
static int Global_spid;
static int got_spid=0;
static int got_pname=0;
#endif
static char* pname=NULL;
static char* pid_str=NULL;

static int src_nid, src_ppid, src_ptl;

/* timeout on getting an initial gdb prompt */
static double gdb_timeout=30.0;
static double gt0;

/* process id for gdb */
static int gpid=0;   

/* pipes for redirecting gdb io */ 
static int fd_w_gdb[2], fd_r_gdb[2];

/* holds a line of output from gdb */
static char line[GDB_LN_SIZE]; 

/* holds collected data from lines of gdb out */
static char cout[GDB_TOTAL_OUT_SIZE];

#if 0
/* gdb commands and their lengths */
static const char* gdbcmd[4] = { "cont\n", "where\n", "cont\n", "quit\n" };
static const int gdbcmdlen[4] = { 5, 6, 5, 5 };
#endif

/* gdb state vars */
static int app_faulted=0, debug_w_gdb=0, gdbcmd_ind=0, save_gdb_output=0; 
static int gdb_count=0;
enum{ CONT1, BACKTRACE, CONT2, QUIT };
enum{ GDB_ERR, GDB_PROMPT, GDB_PROG_RUNNING, INCOMPLETE_LN, GDB_OTHER };

static int gdb_result=0, status;
static char* argv_gdb[6];
static const char* gdb_binary = "/usr/bin/gdb";

static char* bt;
static int btsize;

static int get_args(int argc, char *argv[]);

static void handler(int sig); 

static int init_listen(void);
static int cgdb_request_check(control_msg_handle *mh);
static int free_cgdb_message(control_msg_handle *mh);
static int process_cgdb_req(control_msg_handle *cgdb_req);
static int wait_cgdb_put_msg(int bufnum, int tmout, int howmany);
static int gdb_interactive(void);
static int get_gdb_prompt(void);
static void close_gdb(void); 
static int start_gdb(void);
static int notify_cgdb(void);

static int gdb_pipes(void);
#if 0
static void itoa(int n, char s[]);
#endif
static int getch(int fd);
static int readgdb2(int fd, int* inpos, char line[], int sz);

int
main(int argc, char *argv[])
{
  control_msg_handle cgdb_req;
  int rc;
  int argcm;

    log_open("PORTALS WRAPPER FOR GDB");

    LOCATION("main","top");

    LOCATION("get_args","top");

    if (argc == 3) {
	rc = get_args(argc, argv);
    }
    else {
      argcm = argc-1;
      log_msg("gdbwrap: wrong no. cmd line args");
      log_msg("gdbwrap: expected 2, got %d", argcm);
      exit(-1);
    }

    _my_ppid = register_ppid(&_my_taskInfo, PPID_GDBWRAP, GID_GDBWRAP, "wrap");

    if (_my_ppid != PPID_GDBWRAP){
      log_msg("gdbwrap: Can not register myself with PPID_GDBWRAP...");
      exit(-1);
    }
    rc = server_library_init();

    if (rc){
        log_error("initializing server library");
    }


    signal(SIGTERM, handler);

    LOCATION("init_listen","");
    rc = init_listen();

    if (rc < 0) {
      log_msg("gdbwrap: initializing listen");
      exit(-1);
    }

    debug_w_gdb++;

    while(debug_w_gdb > 0) {

        LOCATION("cgdb_request_check","");
        rc = cgdb_request_check(&cgdb_req);

        if (rc < 0) {
          log_msg("gdbwrap: checking for cgdb requests");
          exit(-1);
        }
        if (rc == 1) {
          
          LOCATION("process_cgdb_req","");
          rc = process_cgdb_req(&cgdb_req);

          if (rc < 0) {
            log_msg("gdbwrap: processing cgdb request");
            exit(-1);
          }
          free_cgdb_message(&cgdb_req);
        }

        usleep(200000);
        //sched_yield();    
    }
    return 0;
}

static int get_args(int argc, char *argv[])
{
  LOCATION("get_args","top");

  (void) argc;
  pname = argv[1];
  pid_str = argv[2];

  return 0;
}


static int init_listen(void)
{
    int rc;

    CLEAR_ERR;

    /*
    ** Set up portals required for CGDB communication
    */
    rc = srvr_init_control_ptl_at(10,GDBWRAP_CGDB_PTL);
    if (rc) {
      log_msg("init_listen: init cgdb ctl portal");
      return -1;
    }

    return 0;
}


static int cgdb_request_check(control_msg_handle *mh)
{
    int rc;
    
    CLEAR_ERR;

    SRVR_CLEAR_HANDLE(*mh);

    rc = srvr_get_next_control_msg(GDBWRAP_CGDB_PTL, mh, NULL, NULL, NULL);

    return rc;
}


static int free_cgdb_message(control_msg_handle *mh)
{
    int rc;

    CLEAR_ERR;

    rc = srvr_free_control_msg(GDBWRAP_CGDB_PTL, mh);

    return rc;
}


static int process_cgdb_req(control_msg_handle *cgdb_req)
{
  int rc, wrc, fail, succeed;
  int status, msg_type;
  char *user_data;
  int tmout, nsent, bufnum;  /* no of secs to timeout on put request */
  static int count=0;

  int ch, cmd_len, i;

  char no_gdb[] = "ERROR(pct): gdb is not running (use yod w/ -bt option?)\n";
  int no_gdb_sz=strlen(no_gdb)+1;

  char cant_signal_gdb[] = "ERROR(pct): cant send SIGINT to gdb?!\n";
  int cant_signal_gdb_sz=strlen(cant_signal_gdb)+1;

  char get_bt_from_gdb[] = "ERROR(pct): trouble getting bt from gdb?!\n";
  int get_bt_from_gdb_sz=strlen(get_bt_from_gdb)+1;

  char* cgdb_err[25];
  int    cgdb_sz[25];

  cgdb_err[GDBWRAP_CGDB_NO_GDB] = no_gdb;
  cgdb_err[GDBWRAP_CGDB_CANT_SIGNAL_GDB] = cant_signal_gdb;
  cgdb_err[GDBWRAP_CGDB_GET_BT_FROM_GDB] = get_bt_from_gdb;

  cgdb_sz[GDBWRAP_CGDB_NO_GDB] = no_gdb_sz;
  cgdb_sz[GDBWRAP_CGDB_CANT_SIGNAL_GDB] = cant_signal_gdb_sz;
  cgdb_sz[GDBWRAP_CGDB_GET_BT_FROM_GDB] = get_bt_from_gdb_sz;

    CLEAR_ERR;

    LOCATION("process_cgdb_req", "top");

    bt = NULL;
    btsize=0;

    status = 0;   /* set non-zero only on pct fatal error */

    msg_type = SRVR_HANDLE_TYPE(*cgdb_req);
    src_nid  = SRVR_HANDLE_NID(*cgdb_req);
    src_ppid  = SRVR_HANDLE_PID(*cgdb_req);
    user_data = SRVR_HANDLE_USERDEF(*cgdb_req);
    src_ptl = CGDB_GDBWRAP_PTL;

    /* these CGDB messages are debug messages arriving 
       from cgdb...
    */

    switch (msg_type) {

      case CGDB_REQUEST_GDB:  /* cgdb wants to attach gdb to process */
        if (Dbglevel > 0) {
          log_msg("process_cgdb_req: CGDB_REQUEST_GDB...");
        }

        /* have pct supply spid and app executable name when it
           starts up gdb wrapper. i.e., we KNOW these things
           already, don't has to get them from cgdb...
        */

        fail = -1;

        rc = start_gdb();

        if (rc < 0) {
          fail = GDBWRAP_CGDB_GDB_START_FAILED;
          log_msg("process_cgdb_req: had problem starting gdb...");
          goto failcheck_start;
        }    

        if (debug_w_gdb == 0) {
          succeed = GDBWRAP_CGDB_GDB_DONE;
        }
        else {
          succeed = GDBWRAP_CGDB_SEND_BT;
        }

failcheck_start:
        if (fail >= 0) {

          bufnum = srvr_comm_put_req( cgdb_err[fail], cgdb_sz[fail], fail,
                                  (CHAR*) &cgdb_sz[fail], sizeof(int),
                                  1, &src_nid, &src_ppid, &src_ptl) ;
          if (Dbglevel > 0) {
            log_msg("process_cgdb_req: did srvr_comm_put_req() 0");
          }
          debug_w_gdb = 0;
        }
        else {

          bufnum = srvr_comm_put_req( bt, btsize, succeed,
                                    (CHAR*) &btsize, sizeof(int), 
                                    1, &src_nid, &src_ppid, &src_ptl);

          if (Dbglevel > 0) {
            log_msg("process_cgdb_req: did srvr_comm_put_req() 1");
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

      case CGDB_REQUEST_BT:    /* cgdb sends this to select pcts */

        if (Dbglevel > 0) {
          log_msg("process_cgdb_req: CGDB_REQUEST_BT...");
        }

/* may not need job id... */
#if 0
        job_id = *( (int*) user_data);
#endif

        if (Dbglevel > 0) {
          log_msg("process_cgdb_req: CDBG_REQUEST_BT from %d/%d (%d)",
                       src_nid, src_ppid, count++);
        }

        /* get back trace and send btsize back to cgdb in ctl portal
           or
           make cgdb_xxxx global and set a flag that cgdb is waiting
           for a bt. then send SIGINT to gdb here, and handle gdb
           prompt elsewhere...
        */

        fail = -1;

        /* make sure gdb is running, else why bother */
        if (Dbglevel > 0) {
          log_msg("process_cgdb_req: check for gdb...");
        }
        if (gpid == 0) {
          log_msg("process_cgdb_req: gdb not running...");

          fail = GDBWRAP_CGDB_NO_GDB;
          goto failcheck_cmd;

        }

#if 0
        log_msg("process_cgdb_req: send gdb signal...");
        /* try to send signal to gdb */
        if ( kill(gpid,SIGINT) < 0 ) {
          log_msg("process_cgdb_req: cant signal gdb...");

          fail = GDBWRAP_CGDB_CANT_SIGNAL_GDB;
          goto failcheck_cmd;
        }
#endif
        if (Dbglevel > 0) {
          log_msg("process_cgdb_req: cmd= %s", user_data); 
        }

        /* this "truncates" the command, if necessary */
        cmd_len = SRVR_USR_DATA_LEN;
        /* get the length of the command */
        for (i=0; i<SRVR_USR_DATA_LEN; i++) {
          ch = user_data[i];
          if (ch == '\n') {
            cmd_len = i+1;
            break;
          }
        }
        /* this is redundant unless user_data[] does not
           have a newline */
        user_data[cmd_len-1] = '\n';

        if (Dbglevel > 0) {
          log_msg("process_cgdb_req: give cmd to gdb...");
        }

        /* give cmd to gdb */
        write( fd_w_gdb[1], user_data, cmd_len );

        /* relay gdb output, up until a prompt, back to cgdb */
        rc = gdb_interactive();

#if 0
        if (rc < 0) {
          log_msg("process_cgdb_req: cant get bt from gdb...");

          fail = GDBWRAP_CGDB_GET_BT_FROM_GDB;
          goto failcheck_cmd;
        }
#endif

        /* both cases send a bt, but the first
           also notifies cgdb that we're all
           done... */
/* MARK */
        if (debug_w_gdb == 0) {
          succeed = GDBWRAP_CGDB_GDB_DONE;
        }
        else {
          succeed = GDBWRAP_CGDB_SEND_BT;
        }

failcheck_cmd:
        if (fail >= 0) {

          bufnum = srvr_comm_put_req( cgdb_err[fail], cgdb_sz[fail], fail,
                                  (CHAR*) &cgdb_sz[fail], sizeof(int) ,
                                  1, &src_nid, &src_ppid, &src_ptl );

          if (Dbglevel > 0) {
            log_msg("process_cgdb_req: did srvr_comm_put_req() 2");
          }
        }
        else {
          /* put bt in data ptl */
          if (Dbglevel > 0) {
            log_msg("process_cgdb_req: calling srvr_comm_put_req()");
          }

          if ( !btsize && bt ) { /* fake bt */
            bt[0] = ' ';
            btsize = 1;
          }  
          bufnum = srvr_comm_put_req( bt, btsize, succeed, 
                                  (CHAR*) &btsize, sizeof(int),
                                  1, &src_nid, &src_ppid, &src_ptl); 

          if (Dbglevel > 0) {
            log_msg("process_cgdb_req: did srvr_comm_put_req() 3");
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
        log_msg("process_cgdb_req: ignoring msg type %d on cgdb msg portal from %d/%d\n", msg_type, src_nid, src_ppid);

        status = 0;
        break;
    }

    return status;
}


/* basically wait_pcts_put_message() from yod */
static int wait_cgdb_put_msg(int bufnum, int tmout, int howmany)
{
  int rc;
  int count=0;

  while( count < tmout ) {
    rc = srvr_test_read_buf(bufnum, howmany);
    if (rc < 0) {
      log_msg("wait_cgdb_put_msg: srvr_test_read_buf failed:");
      log_msg("wait_cgdb_put_msg: bufnum=%d, tmout=%d, howmany=%d",
                                bufnum, tmout, howmany);
      return -1;
    }
    if (rc == 1) return 0;

    sleep(1);
    count++;
  }
  log_msg("wait_cgdb_put_msg: timed out after %d seconds", tmout);
  return -1;
}


static int gdb_interactive(void) 
{
  int i, gpos=0;

#if 0
  if (gdb_result == EOF) {
    gdb_result = 0;
    return -1;
  }
#endif

  while(1) {

  if ( (gdb_result = readgdb2(fd_r_gdb[0], &gpos, line, GDB_LN_SIZE)) != EOF ) {

    if ( gdb_result != INCOMPLETE_LN ) { /* else we read empty pipe; 
                                            try later */
      line[gpos] = 0;
      
      if (Dbglevel > 0) {
        log_msg("line= %s", line);
      }

      /* handle prompt (return) */

      if ( gdb_result == GDB_PROMPT ) {
        
        if (Dbglevel > 0) {
          log_msg("gdb_interactive: got GDB_PROMPT....");
        }
        for (i=0; i<gpos; i++) {
          cout[gdb_count++] = line[i];
        }
        btsize = gdb_count;
        bt = cout;
        gdb_count = 0;
        return 0;
      }
      else {
        /* line was not a gdb prompt */

        if ( gdb_result == GDB_OTHER ) {   /* good stuff */
          if (Dbglevel > 0) {
            log_msg("gdb_interactive: got GDB_OTHER...");
          }
          for (i=0; i<gpos; i++) {
            cout[gdb_count++] = line[i];
          }
        }
        /* bad stuff */
        else {
          log_msg("gdb_interactive: something bad...");
        }
      }
    }
  }
  else {  /* save back trace size, wait for gdb 
             to exit, and reset flags */
    if (Dbglevel > 0) {
      log_msg("gdb_interactive: out of while...");
    }

    btsize = gdb_count;
    bt = cout;
    gdb_count = 0;

    if (Dbglevel > 0) {
      log_msg("gdb_interactive: got EOF...");
    }
    close_gdb();
    break;
  }
} /* while */
  return 0;
}


static int get_gdb_prompt(void)
{
  int gpos=0;
  int i;

  if (Dbglevel > 0) {
    log_msg("get_gdb_prompt: top");
  }

  /* read gdb output until we get the first prompt,
     and then break out...
  */

  gt0 = dclock();
  while ( (gdb_result = readgdb2(fd_r_gdb[0], &gpos, line, GDB_LN_SIZE)) != EOF ) {
    /* only wait so long for prompt */
    if ( (dclock() - gt0) > gdb_timeout ) {

      log_msg("go_app_host: timeout on initial gdb prompt");
      gdb_result = EOF;
      break;
    }

    if ( gdb_result != INCOMPLETE_LN ) {

      if ( gdb_result == GDB_PROMPT ) { 
        for (i=0; i<gpos; i++) {
          cout[gdb_count++] = line[i];
        }
        btsize = gdb_count;
        bt = cout;
        gdb_count = 0;
        break;             /* out of while */
      }
      else {               /* line was not a prompt*/
        if ( gdb_result == GDB_OTHER ) {
          for (i=0; i<gpos; i++) {
            cout[gdb_count++] = line[i];
          }
        }
        else {
#if 0
          line[pos] = 0;
          printf("pct: %s", line);
#endif
        }
      }
    }
  }
  if ( gdb_result == EOF ) { /* probably means gdb died -- normally
                                we get here via the above "break" */
    if (Dbglevel > 0) {
      log_msg("go_app_host: gdb pipe hit unexpected EOF");
    }
    debug_w_gdb = 0;
    return -1;
  }
  return 0;
}


static void close_gdb(void) 
{
   waitpid(gpid, &status, 0);
   gpid = 0;
   gdb_count = 0; 
   gdbcmd_ind = 0;
   gdb_result = 0;
   debug_w_gdb = 0;
   save_gdb_output = 0;
   app_faulted = 0;
   close(fd_w_gdb[1]);
   close(fd_r_gdb[0]);
}

/* open some pipes to use in redirecting gdb's io */
static int gdb_pipes()
{
  if ( pipe(fd_w_gdb) < 0 ) {
    log_msg("go_app_host: in gdb_pipes(), couldn't create first pipe");
    return -1;
  }

  if ( pipe(fd_r_gdb) < 0 ) {
    log_msg("go_app_host: in gdb_pipes(), couldn't create second pipe");
    return -1;
  }

  return 0;
}


#if 0
/* integer to ascii */
static void itoa(int n, char s[])
{
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
#endif

/* do character input from arbitrary file descriptor */
static int getch(int fd) 
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
      INCOMPLETE_LN         for return, but input incomplete
      GDB_OTHER             for other non-EOF
   return length of string read in *pos
   sz is the size of line[]
*/
static int readgdb2(int fd, int* inpos, char line[], int sz) 
{
  static int pos=0;
  static int did_prompt_check=0;
  static int did_long_prompt_check=0;
  int ch, result=0, starting=1, num_zeros=0;

  /* the "prompt" strings", i.e., ones w/o newlines */ 
  const char* gdb_prompt_str = "(gdb) ";
  const char* gdb_running = "The program is running";
  /* "...Quit anyway (and kill it)? (y or n) " */

  if ( sz <= (int) strlen(gdb_prompt_str) ) {
    return GDB_ERR;
  } 

  /* the gdb prompt requires special handling since it
     doesn't supply a newline -- that's why we check 
     pos < len(gdb_prompt)
  */

  while ( (pos < (int) strlen(gdb_prompt_str) || did_prompt_check) && 
          (pos < (int) strlen(gdb_running) || did_long_prompt_check) 
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
  if ( pos == (int) strlen(gdb_prompt_str) ) {
    if ( !strncmp(gdb_prompt_str, line, strlen(gdb_prompt_str)) ) {
     *inpos = pos;
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
  if ( pos == (int) strlen(gdb_running) ) {
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

    result = GDB_OTHER;

    /* add 1 to pos to make it a length rather 
       than a position... 
    */
   *inpos = pos+1;
    pos = 0;
    did_prompt_check=0;
    return result; 
  }
}


static int start_gdb(void) 
{
  int rc;

  rc = gdb_pipes();

  if (rc < 0) {
    log_msg("start_gdb: could not open gdb pipes");
    return -1;
  }

  if ( access(gdb_binary, F_OK) < 0 ) {
    log_msg("start_gdb: could not find gdb binary:");
    log_msg(gdb_binary);
    return -1;
  }

  if ( access(gdb_binary, X_OK) < 0 ) {
    log_msg("start_gdb: do not have execute permission for gdb binary:");
    log_msg(gdb_binary);
    return -1;
  }

  /*
  ** before sending app along to user code, attach
  ** to it with debugger
  */

  if ( (gpid = fork()) < 0 ) {
    log_msg("start_gdb: problem w/ fork for gdb");
    return -1;
  }

  if ( gpid > 0 ) {  /* parent -- get gdb prompt */

    debug_w_gdb++;

    /* write to fd_w_gdb[1] */
    close(fd_w_gdb[0]);
   
    /* read from fd_r_gdb[0] */
    close(fd_r_gdb[1]);

    LOCATION("start_gdb", "about to get gdb prompt");

    /* set flag on pipe we read gdb stuff from so that read()s
       will be non-blocking -- we want to avoid the following
       situation: some other process faults and the local process
       hangs waiting on a result from that process, thus hanging
       gdb, thus hanging the wrapper in readgdb2(). instead, return
       immediately from readgdb2() in case read()ing a char got
       nothing (and we know it's not EOF -- see getch()) 
    */
    fcntl(fd_r_gdb[0], F_SETFL, O_NONBLOCK);

    if (Dbglevel > 0) {
      log_msg("start_gdb: calling get_gdb_prompt");
    }

    if ( get_gdb_prompt() < 0 ) {
      log_msg("start_gdb: get_gdb_prompt failed");
      return -1;
    }
  }
  else {              /* child -- exec gdb */

#if 0
    itoa(Global_spid, pid_str);   /* pid to ascii for execv */
#endif

    /* child reads from this pipe -- parent writes */
    close(fd_w_gdb[1]); 

    /* child writes to this pipe  -- parent reads */
    close(fd_r_gdb[0]); 

    /* redirect child's (gdb's) io to parent (gdb wrapper) */
    dup2( fd_r_gdb[1], STDOUT_FILENO );
    dup2( fd_w_gdb[0], STDIN_FILENO  );

    if (Dbglevel > 0) {
      log_msg("start_gdb: pname = %s", pname);
      log_msg("start_gdb: pid_str = %s", pid_str);
    }

    argv_gdb[0] = strdup("gdb");
    argv_gdb[1] = pname;
    argv_gdb[2] = pid_str;
    argv_gdb[3] = 0;
    if ( execv(gdb_binary, argv_gdb) < 0 ) {
      /* what should be really do? -- note, before forking
         we test executableness of this file... 
      */
      //appStatus.rc = LAUNCH_ERR_???_GDB; 
      log_msg("start_gdb: execv failed...");
      return -1;
    }
    free(argv_gdb[0]);
  }
  return 0;
}


/* do this so that the atexit-registered fns get called;
   in particular, we want the portals task table entry
   for gdb proxy to get cleared out...
*/
static void handler(int sig) 
{
/* wait until we have a comprehensive portals cleanup
   mechanism before doing the following */
#if 1
    notify_cgdb();
#endif
    (void) sig;
    exit(0);
}


/* tell cgdb we're leaving */
static int notify_cgdb(void) {

  int wrc;
  int succeed = GDBWRAP_CGDB_GDB_DONE;

  int tmout, nsent, bufnum;  /* no of secs to timeout on put request */

  bt = NULL;
  btsize = 0;

  bufnum = srvr_comm_put_req( bt, btsize, succeed,
          (CHAR*) &btsize, sizeof(int),
          1, &src_nid, &src_ppid, &src_ptl); 

  if (Dbglevel > 0) {
    log_msg("notify_cgdb: did srvr_comm_put_req() 4");
  }

  tmout=10;
  nsent = 1;
  wrc = wait_cgdb_put_msg(bufnum, tmout, nsent);
  if (Dbglevel > 0) {
    log_msg("notify_cgdb: did wait_cgdb_put_msg()");
  }

  srvr_delete_buf(bufnum);
  if (Dbglevel > 0) {
    log_msg("notify_cgdb: did srvr_delete_buf()");
  }

  return wrc;
}

