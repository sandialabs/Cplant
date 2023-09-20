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
/* $Id: getMac.c,v 1.2 2001/06/19 20:31:45 pumatst Exp $ */

/* getMac.c -- get mac address for indicated nid
   from ptRXTX module */ 
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>

#include <ptRXTX/ptRXTX.h>

int main(int argc, char *argv[] )
{
    int rc, fd; 
    mac_addr_t mac_addr;

    if ( argc != 2 ) {
      fprintf(stderr, "usage: getMac nid\n");
      return 0;
    }

    mac_addr.nid = atoi(argv[1]);

    if ( (fd = open("/dev/ptRXTX", O_RDONLY)) < 0) {
      perror("getMac: could not open /dev/ptRXTX... ");
      exit(-1);
    }

    rc = ioctl(fd, PTRXTX_GET_MAC_ADDR, &mac_addr);
    if ( rc ) {
      perror("getMac: failed to get MAC address... ");
      return rc; 
    }	

    printf("nid %d\n", mac_addr.nid);
    printf("mac= %x:%x:%x:%x:%x:%x\n",  mac_addr.byte[0], mac_addr.byte[1], 
                                       mac_addr.byte[2], mac_addr.byte[3], 
                                       mac_addr.byte[4], mac_addr.byte[5]);
    return 0;
}
