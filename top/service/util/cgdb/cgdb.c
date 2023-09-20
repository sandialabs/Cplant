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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <signal.h>
#include "srvr_comm.h"
#include "sys_limits.h"
#include "portal_assignments.h"
#include "ppid.h"
#include "pct_ports.h"
#include "cgdb.h"
#include <getopt.h>

extern char* optarg;

#define DEBUG 0

static struct option opts[] =
{
  {"nid",      required_argument,     0,   'n'},  /* node id */ 
  {"help",     optional_argument,     0,   'h'},  /* usage */ 
  {0, 0, 0, 0}
};

static control_msg_handle gdbwrap_mh;
static control_msg_handle pct_mh;

static void show_usage(void);
static INT32 init_gdbwrap_ptl(void);
static INT32 init_pct_ptl(void);
static INT32 request_bt(INT32 nid, INT32 pid, char* cmd);
static INT32 request_proxy(INT32 nid, INT32 pid);
static INT32 request_gdb(INT32 nid, INT32 pid);
static int get_opts(int argc, char* argv[]);
static INT32 get_gdbwrap_ctl_msg(INT32* mtype, CHAR** user_data, int* snid, int* spid);
static INT32 get_pct_ctl_msg(INT32* mtype, CHAR** user_data, int* snid, int* spid);
static int filter(void);

static int gdb_done = 0;
static int Gnid=-1;

static  INT32 mtype;
static  CHAR* mdata;
static  char bt[64*1024];                /* bt string */
static int bt_size; 
static  int snid, spid;
static  char cmd[SRVR_USR_DATA_LEN];

int main(int argc, char* argv[])
{
  INT32 rc;
  int index, next_char;

  ppid_type x;

  rc = get_opts(argc, argv);

  if (rc < 0) {
    exit(-1);
  }

#ifdef DEBUG
  printf("requesting bt from node %d\n", Gnid);
#endif

  x = register_ppid(&_my_taskInfo, PPID_AUTO, GID_CGDB, "cgdb");

  if ( x == 0 ) {
    fprintf(stderr, "main: cannot register myself with register_ppid()\n");
    exit(-1);
  }
  else {
    _my_ppid = x;
#if DEBUG
    printf("_my_ppid= %d\n", _my_ppid);
#endif
  }
  rc = server_library_init();

  if (rc){
      fprintf(stderr,"initializing server library");
      exit(-1);
  }

#if DEBUG
  printf("requesting debugging on node %d\n", Gnid);
#endif

  rc = init_gdbwrap_ptl();
  if (rc < 0) {
    fprintf(stderr, "main: could not set up gdbwrap ctl portal\n");
    exit(-1);
  }

  rc = init_pct_ptl();
  if (rc < 0) {
    fprintf(stderr, "main: could not set up pct ctl portal\n");
    exit(-1);
  }

  fprintf(stdout, "cgdb requesting start up of GDB proxy on node %d\n", Gnid);
  rc = request_proxy(Gnid, PPID_PCT);

  if (rc < 0) {
    fprintf(stderr, "Exiting...\n");
    exit(-1);
  }

  //while(1);

  fprintf(stdout, "cgdb requesting start up of GDB on node %d\n", Gnid);
  fprintf(stdout, "waiting for gdb prompt...\n");
  rc = request_gdb(Gnid, PPID_GDBWRAP);

  if (rc < 0) {
    fprintf(stderr, "Exiting...\n");
    exit(-1);
  }

  while(gdb_done == 0) {

    /* get cmd from stdin */
    for (index=0; index<SRVR_USR_DATA_LEN; index++) {
      next_char = getchar();
      cmd[index] = (char) next_char;
      if ( next_char == '\n' ) {
        break;
      }
    }
    cmd[SRVR_USR_DATA_LEN-1] = '\n';

    if ( filter() ) {
      continue;
    }

#if DEBUG
    printf("----------------------------------------------------\n");
#endif
    rc = request_bt(Gnid, PPID_GDBWRAP, cmd);
    if (rc < 0) {
      fprintf(stderr, "main: request_bt() fails, exiting\n");
      exit(-1);
    }
#if DEBUG
    printf("----------------------------------------------------\n");

    printf("main: polling for gdbwrap put requests...\n");
#endif

#if 0
    tmout = 30;
    count = 0;
    while( count < tmout ) {
      rc = get_gdbwrap_ctl_msg(&mtype, &mdata, &snid, &spid);
      if (rc == 1) {
        break;
      }
      sleep(1);
      count++;
    }
#else
    while( 1 ) {
      rc = get_gdbwrap_ctl_msg(&mtype, &mdata, &snid, &spid);
      if (rc == 1) {
        break;
      }
      sleep(1);
    }
#endif

    if (rc != 1) {
      fprintf(stderr, "main: looks like we timed out on getting a ctl msg\n");
      fprintf(stderr, "main: from gdbwrap; exiting...\n");
      exit(-1);
    }

#if DEBUG
    printf("got msg from gdbwrap (%d,%d) of type...", snid, spid);
#endif

    switch (mtype) {

      case GDBWRAP_CGDB_SEND_BT:
#if DEBUG
        printf("GDBWRAP_SEND_BT\n");
        printf("bt_size from gdbwrap's put request= %d\n", *((int*) mdata));
#endif
        bt_size = *(int*) mdata;
        break;

      case GDBWRAP_CGDB_NO_GDB:
#if DEBUG
        printf("GDBWRAP_CGDB_NO_GDB\n");
        printf("error msg sz from gdbwrap's put request= %d\n", *((int*) mdata));
#endif
        bt_size = *(int*) mdata;
        break;

      case GDBWRAP_CGDB_CANT_SIGNAL_GDB:
#if DEBUG
        printf("GDBWRAP_CGDB_CANT_SIGNAL_GDB\n");
        printf("error msg sz from gdbwrap's put request= %d\n", *((int*) mdata));
#endif
        bt_size = *(int*) mdata;
        break;

       case GDBWRAP_CGDB_GET_BT_FROM_GDB:
#if DEBUG
        printf("GDBWRAP_CGDB_GET_BT_FROM_GDB\n");
#endif
        exit(-1);
        break;

      case GDBWRAP_CGDB_GDB_DONE:
#if DEBUG
        printf("GDBWRAP_CGDB_GDB_DONE\n");
        printf("bt_size from gdbwrap's put request= %d\n", *((int*) mdata));
#endif
        bt_size = *(int*) mdata;
        gdb_done = 1;
        break;

      default:
#if DEBUG
        printf("UNKNOWN ERROR TYPE\n");
#endif
        exit(-1);
        break;
    }

    rc = srvr_comm_put_reply(&gdbwrap_mh, bt, bt_size);

    bt[bt_size] = 0;

    printf("------------------------------------------------\n");
    //printf("Reply to bt request from node %d...\n", snid);
    printf("%s", bt);
    //printf("------------------------------------------------\n");

  }

  return 0;
}


