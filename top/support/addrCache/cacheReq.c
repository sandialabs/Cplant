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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>

#include <cache/cache.h>
#include <Myrinet/rtscts/RTSCTS_ioctl.h>

/* register this node as one to send lists of 
   validated addresses to. 
*/

int main(int argc, char *argv[] )
{
    int rc, nid; 
    int fd;	

    if ( argc != 2) {
      fprintf(stderr, "usage: %s node-id\n", argv[0]);
      exit(0);
    }

    nid = atoi(argv[1]);

    if ( (fd = open("/dev/rtscts", O_RDWR)) < 0) {
      perror("cacheReq: could not open /dev/rtscts... ");
      exit(-1);
    }

    /* register w/ indicated node */
    rc = ioctl(fd, RTS_CACHE_REQ, nid);
    if (rc) {
      perror("cacheReq: RTS_CACHE_REQ ioctl failed"); 
    }

    return 0;
}
