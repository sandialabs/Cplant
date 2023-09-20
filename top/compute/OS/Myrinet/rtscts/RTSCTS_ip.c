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
** $Id: RTSCTS_ip.c,v 1.12 2002/02/26 23:15:52 jbogden Exp $
** Functions to handle IP packets
*/
#include <linux/kernel.h>		/* For printk() etc. */
#include <linux/netdevice.h>		/* For struct device */
#include <sys/defines.h>                /* For MIN() */
#include "RTSCTS_proc.h"		/* For rtscts_stat_t() */
#include "RTSCTS_protocol.h"		/* For show_msgID() */
#include "RTSCTS_send.h"		/* For build_page() */
#include "Pkt_send.h"			/* For free_snd_page(), myrPkt_xmit() */
#include "Pkt_module.h"			/* for MYRPKT_MTU */
#include "hstshmem.h"			/* For hstshmem */
#include "MCPshmem.h"			/* For mcpshmem */
#include "RTSCTS_ip.h"

/* >>>  ----------------------------------------------------------------- <<< */
/*
** Globals
*/
static struct NETDEV *IP_rcv_dev= NULL;
static void (*IP_rcv_func)(struct NETDEV *dev, int rcvlen, char *data) = NULL;

/* >>>  ----------------------------------------------------------------- <<< */

void
register_IPrecv(void (fun)(struct NETDEV *dev, int rcvlen, char *data),
	struct NETDEV *dev)
{
    printk("registering IP recv function %p, dev \"%s\"\n", fun, dev->name);
    IP_rcv_func= fun;
    IP_rcv_dev= dev;

}  /* end of register_IPrecv() */

/* >>>  ----------------------------------------------------------------- <<< */

void
unregister_IPrecv(void)
{
    printk("unregistering IP recv function %p\n", IP_rcv_func);
    IP_rcv_func= NULL;

}  /* end of unregister_IPrecv() */

/* >>>  ----------------------------------------------------------------- <<< */

void
handleIP(unsigned int msgID, pkthdr_t *pkthdr, char *buf, unsigned long len)
{

struct NETDEV *dev;


    dev= IP_rcv_dev;

    #ifdef VERBOSE
	printk("handleIP() %s, len %ld, dev %s\n", show_msgID(msgID), len,
	    dev->name);
    #endif /* VERBOSE */

    #ifndef NO_STATS
    rtscts_stat->IPrecv++;
    rtscts_stat->IPrecvlen += len;
    if (len > rtscts_stat->IPrecvmaxlen)   {
	rtscts_stat->IPrecvmaxlen= len;
    }
    if (len < rtscts_stat->IPrecvminlen)   {
	rtscts_stat->IPrecvminlen= len;
    }
    #endif /* NO_STATS */

    if (IP_rcv_func != NULL)   {
	(IP_rcv_func)(dev, len, buf);
    }

}  /* end of handleIP() */

/* >>>  ----------------------------------------------------------------- <<< */
/*
** Send something other than a Portals message. Currently used by the IP
** over Myrinet module.
*/
int
myr_send(struct NETDEV *dev, void *buf, int len, int dst_nid)
{

unsigned long flags;
#ifndef RTSCTS_OVER_ETHERNET
int page_idx;
#else
struct sk_buff *skb;
#endif
int pktlen;
unsigned int payload;
unsigned int headers;
pkthdr_t *pkthdr_start;
unsigned char *data_start;
int rc;


    if (mcpshmem == NULL)   {
	printk("myr_send: MCP is not loaded\n");
	return -1;
    }

    save_flags(flags);
    cli();

    #ifndef NO_STATS
    /* Keep stats */
    rtscts_stat->IPsend++;
    if (len > rtscts_stat->IPsendmaxlen)   {
	rtscts_stat->IPsendmaxlen= len;
    }
    if (len < rtscts_stat->IPsendminlen)   {
	rtscts_stat->IPsendminlen= len;
    }
    #endif /* NO_STATS */


    /*
    ** Error checking on parameters
    */
    if (len < 0)   {
	printk("myr_send() Invalid length %d\n", len);
    #ifndef NO_ERROR_STATS
	rtscts_stat->sendErr6++;
    #endif /* NO_ERROR_STATS */
	restore_flags(flags);
	return -1;
    }


    /*
    ** Everything OK so far. Build IP packet. It consists of the
    ** the packet header, and data (including headers from higher
    ** level protocols).
    **
    ** +----------+------   ---+
    ** | pkthdr_t | data       |
    ** +----------+------   ---+
    **  ^          ^
    **  |          |
    **  |          +--- data_start
    **  +--- pkthdr_start
    */
    headers= sizeof(pkthdr_t);
    payload= MYRPKT_MTU - headers;
    payload= MIN(payload, len);
    pktlen= headers + payload;

    if (payload != len)   {
	printk("myr_send() IP pkt len %d > payload %d\n", len, payload);
    #ifndef NO_ERROR_STATS
	rtscts_stat->sendErr7++;
    #endif /* NO_ERROR_STATS */
	restore_flags(flags);
	return -2;
    }

    /* Get and prep a physical page */
#ifndef RTSCTS_OVER_ETHERNET
    page_idx= build_page(IP, next_msgID(), NULL, dst_nid, payload, &data_start,
	&pkthdr_start, NULL, ILLMSGNUM);
    if (page_idx < 0)   {
	restore_flags(flags);
	return -3;
    }
#else
    skb= build_skb(IP, next_msgID(), NULL, dst_nid, payload, &data_start,
	&pkthdr_start, NULL, ILLMSGNUM);
#endif

    if (payload > 0)   {
	if (memcpy(data_start, buf, payload) == NULL)   {
        #ifndef NO_ERROR_STATS
	    rtscts_stat->badcpy++;
        #endif /* NO_ERROR_STATS */
	    #ifdef VERBOSE
		printk("myr_send() memcpy(%p, %p, %d) failed\n", data_start,
		    buf, payload);
	    #endif /* VERBOSE */
	    send_sig(SIGPIPE, current, 1);
#ifndef RTSCTS_OVER_ETHERNET
	    free_snd_page(page_idx);
#else
            kfree_skb(skb);
#endif
	    restore_flags(flags);
	    return -4;
	}
    }

#ifndef RTSCTS_OVER_ETHERNET
    rc= myrPkt_xmit(dst_nid, page_idx, pktlen);
#else
    rc= skb_xmit(dst_nid, skb);
#endif
    if (rc < 0)   {
	/* The packet couldn't be sent! */
    #ifndef NO_ERROR_STATS
	rtscts_stat->PKTsendbad++;
    #endif /* NO_ERROR_STATS */
	restore_flags(flags);
	return -5;
    } 

    /* It's gone out! */
    
    #ifndef NO_STATS
    rtscts_stat->IPsendlen+= len;
    rtscts_stat->protoSent[IP]++;
    rtscts_stat->PKTsend++;
    rtscts_stat->PKTsendlen += pktlen;
    if (pktlen > rtscts_stat->PKTsendmaxlen)   {
	rtscts_stat->PKTsendmaxlen= pktlen;
    }
    if (pktlen < rtscts_stat->PKTsendminlen)   {
	rtscts_stat->PKTsendminlen= pktlen;
    }
    #endif /* NO_STATS */
    
    restore_flags(flags);
    return 0;

}  /* end of myr_send() */

/* >>>  ----------------------------------------------------------------- <<< */
