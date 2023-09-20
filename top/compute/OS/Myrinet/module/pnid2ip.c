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
/* $Id: pnid2ip.c,v 1.2 2001/09/30 21:25:10 pumatst Exp $ */

/* given a nework id, netmask, and physical node id this utility 
   is used to translate the physical node id to a decimal dotted 
   ip address by
   
        1) first checking that the (node id +1) does not overflow 
           the complement of the netmask 

        2) adding (node id +1) to the network id

   usage: % pnid2ip network netmask pnid
   where network and netmask are in decimal dotted ip notation:

             % pnid2ip 10.10.0.0 255.255.0.0 198

   for example.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ctype.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>

int main(int argc, char *argv[] )
{
  int rc;
  int fd;	
  unsigned int net[4]={0,0,0,0};
  unsigned int mask[4]={0,0,0,0}; 
  unsigned int ip;
  unsigned int nw=0, nm=0;
  unsigned int pnid;

  if ( argc != 4 ) {
    fprintf(stderr, "usage: pnid2ip network-addr netmask pnid\n"); 
    fprintf(stderr, "where network-addr and netmask are in ip address\n");
    fprintf(stderr, "decimal format. eg, \"pnid2ip 10.10.0.0 255.255.0.0 189\"\n");
    exit(-1);
  }
    rc = sscanf( argv[1], "%d.%d.%d.%d",
                 &net[3], &net[2], &net[1], &net[0]);
    if (rc != 4) goto ERR;

    rc = sscanf( argv[2], "%d.%d.%d.%d",
                 &mask[3], &mask[2], &mask[1], &mask[0]);
    if (rc != 4) goto ERR;

    rc = sscanf( argv[3], "%d", &pnid);
    if (rc != 1) goto ERR;

    nw = net[0] + 
             (net[1] << 8) + (net[2] << 16) + (net[3] << 24);

    nm = mask[0] + 
             (mask[1] << 8) + (mask[2] << 16) + (mask[3] << 24);
  
#if 0
    printf("network id= 0x%x\n", nw);
    printf("netmask   = 0x%x\n", nm);
#endif

    if ( pnid < 0 ) {
      fprintf(stderr, "pnid2ip: node id must be >= 0\n");
      exit -1;
    }
    pnid++;
    if ( pnid > ~nm ) {
      fprintf(stderr, "pnid2ip: pnid+1 0x%x overflows ~netmask 0x%x\n",
                       pnid, ~nm);
      exit -1;
    }
    ip = nw + pnid;

    printf("%d.%d.%d.%d", (ip >> 24) & 0xff, (ip >> 16) & 0xff,
                          (ip >> 8 ) & 0xff,  ip & 0xff);

    return 0;

ERR:
    fprintf(stderr, "usage: pnid2ip network-addr netmask pnid\n"); 
    fprintf(stderr, "where network-addr and netmask are in ip address\n");
    fprintf(stderr, "decimal format. eg, \"pnid2ip 10.10.0.0 255.255.0.0 189\"\n");
    exit(-1);

}
