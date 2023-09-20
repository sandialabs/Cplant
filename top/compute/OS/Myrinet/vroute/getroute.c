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
/* getroute.c -- extract specified route from mcpshmem
                 and print it
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "RTSCTS_ioctl.h"
#include "RTSCTS_route.h"

int main(int argc, char *argv[])
{
  int fdr, rc;
  int route_id, i;
  char route[1024];
  int byte;

  if (argc != 2) {
    printf("getroute usage: getroute node-id, where node-id\n");
    printf("specifies the route destination...\n");
    exit(-1);
  }

  route_id = atoi(argv[1]);

#if 0
  /* verify route byte format and store */
  for (i=1; i<argc; i++) {
    if ( argv[i][0] != '0' || argv[i][1] != 'x' 
                       || !isxdigit(argv[i][2]) || !isxdigit(argv[i][3]) ) {
      printf("troute: route byte format error (%s); use 0xST\n", argv[i]);
      printf("        where S and T are hex digits.\n");
      exit(-1);
    } 
    route[i-1] = strtol(argv[i], NULL, 16);
  }
#endif

  /* open rtscts device */

  if ( (fdr = open("/dev/rtscts", O_RDWR)) < 0) {
    printf("getroute: failed to open /dev/rtscts...\n");
    exit(-1);
  }

  /* set route id */

  rc = ioctl(fdr, RTS_SET_ROUTE_ID, (unsigned long) route_id);
  if ( rc < 0 ) {
    printf("getroute: ioctl(RTS_SET_ROUTE_ID) failed\n");
    exit(-1);
  }

  /* get route */

  rc = ioctl(fdr, RTS_GET_ROUTE, (unsigned long) route);
  if ( rc < 0 ) {
    printf("getroute: ioctl(RTS_GET_ROUTE) failed\n");
    exit(-1);
  }

  for (i=0; i<MAX_ROUTE_LEN; i++) {
    byte = (int) route[i];
    if (byte == 0) {
      break;
    }
    byte &= 0xff;
    printf("0x%x ",byte);
  }
  printf("\n");

  return 0;
}
