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
** $Id: IP_send.c,v 1.5 2001/08/22 16:01:15 pumatst Exp $
** Send an IP packet over Myrinet
*/

#include <linux/kernel.h>	/* Import printk() */
#include <linux/netdevice.h>	/* Import struct device */
#include <net/sock.h>
#include "IP_send.h"
#include "IP_module.h"		/* Import MYRIP_MTU */
#include "IP_proc.h"

#ifdef LINUX24
#define NETDEV net_device
#else
#define NETDEV device
#endif

/*> <----------------------------------><----------------------------------> <*/
/*
** Externals
*/
int myr_send(struct NETDEV *dev, void *buf, int len, int dst_nid);


/*> <----------------------------------><----------------------------------> <*/
/*
** IP_send()
*/
int
IP_send(struct NETDEV *dev, struct sk_buff *skb, int dest, void *buf, int len)
{

unsigned long p1, p2;
int rc;


    #ifdef VERBOSE
    printk("IP_send(dev %s, dest %d, buf %p, len %d)\n",
	dev->name, dest, buf, len);
    #endif /* VERBOSE */

    if (len < 0)   {
	printk("IP_send() invalid length %d\n", len);
	proc_stat->snd_err_len++;
	return -1;
    }

    if (len > MYRIP_MTU)   {
	printk("Send len %d > MYRIP_MTU %d\n", len, MYRIP_MTU);
	proc_stat->snd_err_len_mtu++;
	return -1;
    }

    if (dest < 0)   {
	printk("IP_send() Invalid destination %d\n", dest);
	proc_stat->snd_err_dest++;
	return -1;
    }

    p1= (unsigned long)(skb->data) & PAGE_MASK;
    p2= (unsigned long)(skb->end) & PAGE_MASK;
    if (p1 != p2)   {
	printk("Send skb: data %p and end %p are not on same page\n",
	    skb->data, skb->end);
	proc_stat->snd_err_xpage++;
	return -1;
										    }

    rc= myr_send(dev, buf, len, dest);
    proc_stat->total_snd_bytes += len;

    return rc;

}  /* end of IP_send() */

/*> <----------------------------------><----------------------------------> <*/
