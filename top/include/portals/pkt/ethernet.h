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
/*
** $Id: ethernet.h,v 1.1 1997/11/05 01:58:56 mjleven Exp $
*/


#ifndef ETHERNET_H
#define ETHERNET_H

#include <linux/netdevice.h>
#include <linux/kernel.h>
#include <asm/byteorder.h>

static inline void printEthernetHdr( char *data )
{
    short *tmp = (short*) data;
    printk( "802.3 hdr 0x%p\n", data );
    printk("dst addr: 0x%4.4x%4.4x%4.4x\n",
		ntohs(tmp[0]), ntohs(tmp[1]),ntohs(tmp[2]));
    printk("src addr: 0x%4.4x%4.4x%4.4x\n",ntohs(tmp[3]), 
		ntohs(tmp[4]),ntohs(tmp[5]));
    printk("type    : 0x%4.4x\n",ntohs(tmp[6]));
}

#endif
