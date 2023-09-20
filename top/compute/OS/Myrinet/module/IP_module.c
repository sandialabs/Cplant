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
** $Id: IP_module.c,v 1.9 2002/01/18 20:57:35 jbogden Exp $
**
** This module is based on code stolen from the loopback and dummy
** drivers in the Linux kernel source.
*/

#define __NO_VERSION__
#include <linux/module.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/ptrace.h>
#include <linux/ioport.h>
#include <linux/in.h>
#include <linux/malloc.h>
#include <linux/string.h>
#include <asm/system.h>
#include <asm/bitops.h>
#include <asm/dma.h>
#include <linux/errno.h>
#include <net/sock.h>
#include <linux/init.h>
#include <linux/inetdevice.h>
#include <linux/rtnetlink.h>
#include <linux/ip.h>

#include <cTask/cTask.h>
#include "IP_module.h"
#include "addr_convert.h"
#include "IP_send.h"
#include "IP_proc.h"


/*> <----------------------------------><----------------------------------> <*/
/*
** Globals
*/
static int myrIP_active= FALSE;
#ifdef LINUX24
static int stopped;
#endif


/*> <----------------------------------><----------------------------------> <*/
static void
disp_skb(struct sk_buff *skb)
{

    printk("skb at %p\n", (void *)skb);
    if (skb->dev)   {
	printk("    Device %s\n", skb->dev->name);
    } else   {
	printk("    No device\n");
    }
    printk("    len %ld\n", (long)skb->len);
    printk("    head %p\n", skb->head);
    printk("    data %p\n", skb->data);
    printk("    tail %p\n", skb->tail);
    printk("    end  %p\n", skb->end);

#ifndef LINUX24
    printk("    in use %d\n", skb->used);
#endif

}  /* end of disp_skb() */


#ifdef IP_MOD_DEBUG
static void
disp_protocol(unsigned short protocol)
{

    /* Values from linux/if_ether.h */
    protocol= htons(protocol);
    printk("Network protocol %d (0x%04x) is ", protocol, protocol);
    switch (protocol)   {
	case ETH_P_LOOP:
	    printk("Ethernet Loopback packet\n"); break;
	case ETH_P_ECHO:
	    printk("Ethernet Echo packet\n"); break;
	case ETH_P_PUP:
	    printk("Xerox PUP packet\n"); break;
	case ETH_P_IP:
	    printk("Internet Protocol packet\n"); break;
	case ETH_P_X25:
	    printk("CCITT X.25\n"); break;
	case ETH_P_ARP:
	    printk("Address Resolution packet\n"); break;
	case ETH_P_BPQ:
	    printk("G8BPQ AX.25 Ethernet Packet\n"); break;
	case ETH_P_DEC:
	    printk("DEC Assigned proto\n"); break;
	case ETH_P_DNA_DL:
	    printk("DEC DNA Dump/Load\n"); break;
	case ETH_P_DNA_RC:
	    printk("DEC DNA Remote Console\n"); break;
	case ETH_P_DNA_RT:
	    printk("DEC DNA Routing\n"); break;
	case ETH_P_LAT:
	    printk("DEC LAT\n"); break;
	case ETH_P_DIAG:
	    printk("DEC Diagnostics\n"); break;
	case ETH_P_CUST:
	    printk("DEC Customer use\n"); break;
	case ETH_P_SCA:
	    printk("DEC Systems Comms Arch\n"); break;
	case ETH_P_RARP:
	    printk("Reverse Addr Res packet\n"); break;
	case ETH_P_ATALK:
	    printk("Appletalk DDP\n"); break;
	case ETH_P_AARP:
	    printk("Appletalk AARP\n"); break;
	case ETH_P_IPX:
	    printk("IPX over DIX\n"); break;
	case ETH_P_IPV6:
	    printk("IPv6 over bluebook\n"); break;
	case ETH_P_PORTAL:
	    printk("portal protocol\n"); break;
	default:
	    printk("Unknown protocol\n"); break;
    }

}  /* end of disp_protocol() */
#endif /* IP_MOD_DEBUG */

/*> <----------------------------------><----------------------------------> <*/
/*
** From net/ipv4/utils.c
*/

/* Convert an ASCII string to binary IP. */
#ifdef CONFIG_INET
unsigned long
myrIP_in_aton(const char *str)
{

unsigned long l;
unsigned int val;
int i;


    l = 0;
    for (i = 0; i < 4; i++) {
        l <<= 8;
        if (*str != '\0') {
            val = 0;
            while (*str != '\0' && *str != '.') {
                val *= 10;
                val += *str - '0';
                str++;
            }
            l |= val;
            if (*str != '\0') 
                str++;
        }
    }
    #ifdef VERBOSE
	printk("myrIP_in_aton(str \"%s\") is %ld (0x%08lx)\n", str, htonl(l),
	    htonl(l));
    #endif /* VERBOSE */
    return(htonl(l));
}
#endif /* CONFIG_INET */

/*> <----------------------------------><----------------------------------> <*/

int
myrIP_open(struct NETDEV *dev)
{
    #ifdef VERBOSE
    printk("myrIP: open(dev name %s) my pnid is %d\n", dev->name,
	cTaskGetPhysNid());
    #endif /* VERBOSE */

    /* Get our MAC address */
    pnid2mac(dev->dev_addr, cTaskGetPhysNid());
    dev->dev_addr[ETH_ALEN]= 0;

#ifdef LINUX24
    netif_start_queue(dev);
    stopped= 0;
#else
    dev->start= 1;	/* Interface ready */
    dev->tbusy= 0;	/* transmitter not busy */
#endif

    MOD_INC_USE_COUNT;
    myrIP_active= TRUE;
    return 0;

}  /* end of myrIP_open() */


