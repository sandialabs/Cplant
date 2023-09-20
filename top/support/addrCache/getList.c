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

#include <sys/types.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>

#include <cache/cache.h>

/* get copy current list of addr validations 
   into user space and display
*/

int main(int argc, char *argv[] )
{
    int rc, i; 
    int fd;	
    addr_entry_t addrlist[ADDR_LIMIT];

    for (i=0; i<ADDR_LIMIT; i++) {
      addrlist[i].addr = 0;
      addrlist[i].len  = 0;
    }

    if ( (fd = open("/dev/addrCache", O_RDWR)) < 0) {
      perror("getList: could not open /dev/addrCache... ");
      exit(-1);
    }
    rc = ioctl(fd, CACHE_GET_ADDR_LIST, addrlist);

    if ( rc ) {
      perror("getList: failed to get addrlist... "); 
      return rc; 
    }	

    i=0;
    while( addrlist[i].len ) {
      printf("%d: 0x%lx   %d\n", i, addrlist[i].addr, addrlist[i].len);
      i++;
    }
    return 0;
}
