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
/* troute.c -- try a myrinet self ping (rtscts) along a 
               specified route
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

int main(int argc, char *argv[])
{
  int mynid, fdr, fdc, rc, clock, btype;
  int unit=0, verbose=0; 
  int route_len, i;
  char route[1024];

#if 0
  route[0] = 0x84;
  route[1] = 0x81;
  route[2] = 0xb7;
#endif

  if (argc <= 1) {
    printf("troute usage: troute route-bytes, where route-bytes\n");
    printf("is a space-separated list of hex route bytes in the\n");
    printf("format: 0xST (0x followed by 2 hex digits).\n");
    printf("troute will take any sequence of bytes, but the intention\n");
    printf("is that the route should form a path back to the current\n");
    printf("node as a self-ping test will be performed...\n");
    exit(-1);
  }

  route_len = argc-1;

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

  /* open cTask device */

  if ( (fdc = open("/dev/cTask", O_RDWR)) < 0) {
    printf("troute: failed to open /dev/cTask...\n");
    exit(-1);
  }
  mynid = ioctl(fdc, CPL_GET_PHYS_NID, 0L);

  if (mynid < 0) {
    printf("troute: failed to get physical node id...\n");
    exit(-1);
  }

  /* open rtscts device */

  if ( (fdr = open("/dev/rtscts", O_RDWR)) < 0) {
    printf("troute: failed to open /dev/rtscts...\n");
    exit(-1);
  }

#if 0
  /* clear my pINgFO entry */

  rc = ioctl(fdr, RTS_SET_PING, (unsigned long) mynid);
  if ( rc < 0 ) {
    printf("troute: ioctl(RTS_SET_PING) failed\n");
    exit(-1);
  }
#endif

  /* set route len */

  rc = ioctl(fdr, RTS_SET_TEST_ROUTE_LEN, (unsigned long) route_len);
  if ( rc < 0 ) {
    printf("troute: ioctl(RTS_SET_TEST_ROUTE_LEN,%d) failed\n", route_len);
    exit(-1);
  }

  /* ping -- this call will copy the route into host shared memory */

  rc = ioctl(fdr, RTS_DO_TEST_ROUTE, (unsigned long) route);
  if ( rc < 0 ) {
    printf("troute: ioctl(RTS_DO_TEST_ROUTE) failed\n");
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
    printf("troute: FAILED -- no result after %d usecs\n", clock/2);
    return 0;
  }

  if ( rc != mynid ) {
    printf("troute: FAILED -- got ping with bad node id %d\n", rc);
  }
  else {
    printf("troute: SUCCEEDED after %d usecs\n", clock/2);
  }
  return 0;

#if 0
  /* sleep a while */
  usleep(100000);

  /* the returned value is the flag in the pINgFO table */

  rc = ioctl(fdr, RTS_GET_PING, (unsigned long) mynid);

  if (rc == 0) {
    printf("troute: self ping FAILURE!\n");
  }
  else {
    printf("troute: self ping SUCCESS!\n");
  }
#endif
}
