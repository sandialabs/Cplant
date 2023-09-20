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

#include "srvr_comm.h"
#include "sys_limits.h"
#include "portal_assignments.h"
#include "ppid.h"
#include "pct_ports.h"
#include "bt.h"
#include <getopt.h>

extern char* optarg;

#define DEBUG 0

static struct option opts[] =
{
  {"nid",              required_argument,         0,   'n'},  /* node id */ 
  {"jid",              required_argument,         0,   'j'},  /* job id */ 
  {"help",             no_argument      ,         0,   'h'},  /* help */ 
  {0, 0, 0, 0}
};

static control_msg_handle pct_mh;

static INT32 init_pct_ptl(void);
static INT32 send_req_2_pct(INT32 nid, INT32 pid, INT32 jid);
static void usage(void);
static int parse_cmd_line(int argc, char* argv[]);
static INT32 get_pct_ctl_msg(INT32* mtype, CHAR** user_data, int* snid, int* spid);

static int pids[MAX_PROC_PER_GROUP];
static int nids[MAX_PROC_PER_GROUP];
static int nid_index;
static int pid_index;
static int jid;

int main(int argc, char* argv[])
{
  INT32 rc;
  INT32 mtype;
  CHAR* mdata;
  char bt[MAX_PROC_PER_GROUP][1000];         /* slots for bt strings */
  int bt_size[MAX_PROC_PER_GROUP]; 
  int nnodes=0;
  int snid, spid;

  ppid_type x;

  int nodei, tmout, count;


nnodes = parse_cmd_line(argc, argv);

#if DEBUG
printf("requesting bt from %d nodes...\n", nnodes);
#endif

  x = register_ppid(&_my_taskInfo, PPID_AUTO, GID_BT, "bt");

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
        fprintf(stderr,"initializing server library\n");
        exit(-1);
    }

#if DEBUG
  printf("requesting bt from (nid,pid)= (%d,%d)\n", nids[0], pids[0]);
#endif

  rc = init_pct_ptl();
  if (rc < 0) {
    fprintf(stderr, "main: could not set up pct ctl portal\n");
    exit(-1);
  }


#if DEBUG
  printf("----------------------------------------------------\n");
#endif
  for (nodei=0; nodei<nnodes; nodei++) {

    rc = send_req_2_pct(nids[nodei], pids[nodei], jid);
    if (rc < 0) {
      fprintf(stderr, "main: send_req_2_pct() failed, bt exiting..\n");
      exit(-1);
    }
  }
#if DEBUG
  printf("----------------------------------------------------\n");
#endif

  for (nodei=0; nodei<nnodes; nodei++) {

#if DEBUG
    printf("main: polling for pct put requests...\n");
#endif

    tmout = 30;
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
      fprintf(stderr, "main: looks like we timed out on getting a ctl msg\n");
      fprintf(stderr, "main: from the pcts; exiting...\n");
      exit(-1);
    }

#if DEBUG
    printf("got msg from pct (%d,%d) of type...", snid, spid);
#endif

    switch (mtype) {

      case PCT_BT_SEND_BT:
#if DEBUG
        printf("PCT_SEND_BT\n");
        printf("bt_size from pct's put request= %d\n", *((int*) mdata));
#endif
        bt_size[nodei] = *(int*) mdata;
        break;

      case PCT_BT_BAD_JID:
#if DEBUG
        printf("PCT_BT_BAD_JID\n");
        printf("error msg sz from pct's put request= %d\n", *((int*) mdata));
#endif
        bt_size[nodei] = *(int*) mdata;
        break;

      case PCT_BT_NO_GDB:
#if DEBUG
        printf("PCT_BT_NO_GDB\n");
        printf("error msg sz from pct's put request= %d\n", *((int*) mdata));
#endif
        bt_size[nodei] = *(int*) mdata;
        break;

      case PCT_BT_CANT_SIGNAL_GDB:
#if DEBUG
        printf("PCT_BT_CANT_SIGNAL_GDB\n");
        printf("error msg sz from pct's put request= %d\n", *((int*) mdata));
#endif
        bt_size[nodei] = *(int*) mdata;
        break;

      default:
#if DEBUG
        printf("UNKNOWN ERROR TYPE\n");
#endif
        exit(-1);
        break;
    }

    rc = srvr_comm_put_reply(&pct_mh, bt[nodei], bt_size[nodei]);

    bt[nodei][bt_size[nodei]] = 0;

    printf("------------------------------------------------\n");
    printf("Reply to bt request from node %d...\n", snid);
    printf("%s", bt[nodei]);
    printf("------------------------------------------------\n");
  }
  printf("---------------------------------------------------------------------\n");

  return 0;
}


