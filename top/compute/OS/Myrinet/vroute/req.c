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
/* req.c: request that some other node myrinet ping yet another
          node and report the result back here.
          should be run on a compute/service node.
*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "RTSCTS_ioctl.h"
#include "RTSCTS_route.h"
#include "MCPshmem.h"
#include "common.h"
#include "vping.h"
#include "sys/defines.h"
#include <asm/byteorder.h>
#include "lanai_device.h"


/* the number of times we double the timeout period
   (starting at ONE_SECOND) and retry the ping test
*/
#define RETRIES 4

mcpshmem_t *mcpshmem=NULL;
int unit=0;
int verbose=0;

void usage(char* name);

int main(int argc, char* argv[]) {

  int fd, rc, clock, btype;
  int tries, max_retries=RETRIES;
  int rte_nids[2];

  if ( argc < 3 || argc > 4 ) {
    usage(argv[0]);
    exit(-1);
  }

  rte_nids[0] = atoi(argv[1]);
  rte_nids[1] = atoi(argv[2]);

  if ( argc == 4 ) {
    max_retries = atoi(argv[3]);
  }

  /* map the lanai board -- we want to refer to 
     mcpshmem's request result entry...
  */
  if (map_lanai(argv[0], verbose, unit) != OK) {
    printf("%s: could not map_lanai...\n", argv[0]);
    exit(-1);
  }

  printf("%s: lanai type is ", argv[0]);
  btype = get_lanai_type(unit, TRUE);
  printf("\n");

  if ( (mcpshmem = get_mcpshmem( verbose, unit, argv[0], FALSE)) 
                                                            == NULL)   {
    printf("%s: could not get_mcpshmem...\n", argv[0]);
    exit(-1);
  }

  fd = open("/dev/rtscts", O_RDWR);

#if 0
  printf("%s: mcpshmem->route_request= %d\n", argv[0], (int)ntohl(mcpshmem->route_request));
#endif

  tries = 0;
  while( tries <= max_retries ) {

    /* request ping */
    rc = ioctl(fd, RTS_ROUTE_REQ, rte_nids);
#if 0
    printf("%s: mcpshmem->route_request= %d\n", argv[0], (int)ntohl(mcpshmem->route_request));
    usleep(100000);

    printf("%s: mcpshmem->route_request= %d\n", argv[0], (int)ntohl(mcpshmem->route_request));
#endif

    //rc = ioctl(fd, RTS_ROUTE_REQ_RESULT);

    rc = ROUTE_NO_ANSWER;
    clock = 0;
    setRTC(unit,btype,0);
    while ( rc != ROUTE_OK  && clock < (int)pow((double)2,(double)tries)*ONE_SECOND ) {
      rc = (int)ntohl(mcpshmem->route_request);
      clock = getRTC(unit,btype);
    }

    printf("%s: usecs this try: %d\n", argv[0], clock/2);

    if ( rc == ROUTE_OK ) {
      printf("%s: ping request (%d,%d) SUCCEEDED\n", argv[0], rte_nids[0], rte_nids[1]);
      return 0;
    } 

    if ( rc == ROUTE_CONFUSING_REPLY ) {
      printf("%s: ping request (%d,%d) FAILED: confusing reply...\n", argv[0], rte_nids[0], rte_nids[1]);
      return 0;
    }

    tries++ ;
  }
  printf("%s: ping request (%d,%d) FAILED after %d retries\n", argv[0], rte_nids[0], rte_nids[1], max_retries);
  printf("%s: longest timeout: %d seconds\n", argv[0], clock/ONE_SECOND);
  return 0;
}

void usage(char* name)
{
  printf("usage: %s node0 node1 [max_retries]\n", name); 
  printf(" -- should be run from a proxy (service) node,\n");
  printf(" -- proxy requests that node0 does a 2-way ping\n");
  printf(" -- w/ node1 and relays the result back to the\n");
  printf(" -- proxy (service) node...max_retries (optional,>=0)\n");
  printf(" -- limits the number of expanded timeouts used\n");
}