INT32 init_gdbwrap_ptl(void) 
{
  int nprocs, rc;

  nprocs = MAX_PROC_PER_GROUP;

  rc = srvr_init_control_ptl_at(nprocs, CGDB_GDBWRAP_PTL);

  if (rc) {
    printf("init_gdbwrap_ptl: problem setting up gdbwrap portal.\n");
    return -1;
  }

  return 0;
}


INT32 init_pct_ptl(void) 
{
  int nprocs, rc;

  nprocs = MAX_PROC_PER_GROUP;

  rc = srvr_init_control_ptl_at(nprocs, CGDB_PCT_PTL);

  if (rc) {
    printf("init_pct_ptl: problem setting up pct portal.\n");
    return -1;
  }

  return 0;
}


INT32 request_proxy(INT32 nid, INT32 pid)
{
  int rc;
  int tmout, count;

#if DEBUG
  printf("request_proxy: sending to nid= %d, pid= %d\n", nid, pid);
#endif

  rc = srvr_send_to_control_ptl(nid, pid, PCT_CGDB_PTL, CGDB_REQUEST_PROXY, (CHAR*) NULL, 0);

  if (rc < 0) {
    fprintf(stderr, "request_proxy: Error sending start request to pct nid,pid= %d,%d\n", nid, pid);
    return -1;
  }

  tmout = 15;
  count = 0;
  while( count < tmout ) {
    rc = get_pct_ctl_msg(&mtype, &mdata, &snid, &spid);
    if (rc == 1) {
      break;
    }
    sleep(1);
    count++;
  }

  if (rc != 1) {
    fprintf(stderr, "request_proxy: We timed out on getting a proxy start up reply from the pct on %d.\n", Gnid);
    return -1;
  }

#if debug
  printf("request_proxy: got start up reply from pct (%d,%d) of type...", snid, spid);
#endif

  switch (mtype) {

    case PCT_CGDB_START_FAILED:
      fprintf(stderr, "request_proxy: The pct was not able to start PROXY on node %d\n", Gnid);
      bt_size = *(int*) mdata;

      /* complete the protocol, else pct exits */

      rc = srvr_comm_put_reply(&pct_mh, bt, bt_size);
      bt[bt_size] = 0;
#if DEBUG
      printf("------------------------------------------------\n");
      printf("%s", bt);
#endif
      return -1;
      break;

    case PCT_CGDB_START_OK:
#if DEBUG
      printf("request_proxy: GDBWRAP_SEND_BT\n");
      printf("request_proxy: sok_size from pcts's put request= %d\n", *((int*) mdata));
#endif
      bt_size = *(int*) mdata;

      rc = srvr_comm_put_reply(&pct_mh, bt, bt_size);
      bt[bt_size] = 0;
#if DEBUG
      printf("------------------------------------------------\n");
      printf("%s", bt);
#endif

      break;

      default:
#if DEBUG
        printf("request_proxy: UNKNOWN ERROR TYPE\n");
#endif
        return -1;
        break;
    }
    return 0;
}

