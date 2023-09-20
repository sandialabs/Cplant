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
** $Id: addr_convert.h,v 1.6 2001/09/30 21:25:10 pumatst Exp $
** Translate a MAC address into a physical node ID
*/

#ifndef ADDR_CONVERT_H
#define ADDR_CONVERT_H

#include <linux/skbuff.h>	/* For struct sk_buff */

#ifdef LINUX24
#define NETDEV net_device
#else
#define NETDEV device
#endif

int mac2pnid(unsigned char *mac);
void pnid2mac(unsigned char *mac, int pnid);
int pnid2ip(int pnid);
int ip2pnid(unsigned int ip);
int ip2mac(void *buff, struct NETDEV *dev, unsigned int dest,
	    struct sk_buff *skb);

#endif /* ADDR_CONVERT_H */
