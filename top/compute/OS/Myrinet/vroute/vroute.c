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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <portals/sys_portal_lib.h>

#include <sys/file.h>
#include <sys/ioctl.h>

#include <getopt.h>

#include "vroute.h"

extern char* optarg;

static struct option opts[]=
{
  {"help",          no_argument,           0,  'h'}, /* help */
  {"snid",          required_argument,     0,  's'}, /* from nid */
  {"rnid",          required_argument,     0,  'r'}, /* to   nid */
  {"from",          required_argument,     0,  'f'}, /* from hostname */
  {"to",            required_argument,     0,  't'}, /* to   hostname */
  {"port",          required_argument,     0,  'p'}, /* maybe vrouted's port */
  {"verbosity",     no_argument,           0,  'v'}, /* verbosity */
  {"both",          no_argument,           0,  'b'}, /* ping both directions */
  {"z",             required_argument,     0,  'z'}, /* sleep this many seconds */
  {0, 0, 0, 0} 
};

int sock_init(char* hostname, int try_port, int verbosity);
void open_dev(int sd0, int sd1, char buf[], int bufsize, char* str);
int nid_check(int sd0, char buf[], int bufsize, int my_nid, 
               int* nid, char* str, int verify);
int do_ping(int send_sd, int recv_sd, char send_buf[], char recv_buf[], 
            int bufsize, int snid, int rnid, 
            char* send_str, char* recv_str);
int self_ping(int send_sd, char send_buf[], int bufsize, int snid,
              char* send_str);
int check_args(int got_sname, int got_rname);
void show_usage(void);

static int no_sleep_secs=1; /* default value for time we sleep between
                               telling sender to ping and receiver to
                               check for ping */

#define BUFSIZE 100

