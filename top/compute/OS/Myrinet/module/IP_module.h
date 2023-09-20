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
** $Id: IP_module.h,v 1.4 2001/08/22 16:01:15 pumatst Exp $
**
** This module is based on code stolen from the loopback and dummy
** drivers in the Linux kernel source.
*/

#ifndef IP_MODULE_H
#define IP_MODULE_H

#include <linux/netdevice.h>	/* For struct device */
#include <linux/skbuff.h>
#include <linux/etherdevice.h>

#ifdef LINUX24
#define NETDEV net_device
#define NETSTAT net_device_stats
#else
#define NETDEV device
#define NETSTAT enet_statistics
#endif

/*> <----------------------------------><----------------------------------> <*/
/*
** Function Prototypes
*/
int myrIP_xmit(struct sk_buff *skb, struct NETDEV *dev);
struct NETSTAT *myrIP_get_stats(struct NETDEV *dev);
int myrIP_open(struct NETDEV *dev);
int myrIP_close(struct NETDEV *dev);

#ifdef CONFIG_INET
    unsigned long myrIP_in_aton(const char *str);
#endif /* CONFIG_INET */

/*> <----------------------------------><----------------------------------> <*/
/*
** Macros and defines
*/

/*
** We want the MTU to be the same independent of kernel version and
** sk_buff size. So we just set it to 7920 which works.
*/
#define MYRIP_MTU	(7920)


#ifndef FALSE
    #define FALSE	(0)
#endif /* FALSE */
#ifndef TRUE
    #define TRUE	(1)
#endif /* TRUE */


#endif /* IP_MODULE_H */
