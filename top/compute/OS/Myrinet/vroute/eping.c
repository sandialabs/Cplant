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
/* eping.c -- try a 2-way ping between this node and one w/
              specified node id -- does not map a lanai device
              so could be used w/ rtscts over ethernet for instance
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include "RTSCTS_ioctl.h"
#include "RTSCTS_route.h"
#include "common.h"
#include "eping.h"
#include "defines.h"

#define RETRIES 2

int main(int argc, char *argv[])
{
  unsigned long clock, time0;
  int nid, fdr, rc, tries;
  int max_retries = RETRIES;
  struct timeval tv;
  struct timezone tz;

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
    gettimeofday(&tv,&tz);
    time0= (unsigned long)tv.tv_sec;
    clock = 0;
    while ( rc != ROUTE_OK && 
            clock < (int)pow((double)2,(double)tries) ) {
      /* check for receipt of return ping */
      rc = ioctl(fdr, RTS_ROUTE_CHECK, (unsigned long) nid);
      if ( rc < 0 ) {
        printf("%s: ioctl(RTS_ROUTE_CHECK,nid) failed\n", argv[0]);
        exit(-1);
      }
      gettimeofday(&tv,&tz);
      clock = (unsigned long)tv.tv_sec-time0;
    }
    printf("%s: # seconds this try: %ld\n", argv[0], clock);

    if ( rc != ROUTE_OK ) { /* timed out */
      tries++;
    }
    else {
        printf("%s: 2-way ping w/ node %d SUCCEEDED in %d seconds\n", 
                                                       argv[0], nid, clock);
      return 0;
    }
  }
  printf("%s: FAILED -- timed out after %d retries\n", argv[0], max_retries);
  return 0;
}
