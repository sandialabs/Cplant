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
/* $Id: setMac.c,v 1.3 2001/06/26 00:10:31 pumatst Exp $ */

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
  int rc, nid; 
  int fd;	
  mac_addr_t mac_addr;
  int ints[6];

  if ( argc != 3 ) {
    fprintf(stderr, "usage: setMac nid mac-addr\n"); 
    fprintf(stderr, "where mac-addr is a 6-element colon\n");
    fprintf(stderr, "separated list of pairs of hex digits\n");
    exit(-1);
  }

  if ( (fd = open("/dev/ptRXTX", O_RDWR)) < 0) {
    perror("setMac: could not open /dev/ptRXTX... ");
    exit(-1);
  }

  nid = strtol(argv[1], NULL, 10);

  /* verify mac byte format and store */
    if ( !isxdigit(argv[2][0]) || !isxdigit(argv[2][1]) ||
         argv[2][2] != ':' || 
         !isxdigit(argv[2][3]) || !isxdigit(argv[2][4]) ||
         argv[2][5] != ':' || 
         !isxdigit(argv[2][6]) || !isxdigit(argv[2][7]) ||
         argv[2][8] != ':' || 
         !isxdigit(argv[2][9]) || !isxdigit(argv[2][10]) ||
         argv[2][11] != ':' || 
         !isxdigit(argv[2][12]) || !isxdigit(argv[2][13]) ||
         argv[2][14] != ':' || 
         !isxdigit(argv[2][15]) || !isxdigit(argv[2][16]) ) {
      printf("setMac: mac byte format error -- use colon-separated\n");
      printf("setMac: list of pairs of hex digits...\n");
      exit(-1);
    } 

    sscanf( argv[2], "%x:%x:%x:%x:%x:%x",
                &ints[0], &ints[1], &ints[2],
                &ints[3], &ints[4], &ints[5]);

/* cpq2 eth0  Link encap:Ethernet  HWaddr 00:00:F8:76:63:72 */
/* satch eth0  Link encap:Ethernet  HWaddr 00:00:F8:75:58:5B */

    mac_addr.nid = nid;
    mac_addr.byte[0] = (unsigned char) ints[0];
    mac_addr.byte[1] = (unsigned char) ints[1];
    mac_addr.byte[2] = (unsigned char) ints[2];
    mac_addr.byte[3] = (unsigned char) ints[3];
    mac_addr.byte[4] = (unsigned char) ints[4];
    mac_addr.byte[5] = (unsigned char) ints[5];

    rc = ioctl(fd, PTRXTX_SET_MAC_ADDR, &mac_addr);
    if ( rc ) {
      perror("setMac: failed to set MAC address... "); 
      return rc; 
    }	
    return 0;
}
