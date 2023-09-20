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
#include <syslog.h>

#include <cTask/cTask.h>
#include <rtscts/RTSCTS_ioctl.h> 

#include <sys/file.h>
#include <sys/ioctl.h>

#include <netdb.h>           /* gethostbyname() */
#include <netinet/in.h>      /* sockaddr_in     */
#include <sys/socket.h>      /* AF_INET, SOCK_DGRAM */
#include <arpa/inet.h>       /* inet_ntoa()     */

#include "vroute.h"

#include <errno.h>
#include <unistd.h>

void send_interaction(int fd_in, int fd_out, char buf[], int bufsize);
void recv_interaction(int fd_in, int fd_out, char buf[], int bufsize);
void both_interaction(int fd_in, int fd_out, char buf[], int bufsize);
void read_test(int fd, char buf[], int bufsize, int ind);

#define BUFSIZE 100

#include <getopt.h>

extern char* optarg;

static struct option opts[]=
{
  {"inetd",       no_argument      ,     0,  'i'}, /* run using inetd */
  {"port",        required_argument,     0,  'p'}, /* specify service port */
  {"verbosity",   no_argument,          0,  'v'}, /* log progress to system */
  {0, 0, 0, 0} 
};

static int service_port=0;
static int verbosity=0;
static int inetd=0;

