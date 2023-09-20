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
/* $Id: ptRXTX.h,v 1.3 2001/10/23 22:30:46 jsotto Exp $ */

#ifndef _PT_RXTX_
#define _PT_RXTX_

#define PTRXTX_MAJOR 65
#define PORTALS_PROT_ID     0X700a
#define NODES               8009     /* # of nodes in the group */

enum{ PTRXTX_SET_MAC_ADDR, PTRXTX_GET_MAC_ADDR,
      PTRXTX_SET_IFACE, PTRXTX_GET_IFACE};

typedef struct {
  int nid;
  unsigned char byte[6];
} mac_addr_t;

typedef struct {
  char name[100];
  int len;
} iface_t;

#endif
