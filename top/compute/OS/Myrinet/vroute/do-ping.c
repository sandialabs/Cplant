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
/* do-ping.c -- simply send a 1-way pingd to specified 
                node id. can check for its receipt on 
                dest node by doing get-ping on that node
                before and after doing the do-ping,
                and comparing the values (the kernel
                toggles the pINgFO slot between 1 and 0)
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "RTSCTS_ioctl.h"
#include "RTSCTS_route.h"

int main(int argc, char *argv[])
{
  int nid, fdr, rc;

  if ( argc != 2) {
    printf("usage: do-ping nid\n");
    exit(-1);
  }

  nid = strtol(argv[1], (char**)NULL, 10);
  
  printf("do-ping: doing 1-way ping w/ node %d\n", nid);

  /* open rtscts device */

  if ( (fdr = open("/dev/rtscts", O_RDWR)) < 0) {
    printf("do-ping: failed to open /dev/rtscts...\n");
    exit(-1);
  }

  /* send ping */
  rc = ioctl(fdr, RTS_DO_PING, (unsigned long) nid);
  if ( rc < 0 ) {
    printf("do-ping: ioctl(RTS_DO_PING,nid) failed\n");
    exit(-1);
  }

  return 0;
}