INT32 init_pct_ptl(void) 
{
  int nprocs, rc;

  nprocs = MAX_PROC_PER_GROUP;

  rc = srvr_init_control_ptl_at(nprocs, BT_PCT_PTL);

  if (rc) {
    printf("init_pct_ptl: problem setting up pct portal.\n");
    return -1;
  }

  return 0;
}


INT32 send_req_2_pct(INT32 nid, INT32 pid, INT32 jid)
{
  int rc;

#if DEBUG
  printf("send_req_2_pct: sending to nid= %d, pid= %d\n", nid, pid);
#endif

  rc = srvr_send_to_control_ptl(nid, pid, PCT_BT_PTL, BT_REQUEST_BT, (CHAR*) &jid, sizeof(INT32));

  if (rc) {
    fprintf(stderr, "send_req_2_pct: error sending bt request to pct at nid,pid= %d,%d\n", nid, pid);
    return -1;
  }
  return 0;
}


/* this is from get_pct_control_message() in yod_comm.c w/ minor changes */
INT32 get_pct_ctl_msg(INT32* mtype, CHAR** user_data, int* snid, int* spid)
{
INT32 rc, src_nid, src_pid;

  if (SRVR_IS_VALID_HANDLE(pct_mh)) {
    srvr_free_control_msg(BT_PCT_PTL, &pct_mh); /* free the previous one */
  }

  SRVR_CLEAR_HANDLE(pct_mh);

  rc = srvr_get_next_control_msg(BT_PCT_PTL, &pct_mh, mtype, NULL, user_data);

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

int parse_cmd_line(int argc, char* argv[]) 
{
  int nid, index, ch=0;
  int jid_flag=0;
  int nid_flag=0;
  /* holds the node ranges */
  int r0,r1, converts;

  while (ch != EOF) {
    ch = getopt_long_only(argc, argv, "", opts, &index);

    switch(ch) {
      case 'n':
        converts = sscanf( optarg, "%d-%d", &r0, &r1 );
        if (converts !=1 && converts !=2) {
          usage();
          exit(0);
        }
        /* process range */
        if (converts == 1) {
          nid = r0;
          if (nid_index > MAX_NODES) {
            fprintf(stderr, "no. of nids > max no. of nodes: %d\n", MAX_NODES);
            exit (-1);
          }
          nids[nid_index++] = nid;
          pids[pid_index++] = PPID_PCT;
          nid_flag = 1;
        }
        else {
          if ( r1 < r0 ) {
            fprintf(stderr, "invalid nid range... \n");
            usage();
            exit (-1);
          }
          for (nid=r0; nid<=r1; nid++) {
            if (nid_index > MAX_NODES) {
              fprintf(stderr, "no. of nids > max no. of nodes: %d\n", MAX_NODES);
              exit (-1);
            }
            nids[nid_index++] = nid;
            pids[pid_index++] = PPID_PCT;
            nid_flag = 1;
          }
        }

        break;

      case 'j':
         jid = strtol(optarg, (char**)NULL, 10);
         jid_flag = 1;
         break;

      case 'h':
        usage();
        exit(0);
        break;
         
      default:
#if 0
        usage();
        exit(-1);
#endif
        break;
    }
  }

  /* see that we have the same no. of nids as pids */
  if (pid_index != nid_index) {
    fprintf(stderr, "no. of nids (%d) != no. of pids (%d) in node list\n", nid_index, pid_index);
    exit(-1);
  }

  if (jid_flag == 0) {
    fprintf(stderr, "did not get a job id...\n");
    usage();
    exit(-1);
  }

  if (nid_flag == 0) {
    fprintf(stderr, "did not get a node id...\n");
    usage();
    exit(-1);
  }

#if DEBUG
  for (i=0; i<nid_index; i++) {
    PRINTF("nid,pid(%d) = %d,%d\n", i, nids[i], pids[i]);
  }
#endif

  return nid_index;
}

static void usage( void ) 
{
  printf("usage: bt -j job_id -n node-range...\n");
  printf("usage: where possibly repeated node ranges are\n");
  printf("usage: either nonnegative integers or \"-\" separated pairs\n");
  printf("usage: of nonnegative integers: eg. 8 or 8-17\n");
}
