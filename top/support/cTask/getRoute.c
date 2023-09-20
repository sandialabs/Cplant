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
/* $Id: getRoute.c,v 1.3 2001/09/29 22:15:08 pumatst Exp $ */

/* this utility is used to query the network id and netmask
   for this node stored in the cTask module -- 

   usage: % getRoute -network | -netmask
   top specify which piece of information is desired
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ctype.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>

#include <cTask/cTask.h>

int main(int argc, char *argv[] )
{
  int rc;
  int fd;	
  unsigned int net[4]={0,0,0,0};
  unsigned int netmask=0, network=0;

  if ( argc != 2 || 
      (strcmp(argv[1],"-network") && strcmp(argv[1],"-netmask")) ) {
    fprintf(stderr, "usage: getRoute -network | -netmask\n"); 
    exit(-1);
  }

  if ( (fd = open("/dev/cTask", O_RDWR)) < 0) {
    perror("setRoute: could not open /dev/cTask... ");
    exit(-1);
  }

  if ( !strcmp(argv[1], "-network") ) {
    rc = ioctl(fd, CPL_COPY_NETWORK, (unsigned long) &network);
    if ( rc ) {
      perror("getRoute: failed to get network address... "); 
      return rc; 
    }	
    net[0] = (network >> 24) & 0xff;
    net[1] = (network >> 16) & 0xff;
    net[2] = (network >> 8)  & 0xff;
    net[3] =  network        & 0xff;
    printf("network id= %d.%d.%d.%d\n", net[0], net[1], net[2], net[3]);
  }
  else {
    rc = ioctl(fd, CPL_COPY_NETMASK, (unsigned long) &netmask);
    if ( rc ) {
      perror("getRoute: failed to get netmask address... "); 
      return rc; 
    }	
    net[0] = (netmask >> 24) & 0xff;
    net[1] = (netmask >> 16) & 0xff;
    net[2] = (netmask >> 8)  & 0xff;
    net[3] =  netmask        & 0xff;
    printf("netmask= %d.%d.%d.%d\n", net[0], net[1], net[2], net[3]);
  }
  return 0;
}