int main(int argc, char* argv[]) 
{
  int send_sd, recv_sd, ch=0, index;
  int try_port=0;
  int my_snid=0, my_rnid=0, snid, rnid, rc_snid, rc_rnid;
  int rlen, slen;
  int ping_receiver=0, ping_sender=0;
                            
  char recv_buf[BUFSIZE], send_buf[BUFSIZE];

  char* send_host=NULL;
  char* recv_host=NULL;

  int got_snid=0, got_rnid=0, got_sname=0, got_rname=0;

  int both_dirs=0, verbosity=0;

  while ( ch != EOF ) {
    ch = getopt_long_only(argc, argv, "", opts, &index);

    switch(ch) {
      case 'z':
        no_sleep_secs = strtol(optarg, (char**)NULL, 10);
        if (no_sleep_secs < 1) no_sleep_secs = 1; /* the default */
        break;

      case 's':
        my_snid = strtol(optarg, (char**)NULL, 10);
        got_snid = 1;
        break;

      case 'r':
        my_rnid = strtol(optarg, (char**)NULL, 10);
        got_rnid = 1;
        break;

      case 'f':
        send_host = optarg;
        got_sname = 1;
        break;

      case 't':
        recv_host = optarg;
        got_rname = 1;
        break;

      case 'p':
        try_port = strtol(optarg, (char**)NULL, 10);
        break;

      case 'b':
        both_dirs = 1;
        break;

      case 'v':
        printf("setting verbosity");
        verbosity = 1;
        break;

      case 'h':
        show_usage();
        exit(0);
        break;

      case ':':
        printf("option w/o arg\n");
        break;

      default:
    }
  }

  if ( check_args(got_sname, got_rname) < 0 ) {
    return(-1);
  }

  /* if we didn't get a nid on the command line, assign it an
     arbitrary value
  */
  if ( !got_snid ) {
    my_snid = 0;
  }

  if ( !got_rnid ) {
    my_rnid = 0;
  }


  /* if send_host == recv_host then do things a little
     differently...
  */

/*--------------------------------------------------------------------*/

  /* special case: sender pings sender... */

  if ( !strcmp(send_host, recv_host) ) {

    printf("\n------------------------------------------\n");
    printf("vroute: %s pings self...\n\n", send_host);

    if ( got_rnid ) {
      printf("vroute: ignoring -r option\n\n");
    }

    if ( both_dirs ) {
      printf("vroute: ignoring -b option\n\n");
    }

    send_sd = sock_init(send_host,try_port,verbosity); 

    if ( send_sd < 0 ) {
      printf("------------------------------------------\n");
      return(-1);
    }

    if (verbosity) {
      printf("\nvroute: trying initial handshake w/ sender...\n");
      printf("vroute: if this hangs, we may not have vrouted registered w/\n");
      printf("vroute: inetd on the target node...\n");
    }
    write( send_sd, "y", 1 );

    slen = doRead(send_sd, send_buf, BUFSIZE, strlen(handshake_msg)); /* ack */

    if (verbosity) {
      send_buf[strlen(handshake_msg)] = 0;
      printf("vroute: handshake -- %s\n", send_buf);
    }

    if ( strncmp(handshake_msg, send_buf, strlen(handshake_msg)) ) {
      printf("vroute: -- got a hunch we're not talking to vrouted\n");
      printf("vroute: -- could be service is not registered with inetd\n");
      printf("vroute: -- on sender %s...\n", send_host);
      printf("------------------------------------------\n");
      return(-1);
    }

    /* tell the sender that he is sender AND receiver */

    write( send_sd, "b", 1 );
    slen = read(send_sd, send_buf, BUFSIZE);       /* ack */

    if (verbosity) {
      printf("vroute: trying open_dev(%s)...\n", send_host);
    }
    open_dev(send_sd, 0, send_buf, BUFSIZE, send_host);

    rc_snid = nid_check(send_sd, send_buf, BUFSIZE, my_snid, &snid,
                        send_host, got_snid);

    write( send_sd, "y", 1 );            /* proceed */
    slen = read(send_sd, send_buf, BUFSIZE);

    printf("\n");
    ping_receiver = self_ping(send_sd, send_buf, BUFSIZE, snid, send_host);

    printf("------------------------------------------\n");

    if ( ping_receiver == 0 || rc_snid == 0 ) {
      return -1;
    }
    return 0;
  }

/*--------------------------------------------------------------------*/

  
  /* vroute client code -- make socket connection to
     vroute service on a sender AND on a receiver.
     instead of having them talk directly, client coords
     their interactions.
  */


  printf("\n------------------------------------------\n");

  if ( both_dirs ) {
    printf("vroute: %s and %s ping each other (two-way test)\n\n", send_host, recv_host);
  }
  else {
    printf("vroute: %s pings %s (one-way test)\n\n", send_host, recv_host);
  }

/*------------------------------------------------------------------*/
  send_sd = sock_init(send_host,try_port,verbosity); 

  if ( send_sd < 0 ) {
    printf("------------------------------------------\n");
    return(-1);
  }

  recv_sd = sock_init(recv_host,try_port,verbosity); 

  if ( recv_sd < 0 ) {
    write( send_sd, "n", 1 );
    printf("------------------------------------------\n");
    return(-1);
  }

  if (verbosity) {
    printf("\nvroute: trying initial handshake w/ sender and receiver...\n");
    printf("vroute: if this hangs, we may not have vrouted registered w/\n");
    printf("vroute: inetd on one of the target nodes...\n");
  }
  write( send_sd, "y", 1 );
  write( recv_sd, "y", 1 );

  slen = doRead(send_sd, send_buf, BUFSIZE, strlen(handshake_msg)); /* ack */

  if (verbosity) {
    printf("vroute: got handshake from sender...do receiver\n");
  }

  rlen = doRead(recv_sd, recv_buf, BUFSIZE, strlen(handshake_msg)); /* ack */

  if (verbosity) {
    printf("vroute: got handshake from receiver...now verify...\n");
  }

  if (verbosity) {
    send_buf[strlen(handshake_msg)] = 0;
    printf("vroute: handshake -- %s\n", send_buf);
  }

  if ( strncmp(handshake_msg, send_buf, strlen(handshake_msg)) ) {
    printf("vroute: -- got a hunch we're not talking to vrouted\n");
    printf("vroute: -- could be service is not registered with inetd\n");
    printf("vroute: -- on sender %s...\n", send_host);
    write( recv_sd, "n", 1 );            /* kill receiver */
    printf("------------------------------------------\n");
    return(-1);
  }

  if ( strncmp(handshake_msg, recv_buf, strlen(handshake_msg)) ) {
    printf("vroute: -- got a hunch we're not talking to vrouted\n");
    printf("vroute: -- could be service is not registered with inetd\n");
    printf("vroute: -- on receiver %s...\n", recv_host);
    write( send_sd, "n", 1 );            /* kill sender */
    printf("------------------------------------------\n");
    return(-1);
  }

  /* get things started */

  /* tell sender that he is the sender */

  write( send_sd, "s", 1 );
  slen = read(send_sd, send_buf, BUFSIZE);       /* ack */

  /* tell receiver that he is the receiver */

  write( recv_sd, "r", 1 );
  rlen = read(recv_sd, recv_buf, BUFSIZE);       /* ack */

  /* sender opens devices */

  if (verbosity) {
    printf("vroute: trying open_dev(%s)...\n", send_host);
  }
  open_dev(send_sd, recv_sd, send_buf, BUFSIZE, send_host);

  /* check sender's nid */

  rc_snid = nid_check(send_sd, send_buf, BUFSIZE, my_snid, &snid, send_host,
            got_snid);

  /* receiver opens devices */
  printf("\n");

  if (verbosity) {
    printf("vroute: trying open_dev(%s)...\n", recv_host);
  }
  open_dev(recv_sd, send_sd, recv_buf, BUFSIZE, recv_host);

  /* check receiver's nid */

  rc_rnid = nid_check(recv_sd, recv_buf, BUFSIZE, my_rnid, &rnid, recv_host,
            got_rnid);

  write( recv_sd, "y", 1 );            /* proceed */
  write( send_sd, "y", 1 );            /* proceed */
  slen = read(send_sd, send_buf, BUFSIZE);
  rlen = read(recv_sd, recv_buf, BUFSIZE);

  /* sender and receiver should be synchronized */

  /* tell sender receiver's nid */

  itoa(rnid,istr);
  write( send_sd, istr, sizeof(istr) );

  /* tell receiver sender's nid */
  itoa(snid,istr);
  write( recv_sd, istr, sizeof(istr) );

  slen = read(send_sd, send_buf, BUFSIZE);
  rlen = read(recv_sd, recv_buf, BUFSIZE);

  printf("\n");
  ping_receiver = do_ping(send_sd, recv_sd, send_buf, recv_buf, BUFSIZE, 
                          snid, rnid, send_host, recv_host);

  /* try other direction? */

  if ( both_dirs == 1 ) {

    printf("\n");
    ping_sender = do_ping(recv_sd, send_sd, recv_buf, send_buf, BUFSIZE, 
                          rnid, snid, recv_host, send_host);
  }
  else {
    write( recv_sd, "n", 1 );            /* kill receiver */
    write( send_sd, "n", 1 );            /* kill sender */
  }

  printf("------------------------------------------\n");

/*------------------------------------------------------------------*/

  if ( both_dirs ) {
    if ( ping_receiver == 0 || ping_sender == 0 ) {
      return -1; 
    }
  }
  else {
    if ( ping_receiver == 0 ) {
      return -1; 
    }
  }

  if ( rc_snid == 0 || rc_rnid == 0 ) {
    return -1; 
  }

  return 0;
}