int main(int argc, char* argv[]) 
{
  int fds, ch=0, index;
  int len, rlen, clientlen, sock;
  struct servent* serv_p;
  struct sockaddr_in sname, client;
  char host[BUFSIZE];
  char buf[BUFSIZE];

  /* vroute server code */

  while ( ch != EOF ) {
    ch = getopt_long_only(argc, argv, "", opts, &index);

    switch(ch) {
      case 'p':
        service_port = strtol(optarg, NULL, 10);
        break;

      case 'i':
        inetd = 1;
        break;

      case 'v':
        verbosity = 1;
        break;

      default:
    }
  }

  if ( inetd==0 && service_port<=0 ) {
    printf("vrouted: ERROR: if run by hand, must supply valid service\n");
    printf("vrouted: ERROR: port>0 using the -p command line option...\n");
    exit(-1);
  }

  if ( inetd==1 && service_port>0 ) {
    printf("vrouted: ERROR: -i and -p flags are mutually exclusive,\n");
    printf("vrouted: ERROR: setting service_port=0...\n");
    service_port=0;
  }

  sname.sin_family = AF_INET;
  sname.sin_addr.s_addr = INADDR_ANY;
 
  if ( !service_port ) {
    openlog("vrouted", LOG_CONS, LOG_DAEMON);
  }

  if ( gethostname(host,80) <0 ) {
    if ( service_port) {
      perror("vroute: gethostname() failed");
    }
    else {
      syslog(LOG_DAEMON | LOG_ERR, "gethostname() failed\n");
    }
    exit(-1);
  }

  if ( service_port ) {
    printf("vrouted: %s trying port from user: %d\n", host, service_port);
    sname.sin_port = htons(service_port);
  }
  else {
    if ( (serv_p = getservbyname("vroute","tcp")) == NULL ) {
      syslog(LOG_DAEMON | LOG_ERR, "getservbyname() failed...\n");
      exit(-1);
    }
    if ( verbosity ) {
      syslog(LOG_DAEMON | LOG_ERR, "%s trying vroute service port: %d\n", host, ntohs(serv_p->s_port));
    }
    sname.sin_port = serv_p->s_port;
  }

  /* create the socket */

  if ( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
    if ( service_port ) {
      perror("vrouted: socket() failed");
    }
    else {
      syslog(LOG_DAEMON | LOG_ERR, "socket() failed\n");
    }
    exit(-1);
  }

  /* bind address to local end of socket */

  if (service_port) { /* user specifies service port */
    if ( bind(sock, (struct sockaddr*) &sname, sizeof(sname)) < 0 ) {
      perror("vrouted: bind() failed"); 
      exit(-1);
    }
  }
  else {
    if ( bind(sock, (struct sockaddr*) &sname, sizeof(sname)) < 0 ) {
      if (errno == EADDRINUSE) {
        /* close sock -- read and write to STDIN and STDOUT */
        close(sock);
        goto proceed;
      }
      syslog(LOG_DAEMON | LOG_ERR, "bind() failed\n");
      exit(-1);
    }

proceed:
  }

  if (service_port) {

    /* accept connections */

    listen(sock,5);
    len = sizeof(sname);
    clientlen = sizeof(client);

    printf("vrouted: %s accept()ing connections...\n", host);

    if ( (fds = accept(sock, (struct sockaddr*) &sname, &len)) < 0 ) {
      perror("vrouted: server accept");
      exit(-1);
    }

    printf("vrouted: %s got connection...\n", host);

    /* announce the accept */

    if ( getpeername(fds, (struct sockaddr*) &client, &clientlen) <0 ) {
      perror("vroute: getpeername() failed");
      exit(-1);
    }


    printf("vrouted: %s accept connection from %s\n", host, inet_ntoa(client.sin_addr));

    read_test(fds,buf,BUFSIZE,0);
    write(fds, handshake_msg, strlen(handshake_msg));

    rlen = read(fds, buf, BUFSIZE);
    buf[rlen] = 0;

    if ( !strncmp("s", buf, 1) ) {
      printf("vrouted: i'm the sender...\n");

      write(fds, "a", 1);
      send_interaction(fds,fds,buf,BUFSIZE);
    }
    else {
      if ( !strncmp("r", buf, 1) ) {
        printf("vrouted: i'm the receiver...\n");

        write(fds, "a", 1);
        recv_interaction(fds,fds,buf,BUFSIZE);
      }
      else {
        if ( !strncmp("b", buf, 1) ) {
          printf("vrouted: i'm both sender AND receiver...\n");

          write(fds, "a", 1);
          both_interaction(fds,fds,buf,BUFSIZE);
        }
        else {
          printf("vrouted: protocol error: getting my identity\n");
          exit(-1);
        }
      }
    }
  } 
  else {
    /* port for vroute service (vrouted) defined in /etc/services */

#if 0
    if ( getpeername(STDIN_FILENO, (struct sockaddr*) &client, &clientlen) <0 ) {
      syslog(LOG_DAEMON | LOG_ERR, "getpeername() failed\n");
      exit(-1);
    }

    syslog(LOG_DAEMON | LOG_ERR, "%s accept connection from %s\n", host, inet_ntoa(client.sin_addr));
#else
    if ( verbosity ) {
      syslog(LOG_DAEMON | LOG_ERR, "%s accept connection from...\n", host);
    }
#endif

    read_test(STDIN_FILENO,buf,BUFSIZE,0);
    write(STDOUT_FILENO, handshake_msg, strlen(handshake_msg));

    /* get identity */

    rlen = read(STDIN_FILENO, buf, BUFSIZE);
    buf[rlen] = 0;

    if ( !strncmp("s", buf, 1) ) {
      if (verbosity) {
        syslog(LOG_DAEMON | LOG_ERR, "looks like i'm the sender\n");
      }

      write(STDOUT_FILENO, "a", 1);
      send_interaction(STDIN_FILENO,STDOUT_FILENO,buf,BUFSIZE);
    }
    else {
      if ( !strncmp("r", buf, 1) ) {
        if (verbosity) {
          syslog(LOG_DAEMON | LOG_ERR, "looks like i'm the receiver\n");
        }

        write(STDOUT_FILENO, "a", 1);
        recv_interaction(STDIN_FILENO,STDOUT_FILENO,buf,BUFSIZE);
      }
      else {
        if ( !strncmp("b", buf, 1) ) {
          if (verbosity) {
            syslog(LOG_DAEMON | LOG_ERR, "looks like i'm sender AND receiver\n");
          }

          write(STDOUT_FILENO, "a", 1);
          both_interaction(STDIN_FILENO,STDOUT_FILENO,buf,BUFSIZE);
        }
        else {
          syslog(LOG_DAEMON | LOG_ERR, "protocol error: getting my identity...\n");
          exit(-1);
        }
      }
    }
  }
  return 0;
}


