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
** $Id: IP_send.h,v 1.3 2001/08/22 16:01:15 pumatst Exp $
** Send an IP packet over Myrinet
*/

#ifndef IP_SEND_H
#define IP_SEND_H

#ifdef LINUX24
#define NETDEV net_device
#else
#define NETDEV device
#endif

    int IP_send(struct NETDEV *dev, struct sk_buff *skb, int dest, void *buf,
	    int len);

#endif /* IP_SEND_H */