void
open_dev(int sd0, int sd1, char buf[], int bufsize, char* str)
{
  int len;

  /* try to open /dev/portals */
  write( sd0, "y", 1 );            /* proceed */
  len = read(sd0, buf, bufsize);
  buf[len] = 0;

  if ( !strncmp("n", buf, strlen("n")) ) {
    printf("vroute: %s failed to open /dev/portals\n", str);
    write( sd1, "n", 1 );          /* kill receiver */
    exit(-1);
  printf("------------------------------------------\n");
  }
  printf("vroute: %s opened /dev/portals\n", str);

  /* try to open /dev/rtscts */
  write( sd0, "y", 1 );            /* proceed */
  len = read(sd0, buf, bufsize);
  buf[len] = 0;

  if ( !strncmp("n", buf, strlen("n")) ) {
    printf("vroute: %s failed to open /dev/rtscts\n", str);
    write( sd1, "n", 1 );          /* kill receiver */
    printf("------------------------------------------\n");
    exit(-1);
  }
  printf("vroute: %s opened /dev/rtscts\n", str);
}


int
nid_check(int sd0, char buf[], int bufsize, int my_nid, int* nid, 
          char* str, int verify)
{
  int len, rc=1;

  write( sd0, "y", 1 );            /* proceed */
//len = read(sd0, buf, bufsize);
  len = doRead(sd0, buf, bufsize, sizeof(istr));
  buf[len] = 0;

 *nid = strtol(buf, NULL, 10);
//printf("vroute: %s: nid, my_nid= %d, %d\n", str, *nid, my_nid);

  if ( verify ) {
    if ( *nid != my_nid ) {
      rc = 0;
      printf("vroute: %s and i disagree on his nid: i  say  %d\n", str, my_nid);
      printf("vroute: %s and i disagree on his nid: he says %d\n", str, *nid);
    }
    else {
      printf("vroute: %s and i agree on his nid %d: verified.\n", str, *nid);
    }
  }
  return rc;
}