void both_interaction(int fd_in, int fd_out, char buf[], int bufsize)
{
  int len, my_nid, fdc, fdr;

  read_test(fd_in,buf,bufsize,1);

  if ( (fdc = open("/dev/cTask", O_RDWR)) < 0) {
    if ( service_port ) {
      perror("vrouted: could not open /dev/cTask...");
    }
    else {
      syslog(LOG_DAEMON | LOG_ERR, "could not open /dev/cTask\n");
    }
    write(fd_out, "n", 1);
    len = read(fd_in, buf, bufsize);
    exit(-1);
  }
  /* was able to open /dev/cTask */
  write(fd_out, "y", 1);

  read_test(fd_in,buf,bufsize,2);

  if ( (fdr = open("/dev/rtscts", O_RDWR)) < 0) {
    if ( service_port ) {
      perror("vrouted: could not open /dev/rtscts...");
    }
    else {
      syslog(LOG_DAEMON | LOG_ERR, "could not open /dev/rtscts\n");
    }
    write(fd_out, "n", 1);
    len = read(fd_in, buf, bufsize);
    exit(-1);
  }
  /* was able to open /dev/rtscts */
  write( fd_out, "y", 1 );

  read_test(fd_in,buf,bufsize,3);


  /* my nid is... */
  my_nid = ioctl(fdc, CPL_GET_PHYS_NID, 0L);
  itoa(my_nid,istr);
  write(fd_out, istr, sizeof(istr));

  /* synchronize */

  read_test(fd_in,buf,bufsize,4);
  write(fd_out, "a", 1);

  /* clear pINgFO[my_nid] */

  read_test(fd_in,buf,bufsize,5);


  ioctl(fdr, RTS_SET_PING, (unsigned long) my_nid);
  write(fd_out, "a", 1);

  /* ping self */

  read_test(fd_in,buf,bufsize,6);

  ioctl(fdr, RTS_DO_PING, (unsigned long) my_nid);
  write(fd_out, "a", 1);

  /* check pINgFO[my_nid] for change */

  read_test(fd_in,buf,bufsize,7);

  if ( ioctl(fdr, RTS_GET_PING, (unsigned long) my_nid) == 0 ) {
    write(fd_out, "n", 1);
  }
  else {
    write(fd_out, "y", 1);
  }
}


void send_interaction(int fd_in, int fd_out, char buf[], int bufsize)
{
  int len, my_nid, fdc, fdr;
  int rnid;
  char istr[21];

  read_test(fd_in,buf,bufsize,1);

  if ( (fdc = open("/dev/cTask", O_RDWR)) < 0) {
    if ( service_port ) {
      perror("vrouted: could not open /dev/cTask...");
    }
    else {
      syslog(LOG_DAEMON | LOG_ERR, "could not open /dev/cTask\n");
    }
    write(fd_out, "n", 1);
    len = read(fd_in, buf, bufsize);
    exit(-1);
  }
  /* was able to open /dev/cTask */
  write(fd_out, "y", 1);

  read_test(fd_in,buf,bufsize,2);

  if ( (fdr = open("/dev/rtscts", O_RDWR)) < 0) {
    if ( service_port ) {
      perror("vrouted: could not open /dev/rtscts...");
    }
    else {
      syslog(LOG_DAEMON | LOG_ERR, "could not open /dev/rtscts\n");
    }
    write(fd_out, "n", 1);
    len = read(fd_in, buf, bufsize);
    exit(-1);
  }
  /* was able to open /dev/rtscts */
  write( fd_out, "y", 1 );

  read_test(fd_in,buf,bufsize,3);

  /* my nid is... */
  my_nid = ioctl(fdc, CPL_GET_PHYS_NID, 0L);
  itoa(my_nid,istr);
  write(fd_out, istr, sizeof(istr));

  /* synchronize */

  read_test(fd_in,buf,bufsize,4);
  write(fd_out, "a", 1);

  /* get receiver's nid */

//len = read(fd_in, buf, bufsize);
  len = doRead(fd_in, buf, bufsize, strlen(istr));
  buf[len] = 0;
  rnid = strtol(buf, NULL, 10);

#if 0
  if (service_port) {
    printf("vrouted: sender: i think receiver= %d\n", rnid);
  }
#endif

  write(fd_out, "a", 1);

  /* ping receiver */

  read_test(fd_in,buf,bufsize,5);

  ioctl(fdr, RTS_DO_PING, (unsigned long) rnid);
  write(fd_out, "a", 1);

  /* ping in other direction? */

  /* if so, clear pINgFO[rnid] */

  read_test(fd_in,buf,bufsize,6);

  ioctl(fdr, RTS_SET_PING, (unsigned long) rnid);
  write(fd_out, "a", 1);

  /* check pINgFO[rnid] for change */

  read_test(fd_in,buf,bufsize,7);

  if ( ioctl(fdr, RTS_GET_PING, (unsigned long) rnid) == 0 ) {
    write(fd_out, "n", 1);
  }
  else {
    write(fd_out, "y", 1);
  }
}


