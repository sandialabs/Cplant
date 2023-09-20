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
#include <cTask/cTask.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>

int main(int argc, char *argv[] )
{
    int retval; 
    int fd;	

    if ( argc != 2 ) {
	printf("usage: %s node_id\n",argv[0]);
	exit( 1 );
    }

    if ( (fd = open("/dev/cTask", O_RDWR)) < 0) {
      perror("setNid: could not open /dev/cTask... ");
      exit(-1);
    }
    retval = ioctl(fd, CPL_SET_PHYS_NID, (unsigned long) atoi(argv[1]) );

    if ( retval != 0 ) {
	perror("setNid: failed to set physical node id");
    }	
    return retval; 
}
