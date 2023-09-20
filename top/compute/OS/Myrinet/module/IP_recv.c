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
** $Id: IP_recv.c,v 1.7 2002/01/18 20:57:35 jbogden Exp $
** Receive an IP packet over Myrinet
*/

#include <linux/kernel.h>	/* Import printk() */
#include <linux/netdevice.h>	/* Import struct device */
#include "IP_recv.h"
#include "IP_module.h"		/* Import MYRIP_MTU */
#include "IP_proc.h"


/*> <----------------------------------><----------------------------------> <*/

struct sk_buff_head *backlog= NULL;

#ifndef VERSION_CODE
#  define VERSION_CODE(vers,rel,seq) ( ((vers)<<16) | ((rel)<<8) | (seq) )
#endif

#if LINUX_VERSION_CODE >= VERSION_CODE(2,1,79)
    #define HIGH_WATER	(netdev_max_backlog - 50)
#else
    #define HIGH_WATER	(300 - 50)
#endif

/*> <----------------------------------><----------------------------------> <*/

int
IP_init_recv(void)
{

struct sk_buff *fake_skb;
int err;


    #ifdef VERBOSE
	printk("IP_init_recv()\n");
    #endif /* VERBOSE */
    /*
    ** We need a pointer to the head of the backlog queue so we can avoid
    ** overflowing it. We submit a fake skb to netif_rx(). The skb will get
    ** put onto the backlog queue, and we can then use skb->list to get the
    ** pointer we need.
    */
    fake_skb= dev_alloc_skb(sizeof(struct sk_buff));
    if (fake_skb == NULL)   {
	/* This is bad! */
	printk("IP_init_recv() Could not get fake skb\n");
	return -1;
    }

    err = 0;
    fake_skb->dev= dev_alloc("fake_skb", &err);
    if ((fake_skb->dev == NULL) || (err < 0)) {
      printk("IP_init_recv() could not alloc device space\n");
      return -1;
    }
    fake_skb->dev->flags = 0;
#ifndef LINUX24
    fake_skb->dev->slave = NULL;
#endif
    fake_skb->protocol= 0;
    netif_rx(fake_skb);
    if (fake_skb->list == NULL)   {
	/* The skb didn't get put onto the backlog! */
	printk("IP_init_recv() fake skb did not go on backlog list\n");
	return -1;
    }

    backlog= fake_skb->list;
    /* Now, take it off again, before it causes any trouble ;-) */
    skb_dequeue(backlog);

    #ifdef VERBOSE
    printk("IP_init_recv() fake skb made it onto backlog list %p\n", backlog);
    #endif /* VERBOSE */

    return 0;

}  /* end of IP_init_recv() */

/*> <----------------------------------><----------------------------------> <*/

void
IP_rcv_start(struct NETDEV *dev, int rcvlen, char *data)
{

struct sk_buff *skb;
unsigned short proto;
unsigned int *proto_p;


    #ifdef VERBOSE
	printk("IP_rcv_start() rcvlen %d\n", rcvlen);
    #endif /* VERBOSE */

    if (backlog->qlen > HIGH_WATER)   {
	/* The backlog is getting full. No point in calling netif_rx() */
	proc_stat->rcv_backoff++;
	proc_stat->rcv_dropped++;
	return;
    }

    if (rcvlen > MYRIP_MTU)   {
	printk("Received len %d > MYRIP_MTU %d\n", rcvlen, MYRIP_MTU);
	proc_stat->rcv_len1++;
	proc_stat->rcv_dropped++;
	return;
    }


    /*
    ** There is an extra long at the start of data. The upper 16 bits of
    ** that long contain the protocol. Once we know what the protocol is,
    ** we pull the skb->data pointer up so it will be at the start of the
    ** payload.
    */
    proto_p= (unsigned int *)(data + sizeof(long) - sizeof(int));
    proto= *proto_p >> 16;

    skb= dev_alloc_skb(rcvlen + 2);
    if (skb == NULL)   {
	/* Not enough memory. We'll have to drop this packet. */
	proc_stat->rcv_skb_alloc_fail++;
	proc_stat->rcv_dropped++;
    } else   {
	proc_stat->rcv_skb_alloc++;

	memcpy(skb_put(skb, rcvlen - sizeof(long)), data + sizeof(long),
	    rcvlen - sizeof(long));
	skb->dev= dev;
	skb->protocol= proto;
	/* Silly 2.2.x kernel insists that MAC is inside skb */
	skb->mac.raw= (void *)((char *)skb->head + 2);
	skb->ip_summed= CHECKSUM_UNNECESSARY;
	skb->truesize= 0;
	proc_stat->total_rcv_bytes += skb->len;
	netif_rx(skb);
	if (skb->list == NULL)   {
	    /* netif_rx() did not enqueue skb! backlog is full. */
	    proc_stat->total_rcv_bytes -= skb->len;
	    proc_stat->netif_rx_dropped++;
	    proc_stat->rcv_dropped++;
	} else   {
	    proc_stat->rcv_pkt++;
	}
    }

    return;

}  /* end of IP_rcv_start() */

/*> <----------------------------------><----------------------------------> <*/
