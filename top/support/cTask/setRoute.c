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
/* $Id: setRoute.c,v 1.4 2001/09/29 22:15:08 pumatst Exp $ */

/* this utility is used to pass a specified network address and
   netmask for a node to the cTask module -- 
   similar to the Unix "route" command

   usage: % setRoute network-id netmask
   where the 2 arguments are in dotted decimal ip notation:
          % setRoute 10.10.0.0 255.255.0.0 , for example. 
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
  unsigned int mask[4]={0,0,0,0}; 
  unsigned int nw=0, nm=0;

  if ( argc != 3 ) {
    fprintf(stderr, "usage: setRoute network-addr netmask\n"); 
    fprintf(stderr, "where network-addr and netmask are in ip address\n");
    fprintf(stderr, "decimal format. eg, \"setRoute 10.10.0.0 255.255.0.0\"\n");
    exit(-1);
  }
    rc = sscanf( argv[1], "%d.%d.%d.%d",
                 &net[3], &net[2], &net[1], &net[0]);
    if (rc != 4) goto ERR;

    rc = sscanf( argv[2], "%d.%d.%d.%d",
                 &mask[3], &mask[2], &mask[1], &mask[0]);
    if (rc != 4) goto ERR;

    nw = net[0] + 
             (net[1] << 8) + (net[2] << 16) + (net[3] << 24);

    nm = mask[0] + 
             (mask[1] << 8) + (mask[2] << 16) + (mask[3] << 24);
  
#if 0
    printf("network id= 0x%x\n", nw);
    printf("netmask   = 0x%x\n", nm);
#endif

    if ( (fd = open("/dev/cTask", O_RDWR)) < 0) {
      perror("setRoute: could not open /dev/cTask... ");
      exit(-1);
    }
    rc = ioctl(fd, CPL_SET_NETWORK, (unsigned long) nw);
    if ( rc ) {
      perror("setRoute: failed to set network address... "); 
      return rc; 
    }	
    rc = ioctl(fd, CPL_SET_NETMASK, (unsigned long) nm);
    if ( rc ) {
      perror("setRoute: failed to set netmask... "); 
      return rc; 
    }	
    return 0;

ERR:
    fprintf(stderr, "usage: setRoute network-addr netmask\n"); 
    fprintf(stderr, "where network-addr and netmask are in ip address\n");
    fprintf(stderr, "decimal format. eg, \"setRoute 10.10.0.0 255.255.0.0\"\n");
    exit(-1);
}