int do_ping(int send_sd, int recv_sd, char send_buf[], char recv_buf[], 
            int bufsize, int snid __attribute__ ((unused)), int rnid, 
            char* send_str, char* recv_str)
{
  int rlen, slen, ping_ok;

  /* tell receiver to proceed -- clear pINgFO[snid] */

  write( recv_sd, "y", 1 );            /* proceed */
  rlen = read(recv_sd, recv_buf, bufsize);
  printf("vroute: %s cleared ping_info[%s]\n", recv_str, send_str);

  /* tell sender to proceed -- ping receiver */
 
  write( send_sd, "y", 1 );            /* proceed */
  slen = read(send_sd, send_buf, bufsize);
  printf("vroute: %s tried to ping %s --\n", send_str, recv_str); 
  printf("vroute: wait a bit before telling %s to check for incoming msg...\n", recv_str);

  /* sleep for a couple of seconds -- else have receiver poll
     pINgFO[snid] and timeout...
  */
  sleep(no_sleep_secs);

  /* tell receiver to proceed -- check pINgFO[snid]  for change */
  write( recv_sd, "y", 1 ); 
  rlen = read(recv_sd, recv_buf, bufsize);

  if ( !strncmp("y", recv_buf, strlen("y")) ) {
    printf("vroute: PING SUCCEEDED: %s got ping from %s...\n", recv_str, send_str);
    ping_ok = 1;
  }
  else {
    printf("vroute: PING FAILED: %s did not see change in ping_info[%s]\n", recv_str, send_str);
    ping_ok = 0;
  }
  return ping_ok;
}



int self_ping(int send_sd, char send_buf[], int bufsize,
              int snid __attribute__ ((unused)), char* send_str)
{
  int slen, ping_ok;

  /* tell sender to proceed -- clear pINgFO[snid] */

  write( send_sd, "y", 1 );            /* proceed */
  slen = read(send_sd, send_buf, bufsize);
  printf("vroute: %s cleared ping_info[%s]\n", send_str, send_str);

  /* tell sender to proceed -- ping self */
 
  write( send_sd, "y", 1 );            /* proceed */
  slen = read(send_sd, send_buf, bufsize);
  printf("vroute: %s tried to ping self --\n", send_str); 
  printf("vroute: wait a bit before telling %s to check for incoming msg...\n", send_str);

  /* sleep for a couple of seconds -- else have receiver poll
     pINgFO[snid] and timeout...
  */
  sleep(2);

  /* tell sender to proceed -- check pINgFO[snid]  for change */
  write( send_sd, "y", 1 ); 
  slen = read(send_sd, send_buf, bufsize);

  if ( !strncmp("n", send_buf, strlen("n")) ) {
    printf("vroute: PING FAILED: %s did not see change in ping_info[%s]\n", send_str, send_str);
    ping_ok = 0;
  }
  else {
    printf("vroute: PING SUCCEEDED: %s got ping from %s...\n", send_str, send_str);
    ping_ok = 1;
  }
  return ping_ok;
}


int check_args(int got_sname, int got_rname)
{
  int fail = 0;

  if ( !got_sname ) {
    printf("vroute: ERROR -- missing cmd line arg for sender host name!\n");
    fail = -1;
  }

  if ( !got_rname ) {
    printf("vroute: ERROR -- missing cmd line arg for receiver host name!\n");
    fail = -1;
  }

  if (fail == -1) {
    show_usage();
  }

  return fail;
}

void show_usage()
{
  printf("usage:\n");
  printf("vroute runs on an SSS node...\n");
  printf("  -f, -from  nodename   -- do myrinet ping from this node (required)\n");
  printf("  -t, -to    nodename   -- do myrinet ping to   this node (required)\n");
  printf("  -b, -both             -- attempt ping in both directions (optional)\n");
  printf("  -p, -port             -- specify vrouted port number (optional)\n");

  printf("  -s, -snid  node id    -- verify the sender's   node id (optional)\n");
  printf("  -r, -rnid  node id    -- verify the receiver's node id (optional)\n");
  printf("  -z,        no_secs    -- number of seconds we sleep between asking\n");
  printf("                        -- sender to send and receiver to check for receipt.\n");
  printf("                        -- the default is 1 second (optional)\n");

  printf("  -v, -verbosity        -- get verbose output (optional)\n");

  printf("  -h, -help             -- this stuff (optional)\n");

}
