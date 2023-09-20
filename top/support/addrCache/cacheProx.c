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

/* check for arrival of new validations info.
   print info to stdout
*/

int main(int argc, char *argv[] )
{
    int rc, i; 
    int fd;	
    addr_entry_t addrlist[ADDR_LIMIT];
    addr_summary_t addrSummary;

#if 0
    for (i=0; i<ADDR_LIMIT; i++) {
      addrlist[i].addr = 0;
      addrlist[i].len  = 0;
    }
#endif

    if ( (fd = open("/dev/rtscts", O_RDWR)) < 0) {
      perror("cache: could not open /dev/rtscts... ");
      return(-1);
    }

    /* check for new validations */
    rc = ioctl(fd, RTS_CACHE_RETRIEVE_DATA, &addrSummary);

    if (!rc) { /* got one */
#if 0
      i=0;
      while( addrlist[i].len ) {
        fprintf(stdout, "0x%lx, %d\n", addrlist[i].addr, 
                                       addrlist[i].len ); 
        i++;
      }
#endif
      printf("name: %s\n", addrSummary.name);
      for (i=0; i<NUM_SUBCACHES; i++) {
        printf("# validations for subCache[%d]= %d\n", i, addrSummary.numvals[i]);
      }
    }
    return 0;
}