INT32 request_gdb(INT32 nid, INT32 pid)
{
  int rc;
  int tmout, count;

#if DEBUG
  printf("request_gdb: sending to nid= %d, pid= %d\n", nid, pid);
#endif

  rc = srvr_send_to_control_ptl(nid, pid, GDBWRAP_CGDB_PTL, CGDB_REQUEST_GDB, (CHAR*) NULL, 0);

  if (rc < 0) {
    fprintf(stderr, "request_gdb: Error sending start request to GDB wrapper at nid,pid= %d,%d\n", nid, pid);
    return -1;
  }

  tmout = 15;
  count = 0;
  while( count < tmout ) {
    rc = get_gdbwrap_ctl_msg(&mtype, &mdata, &snid, &spid);
    if (rc == 1) {
      break;
    }
    sleep(1);
    count++;
  }

  if (rc != 1) {
    fprintf(stderr, "request_gdb: We timed out on getting a prompt from a GDB proxy on node %d.\n", Gnid);
    fprintf(stderr, "request_gdb: Was a process started on that node using 'yod -attach'?\n");
    return -1;
  }

#if DEBUG
  printf("request_gdb: got msg from gdbwrap (%d,%d) of type...", snid, spid);
#endif

  switch (mtype) {

    case GDBWRAP_CGDB_GDB_START_FAILED:
      fprintf(stderr, "request_gdb: The GDB proxy was not able to start GDB on node %d\n", Gnid);
      return -1;
      break;

    case GDBWRAP_CGDB_SEND_BT:
#if DEBUG
      printf("request_gdb: GDBWRAP_SEND_BT\n");
      printf("request_gdb: bt_size from gdbwrap's put request= %d\n", *((int*) mdata));
#endif
      bt_size = *(int*) mdata;
      break;

      default:
#if DEBUG
        printf("request_gdb: UNKNOWN ERROR TYPE\n");
#endif
        return -1;
        break;
    }

    rc = srvr_comm_put_reply(&gdbwrap_mh, bt, bt_size);

    bt[bt_size] = 0;

    printf("------------------------------------------------\n");
    //printf("Reply to bt request from node %d...\n", snid);
    printf("%s", bt);

    return 0;
}


INT32 request_bt(INT32 nid, INT32 pid, char* cmd)
{
  int rc;

#if DEBUG
  printf("request_bt: sending to nid= %d, pid= %d\n", nid, pid);
#endif

  rc = srvr_send_to_control_ptl(nid, pid, GDBWRAP_CGDB_PTL, CGDB_REQUEST_BT, (CHAR*) cmd, SRVR_USR_DATA_LEN);

#if DEBUG
  printf("request_bt: return from srvr_send_to_control_ptl= %d\n", rc);
#endif

  if (rc) {
    fprintf(stderr, "request_bt: error sending bt request to gdbwrap at nid,pid= %d,%d\n", nid, pid);
    return -1;
  }
  return 0;
}


