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
/* $Id: setIface.c,v 1.1 2001/06/19 18:49:32 pumatst Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ctype.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>

#include <ptRXTX/ptRXTX.h>

int main(int argc, char *argv[] )
{
  int rc, fd;
  iface_t interface;
  
  if ( argc != 2 ) {
    fprintf(stderr, "usage: setIface interface\n"); 
    fprintf(stderr, "where interface is a string like \"eth0\"\n");
  }

  if ( (fd = open("/dev/ptRXTX", O_RDWR)) < 0) {
    perror("setIface: could not open /dev/ptRXTX... ");
    exit(-1);
  }

  strcpy(interface.name, argv[1]);

  rc = ioctl(fd, PTRXTX_SET_IFACE, &interface);
  if ( rc ) {
    perror("setIface: failed to set interface... "); 
    return rc; 
  }	
  return 0;
}
