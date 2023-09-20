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
/* rroute.c -- try a myrinet ping (rtscts) along a 
               specified route; request an ack along a
               specified reverse route
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <cTask/cTask.h>
#include "RTSCTS_ioctl.h"
#include "common.h"
#include "vping.h"
#include "defines.h"
#include "lanai_device.h"

/* time out after this no. of seconds */
#define TIMEOUT 2

mcpshmem_t *mcpshmem;

#define NBYTES 31

char fbytes[NBYTES] = {0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88,
                 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0xbf, 0xbe,
                 0xbd, 0xbc, 0xbb, 0xba, 0xb9, 0xb8, 0xb7, 0xb6, 0xb5,
                 0xb4, 0xb3, 0xb2, 0xb1};

char rbytes[NBYTES] = {0x80, 0xbf, 0xbe, 0xbd, 0xbc, 0xbb, 0xba, 0xb9, 0xb8, 
                 0xb7, 0xb6, 0xb5, 0xb4, 0xb3, 0xb2, 0xb1, 0x81, 0x82,
                 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b,
                 0x8c, 0x8d, 0x8e, 0x8f};

int main(int argc, char *argv[])
{
  int fdr, rc, clock, btype;
  int unit=0, verbose=0; 
  int route_len, i, j;
  char route[1024];
  char rev_route[1024];

  if (argc <= 1) {
    printf("rroute usage: rroute route-bytes, where route-bytes\n");
    printf("is a space-separated list of hex route bytes in the\n");
    printf("format: 0xST (0x followed by 2 hex digits).\n");
    printf("rroute will take any sequence of bytes, but the intention\n");
    printf("is that the route should form a path to some node that\n");
    printf("will send an ack along the reverse set of route bytes...\n");
    exit(-1);
  }

  route_len = argc-1;

  /* verify route byte format and store */
  for (i=1; i<argc; i++) {
    if ( argv[i][0] != '0' || argv[i][1] != 'x' 
                       || !isxdigit(argv[i][2]) || !isxdigit(argv[i][3]) ) {
      printf("rroute: route byte format error (%s); use 0xST\n", argv[i]);
      printf("        where S and T are hex digits.\n");
      exit(-1);
    } 
    route[i-1] = strtol(argv[i], NULL, 16);
  }

  for (i=0; i<route_len; i++) {
    for (j=0; j<NBYTES; j++) {
      if ( route[i] == fbytes[j] ) {
//      rev_route[i] = rbytes[j];
        rev_route[route_len-i-1] = rbytes[j];
//      printf("fbyte, rbyte= 0x%x, 0x%x\n", route[i], rev_route[route_len-i-1]);
        break;
      }
    }
  }

  /* map the lanai board -- we want to refer to 
     mcpshmem's ping status entry
  */
  if (map_lanai(argv[0], verbose, unit) != OK) {
    printf("%s: could not map_lanai...\n", argv[0]);
    exit(-1);
  }

  printf("%s: lanai type is ", argv[0]);
  btype = get_lanai_type(unit, TRUE);
  printf("\n");

  if ( (mcpshmem = get_mcpshmem( verbose, unit, argv[0], FALSE)) 
                                                       == NULL) {
    printf("%s: could not get_mcpshmem...\n", argv[0]);
    exit(-1);
  }

  /* open rtscts device */

  if ( (fdr = open("/dev/rtscts", O_RDWR)) < 0) {
    printf("rroute: failed to open /dev/rtscts...\n");
    exit(-1);
  }

  /* set route len */

  rc = ioctl(fdr, RTS_SET_TEST_ROUTE_LEN, (unsigned long) route_len);
  if ( rc < 0 ) {
    printf("rroute: ioctl(RTS_SET_TEST_ROUTE_LEN,%d) failed\n", route_len);
    exit(-1);
  }

  /* copy the reverse route to kernel memory */
  rc = ioctl(fdr, RTS_SET_REVERSE_ROUTE, (unsigned long) rev_route);
  if ( rc < 0 ) {
    printf("rroute: ioctl(RTS_SET_REVERSE_ROUTE) failed\n");
    exit(-1);
  }

  /* ping -- this call will copy the route into host shared memory */

  rc = ioctl(fdr, RTS_DO_TEST_ROUTE_W_REV, (unsigned long) route);
  if ( rc < 0 ) {
    printf("rroute: ioctl(RTS_DO_TEST_ROUTE_W_REV) failed\n");
    exit(-1);
  }

  rc = -1;
  clock = 0;
  setRTC(unit,btype,0);
  while ( rc == -1 && clock < TIMEOUT*ONE_SECOND ) {
    rc = (int)ntohl(mcpshmem->ping_stat);
    clock = getRTC(unit,btype);
  }

  if ( rc == -1 ) {
    printf("rroute: FAILED -- no result after %d usecs\n", clock/2);
    return 0;
  }
  else {
    printf("rroute: got ack from node %d after %d usecs\n", rc, clock/2);
  }

  return 0;
}