/* this is from get_gdbwrap_control_message() in yod_comm.c w/ minor changes */
INT32 get_gdbwrap_ctl_msg(INT32* mtype, CHAR** user_data, int* snid, int* spid)
{
INT32 rc, src_nid, src_pid;

  if (SRVR_IS_VALID_HANDLE(gdbwrap_mh)) {
    srvr_free_control_msg(CGDB_GDBWRAP_PTL, &gdbwrap_mh); /* free the previous one */
  }

  SRVR_CLEAR_HANDLE(gdbwrap_mh);

  rc = srvr_get_next_control_msg(CGDB_GDBWRAP_PTL, &gdbwrap_mh, mtype, NULL, user_data);

  if (rc == 1) {
    src_nid = SRVR_HANDLE_NID(gdbwrap_mh);
    src_pid = SRVR_HANDLE_PID(gdbwrap_mh);

#if DEBUG
    printf("get_gdbwrap_ctl_msg: got ctl message from nid,pid= %d,%d\n", src_nid, src_pid);
#endif
  }

 *snid = src_nid;
 *spid = src_pid;
  return rc;    /* 1 - there's a message, 0 - no message */
}


INT32 get_pct_ctl_msg(INT32* mtype, CHAR** user_data, int* snid, int* spid)
{
INT32 rc, src_nid, src_pid;

  if (SRVR_IS_VALID_HANDLE(pct_mh)) {
    srvr_free_control_msg(CGDB_PCT_PTL, &pct_mh); /* free the previous one */
  }

  SRVR_CLEAR_HANDLE(pct_mh);

  rc = srvr_get_next_control_msg(CGDB_PCT_PTL, &pct_mh, mtype, NULL, user_data);

  if (rc == 1) {
    src_nid = SRVR_HANDLE_NID(pct_mh);
    src_pid = SRVR_HANDLE_PID(pct_mh);

#if DEBUG
    printf("get_pct_ctl_msg: got ctl message from nid,pid= %d,%d\n", src_nid, src_pid);
#endif
  }

 *snid = src_nid;
 *spid = src_pid;
  return rc;    /* 1 - there's a message, 0 - no message */
}


int get_opts(int argc, char* argv[]) 
{
  int index, ch=0;

  while (ch != EOF) {
    ch = getopt_long_only(argc, argv, "", opts, &index);

    switch(ch) {
      case 'n':
        Gnid = strtol(optarg, (char**)NULL, 10);
        break;

      case 'h':
        show_usage();
        exit(0);
        break;

      default:
        break;
    }
  }

  /* see that we got a node id */
  if ( Gnid == -1 ) {
    show_usage();
    return -1;
  }
  return 0;
}


void show_usage(void)
{
  printf("cgdb is used to attach to a running cplant process. it is\n");
  printf("commonly used in conjunction with the -at option to yod,\n");
  printf("which pauses the job before main() and advertises node ids\n");
  printf("at that point one can use cgdb to attach gdb to a process\n");
  printf("on a node...\n\n");
  printf("cgdb usage:\n");
  printf("  -n node_id   --  the single (REQUIRED) argument, node_id,\n");
  printf("               --  is the phyiscal node where gdb should be\n");
  printf("               --  started.\n");
  printf("               --  node ids and their relation to process rank\n");
  printf("               --  can be obtained as output from 'yod -attach'\n");
  printf("               --  or from pingd.\n");
  printf("  -h, -help    -- this stuff (optional)\n\n");
  printf("Cgdb is mainly a wrapper/interface to the GNU debugger. Starting\n");
  printf("cgdb results in an instance of gdb being attached to a running\n");
  printf("Cplant job process. Cgdb then acts as a proxy for feeding commands\n");
  printf("to gdb and for displaying its output.\n\n");
  printf("It is not possible to pass gdb command line arguments thru cgdb;\n");
  printf("one is limited to feeding commands directly to gdb. Of course,\n");
  printf("some gdb commands do not make sense in this environment. For example,\n");
  printf("the \"run\" command cannot be used. Other commands like \"dir\"\n");
  printf("(if the user's source files are available on compute nodes), may\n");
  printf("be useful.\n"); 
  printf("The set of gdb commands can be found online at\n");
  printf("www.gnu.org/manual.\n");
}

int filter(void) 
{
  int i;

  for (i=0; i<SRVR_USR_DATA_LEN-2; i++) {
     if ( cmd[i] == 'r' && cmd[i+1] == 'u' && cmd[i+2] == 'n') {
       printf("Not a viable command in the Cplant environment.\n");
       return 1;
     }
  }
  return 0;
}

