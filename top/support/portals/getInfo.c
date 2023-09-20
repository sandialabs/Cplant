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
#include <fcntl.h>
#include <sys/ioctl.h>

int main(int argc __attribute__ ((unused)), char *argv[] __attribute__ ((unused)) )
{
    int	  num_nodes;

    int fdc;

    if ( (fdc = open("/dev/cTask", O_RDWR)) < 0) {
      perror("getInfo: could not open /dev/cTask... ");
      exit(-1);
    }

    printf("physical node id      %i\n",ioctl(fdc, CPL_GET_PHYS_NID, 0L ));
    printf("number physical nodes %i\n",
	    num_nodes = ioctl(fdc, CPL_GET_NUM_PHYS_NODES, 0L ));
    return 0;
}
