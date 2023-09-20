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
/* vping.c -- try a 2-way ping between this node and one w/
              specified node id
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "RTSCTS_ioctl.h"
#include "RTSCTS_route.h"
#include "common.h"
#include "vping.h"
#include "lanai_device.h"
#include "defines.h"
#include "MCPshmem.h"

#define RETRIES 2

mcpshmem_t *mcpshmem;

int main(int argc, char *argv[])
{
  int nid, fdr, rc, btype, clock, tries;
  int max_retries = RETRIES;
  int verbose=0;
  int unit=0;

  if ( argc <2 || argc >3 ) {
    printf("usage: %s nid [max_retries]\n", argv[0]);
    printf("max_retries (optional,>=0) limits the no.\n");
    printf("of expanded timeouts used\n");
    exit(-1);
  }

  nid = strtol(argv[1], (char**)NULL, 10);
 
  if ( argc == 3 ) {
    max_retries = strtol(argv[2], (char**)NULL, 10);
  }
  
  printf("%s: doing 2-way ping w/ node %d\n", argv[0], nid);

  /* map the lanai board -- we want to refer to 
     mcpshmem's route status table (collapsed to single entry?)
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
    printf("%s: failed to open /dev/rtscts...\n", argv[0]);
    exit(-1);
  }

  tries = 0;
  while (tries <= max_retries) {

    /* send ping */
    rc = ioctl(fdr, RTS_ROUTE_STAT, (unsigned long) nid);
    if ( rc < 0 ) {
      printf("%s: ioctl(RTS_ROUTE_STAT,nid) failed\n", argv[0]);
      exit(-1);
    }

    rc = -1;
    setRTC(unit,btype,0);
    clock = 0;
    while ( rc == -1  && clock < (int)pow((double)2,(double)tries)*ONE_SECOND ) {
      rc = (int)ntohl(mcpshmem->route_stat);
      clock = getRTC(unit,btype);
    }
    printf("%s: # usecs this try: %d\n", argv[0], clock/2);

    if ( rc == -1 ) { /* timed out */
      tries++;
    }
    else {
      if ( rc != nid ) {
        printf("%s: FAILED -- got reply from %d, requested from %d\n",
                                                argv[0], rc, nid);
      }
      else {
        printf("%s: 2-way ping w/ node %d SUCCEEDED in %d usecs\n", argv[0], nid, clock/2);
      }
      return 0;
    }
  }
  printf("%s: FAILED -- timed out after %d retries\n", argv[0], max_retries);
  return 0;

#if 0 
  /* sleep a while */
  usleep(100000);

  /* check for receipt of return ping */
  rc = ioctl(fdr, RTS_ROUTE_CHECK, (unsigned long) nid);
  if ( rc < 0 ) {
    printf("%s: ioctl(RTS_ROUTE_CHECK,nid) failed\n", argv[0]);
    exit(-1);
  }

  if ( rc == ROUTE_OK ) {
    printf("%s: 2-way ping w/ node %d SUCCEEDED\n", argv[0], nid);
  }
  else {
    printf("vping: 2-way ping w/ node %d FAILED\n", argv[0], nid);
  }

  return 0;
#endif
}
