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
** $Id: RTSCTS_ip.h,v 1.5 2001/10/12 22:38:22 pumatst Exp $
*/
#ifndef RTSCTS_IP_H
#define RTSCTS_IP_H

#include <linux/netdevice.h>
#include "RTSCTS_pkthdr.h"	/* For pkthdr_t */

#ifdef LINUX24
#define NETDEV net_device
#else
#define NETDEV device
#endif


    void handleIP(unsigned int msgID, pkthdr_t *pkthdr, char *buf,
	unsigned long len);
    void register_IPrecv(void (fun)(struct NETDEV *dev, int rcvlen, char *data),
	struct NETDEV *dev);
    void unregister_IPrecv(void);


#endif /* RTSCTS_IP_H */