void recv_interaction(int fd_in, int fd_out, char buf[], int bufsize)
{
  int len, my_nid, snid, fdc, fdr;
  char istr[21];

  read_test(fd_in,buf,bufsize,0);

  if ( (fdc = open("/dev/cTask", O_RDWR)) < 0) {
    if ( service_port ) {
      perror("vrouted: could not open /dev/cTask...");
    }
    else {
      syslog(LOG_DAEMON | LOG_ERR, "could not open /dev/cTask\n");
    }
    write(fd_out, "n", 1);
    len = read(fd_in, buf, bufsize);
    exit(-1);
  }
  /* was able to open /dev/cTask */
  write(fd_out, "y", 1);

  read_test(fd_in,buf,bufsize,1);

  if ( (fdr = open("/dev/rtscts", O_RDWR)) < 0) {
    if ( service_port ) {
      perror("vrouted: could not open /dev/rtscts...");
    }
    else {
      syslog(LOG_DAEMON | LOG_ERR, "could not open /dev/rtscts\n");
    }
    write(fd_out, "n", 1);
    len = read(fd_in, buf, bufsize);
    exit(-1);
  }
  /* was able to open /dev/rtscts */
  write(fd_out, "y", 1);

  read_test(fd_in,buf,bufsize,2);

  /* my nid is... */
  my_nid = ioctl(fdc, CPL_GET_PHYS_NID, 0L);
  itoa(my_nid,istr);
  write(fd_out, istr, sizeof(istr));

  /* synchronize -- client may want to kill us */

  read_test(fd_in,buf,bufsize,3);
  write(fd_out, "a", 1);

  /* get sender's nid */

//len = read(fd_in, buf, bufsize);
  len = doRead(fd_in, buf, bufsize, strlen(istr));
  buf[len] = 0;
  snid = strtol(buf, NULL, 10);

#if 0
  if (service_port) {
    printf("vrouted: receiver: i think sender= %d\n", snid);
  }
#endif

  write(fd_out, "a", 1);

  /* clear pINgFO[snid] */

  read_test(fd_in,buf,bufsize,4);

  ioctl(fdr, RTS_SET_PING, (unsigned long) snid);
  write(fd_out, "a", 1);

  /* check pINgFO[snid] for change */

  read_test(fd_in,buf,bufsize,5);

  if ( ioctl(fdr, RTS_GET_PING, (unsigned long) snid) == 0 ) {
    write(fd_out, "n", 1);
  }
  else {
    write(fd_out, "y", 1);
  }

  /* ping in other direction? */

  /* if so, ping sender */

  read_test(fd_in,buf,bufsize,6);

  ioctl(fdr, RTS_DO_PING, (unsigned long) snid);
  write(fd_out, "a", 1);

}


void read_test(int fd, char buf[], int bufsize, int ind) 
{
  int len;

  len = read(fd, buf, bufsize);
  buf[len] = 0;
  if ( !strncmp("n", buf, 1) ) {
    if ( service_port ) {
      printf("vrouted: killed by controller at %d\n", ind);
    }
    else {
      syslog(LOG_DAEMON | LOG_ERR, "killed by controller at %d\n", ind);
    }
    exit(-1);
  }
  return;
}