int
myrIP_close(struct NETDEV *dev)
{
    printk("myrIP: close(dev name %s)\n", dev->name);
    myrIP_active= FALSE;
#ifdef LINUX24
    netif_stop_queue(dev);
#else
    dev->tbusy= 1;
    dev->start= 0;
#endif
    MOD_DEC_USE_COUNT;
    return 0;

}  /* end of myrIP_close() */


int
myrIP_xmit(struct sk_buff *skb, struct NETDEV *dev)
{

struct iphdr *ih;
u32 *saddr, *daddr;
int rc;

  
    proc_stat->snd_attempts++;
    #ifdef VERBOSE
	printk("myrIP_xmit() skb %p, dev %p\n", skb, dev);
    #endif /* VERBOSE */

    if (skb == NULL || dev == NULL)   {
        printk("myrIP_xmit() skb %p, dev %p, myrIP_active %d\n", skb, dev,
	    myrIP_active);
	proc_stat->snd_bad_arg++;
        return 0;
    }

    if (!myrIP_active)   {
	/* device not open! */
        printk("myrIP_xmit() device not open!\n");
#ifdef LINUX24
        netif_stop_queue(dev);
        stopped= 1;
#else
	dev->tbusy= 1;
#endif
	proc_stat->snd_dev_closed++;
	dev_kfree_skb(skb);
	return -EBUSY;
    }

#ifdef LINUX24
    if (stopped) {
#else
    if (dev->tbusy)   {
#endif
        printk("myrIP_xmit() dev->tbusy\n");
	proc_stat->snd_dev_busy++;
	dev_kfree_skb(skb);
	return -EBUSY;
    }

#ifdef LINUX24
    netif_stop_queue(dev);
#else
    dev->tbusy= 1;		/* We're now busy! */
#endif
    dev->trans_start= jiffies;	/* Save the timestamp */
    ih= (struct iphdr *)((char *)skb->data + sizeof(struct ethhdr));
    saddr= &ih->saddr;
    daddr= &ih->daddr;

    /*
    ** Don't transmit the ethernet header. On the other hand, we
    ** do need the protocol field on the receive side. Therefore,
    ** we eliminate the (14 byte) ethernet header, but then back
    ** off by the size of a long to let the protocol field go over
    ** the wire.
    ** We use a long to keep skb->data aligned on a long word
    ** boundary on the receive side.
    */
    if (skb->len <= (dev->hard_header_len - sizeof(long)))   {
	printk("Not enough len (%ld) for a pull %ld!\n", (long)skb->len,
	    dev->hard_header_len - sizeof(long));
	disp_skb(skb);
	proc_stat->snd_len++;
	dev_kfree_skb(skb);
	return -EBUSY;
    }
    skb_pull(skb, dev->hard_header_len - sizeof(long));

    rc= IP_send(dev, skb, ip2pnid(*daddr), skb->data, skb->len);

    if (rc == 0)   {
	/* Send went OK */
	proc_stat->snd_xmit++;
	if (skb->list)   {
	    proc_stat->snd_skb_unlink++;
	    disp_skb(skb);
	    skb_unlink(skb);
	}
	dev_kfree_skb(skb);

	proc_stat->snd_skb_free++;
#ifndef LINUX24
	mark_bh(NET_BH);
#endif
    } else   {
	short *tmp = (short*)daddr;
	printk("IP_send() failed with IP addr %2d.%2d.%2d.%2d\n",
		ntohs(tmp[0]), ntohs(tmp[1]), ntohs(tmp[2]), ntohs(tmp[3]));
	proc_stat->snd_errors++;
	rc= -EBUSY;
    }

#ifdef LINUX24
    netif_wake_queue(dev);
    stopped= 0;
#else
    dev->tbusy= 0;
#endif
    return(rc);

}  /* end of myrIP_xmit() */


struct NETSTAT *
myrIP_get_stats(struct NETDEV *dev)
{

static struct NETSTAT __stats;
struct NETSTAT *stats;


    #ifdef VERBOSE
    printk("myrIP: myrIP_get_stats()\n");
    #endif /* VERBOSE */

    stats= &__stats;
    stats->rx_packets= proc_stat->rcv_pkt;
    stats->rx_errors= proc_stat->rcv_skb_alloc_fail + proc_stat->rcv_backoff;
    stats->rx_dropped= proc_stat->rcv_dropped + proc_stat->netif_rx_dropped +
			    proc_stat->rcv_backoff;
    stats->rx_length_errors= proc_stat->rcv_len1;
    stats->rx_fifo_errors= proc_stat->netif_rx_dropped + proc_stat->rcv_backoff;

    stats->tx_packets= proc_stat->snd_xmit;
    stats->tx_errors= proc_stat->snd_errors + proc_stat->snd_bad_arg +
	proc_stat->snd_dev_closed + proc_stat->snd_dev_busy +
	proc_stat->snd_len;
    stats->tx_dropped= proc_stat->snd_bad_arg + proc_stat->snd_dev_closed +
			    proc_stat->snd_dev_busy + proc_stat->snd_len;

    stats->rx_bytes= proc_stat->total_rcv_bytes;
    stats->tx_bytes= proc_stat->total_snd_bytes;

    return stats;

}  /* end of myrIP_get_stats() */

/*> <----------------------------------><----------------------------------> <*/
