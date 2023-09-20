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
** $Id: RTSCTS_route.c,v 1.22 2002/02/26 23:15:52 jbogden Exp $
** Functions to handle route packets
*/
#include <linux/kernel.h>		/* For printk() etc. */
#include <linux/time.h>			/* For time_t in MSCshmem.h */
#include "MCPshmem.h"           	/* For MAX_NUM_ROUTES */
#include "hstshmem.h"           	/* For hstshmem */
#include "RTSCTS_proc.h"		/* For rtscts_stat */
#include "RTSCTS_ioctl.h"		/* For route_status_t */
#include "RTSCTS_send.h"		/* For sendProtoMSG() */
#include "Pkt_send.h"			/* For myrPkt_xmit(), free_snd_page() */
#include "RTSCTS_route.h"


/* >>>  ----------------------------------------------------------------- <<< */
/*
** Globals
*/

/* In doing a route check, is the result being requested by some node? */
static int route_requestor = -1;


/* >>>  ----------------------------------------------------------------- <<< */
/*
** each node has a ping-info entry for every possible
** node in the system
*/
int pINgFO[MAX_NUM_ROUTES];


/*
** For /proc/plant/routes we store the status of each route. Right now
** we support unknown (untested, no answer, bad answer (wrong dnid), okay.
*/
route_status_t route_status[MAX_NUM_ROUTES];
route_status_t route_request;


/* Count each route used for outgoing packets */
unsigned long route_used[MAX_NUM_ROUTES];


/* >>>  ----------------------------------------------------------------- <<< */

void
handlePING(int pnid)
{
    #if VERBOSE
	printk("handlePING: handling a ping...\n");
    #endif /* VERBOSE */

    pINgFO[pnid] = 1-pINgFO[pnid];
    mcpshmem->ping_stat = (int)htonl(pnid);

}  /* end of handlePING() */

void
handlePINGA(int pnid)
{
    #if VERBOSE
	printk("handlePINGA: handling a ping(r)...\n");
    #endif /* VERBOSE */

    pINgFO[pnid] = 1-pINgFO[pnid];
    mcpshmem->ping_stat = (int)htonl(pnid);

    sendProtoMSG(PING, next_msgID(), (unsigned short) pnid, 0, 0, 0, NULL);

}  /* end of handlePINGA() */

/* >>>  ----------------------------------------------------------------- <<< */

void
handleROUTE_REQ_REPLY(int rte_nid0, route_status_t status)
{
    /* rte_nid0 should indicate who set the request reply 
       we might want to check it against id of who we think
       we sent the request to...
    */
    #if VERBOSE
    printk("handleROUTE_REQ_REPLY: id of node that sent reply, %d\n", rte_nid0);
    printk("handleROUTE_REQ_REPLY: status, %d\n", status);
    #endif /* VERBOSE */

    if ( rte_nid0 != rte_nids[0] ) {
      printk("handleROUTE_REQ_REPLY: id of node handling request (%d) does not match\n", rte_nid0);
      printk("handleROUTE_REQ_REPLY: id of node we sent the request to (%d)\n", rte_nids[0]);
      status = ROUTE_CONFUSING_REPLY;
    }

    route_request = status;
    mcpshmem->route_request = (int)htonl(status);
}

/* >>>  ----------------------------------------------------------------- <<< */

void
handleRouteStat(unsigned int msgID, unsigned short src_nid,
    unsigned short expected_pnid)
{

    #ifdef VERY_VERBOSE
	printk("handleRouteStat(msgID 0x%08x, src_nid %d, expected dnid %d)\n",
	    msgID, src_nid, expected_pnid);
    #endif /* VERY_VERBOSE */

    sendRouteAck(src_nid, expected_pnid);

}  /* end of handleRouteStat() */

/* >>>  ----------------------------------------------------------------- <<< */

void
handleRouteAck(unsigned int msgID, unsigned short src_nid,
    unsigned short expected_pnid)
{

    #ifdef VERY_VERBOSE
	printk("handleRouteAck(msgID 0x%08x, src_nid %d, expected dnid %d), "
	    "my pnid %d\n", msgID, src_nid, expected_pnid, hstshmem->my_pnid);
    #endif /* VERY_VERBOSE */

    if (src_nid != expected_pnid)   {
	if (src_nid == hstshmem->my_pnid)   {
	    /* A zero route that was bent back to us */
	    if (expected_pnid < MAX_NUM_ROUTES)   {
		route_status[expected_pnid]= ROUTE_NOT_SET;
	    }
	} else   {
	    if (expected_pnid < MAX_NUM_ROUTES)   {
		route_status[expected_pnid]= ROUTE_BAD_DNID;
	    } else if (src_nid < MAX_NUM_ROUTES)   {
		/* We assume expected_pnid got corrupted & src_nid is OK */
		route_status[src_nid]= ROUTE_BAD_DNID;
	    }
	}
    } else   {
	if (expected_pnid < MAX_NUM_ROUTES)   {
	    route_status[expected_pnid]= ROUTE_OK;
	}
    }
    /* just say who sent the ack...*/
    mcpshmem->route_stat = (int)htonl(src_nid);

    if ( route_requestor != -1 ) {
	sendProtoMSG(ROUTE_REQ_REPLY, next_msgID(),
	    (unsigned short) route_requestor,
	    (unsigned int) route_status[expected_pnid], 0, 0, NULL);
	route_requestor = -1;
    }

}  /* end of handleRouteAck() */

/* >>>  ----------------------------------------------------------------- <<< */

void
handlePINGR(int pnid, char* buf, int len)
{
    #if VERBOSE
    int i;
    char *ch;
    printk("handlePINGR: handling a ping(r) from %d...\n", pnid);
    #endif /* VERBOSE */

    pINgFO[pnid] = 1-pINgFO[pnid];
    mcpshmem->ping_stat = (int)htonl(pnid);

    /* copy the requested return route into the test_route buffer */ 
    memcpy(mcpshmem->test_route, (void*)buf, len);
    mcpshmem->test_route_len = (int) htonl(len);

    //sendProtoMSG(PING, next_msgID(), (unsigned short) pnid, 0, 0, 0, NULL);

    #if VERBOSE
    ch = (char*) mcpshmem->test_route;
    for (i=0; i<len; i++) {
      printk("handlePINGR: byte %d= 0x%x\n", i, ch[i]);
    }
    #endif /* VERBOSE */

    /* ack along the requested route */
    sendProtoMSG(PING, next_msgID(), 
                 (unsigned short) TEST_ROUTE_ID, 0, 0, 0, NULL);

}  /* end of handlePINGR() */

/* >>>  ----------------------------------------------------------------- <<< */

/*
** Send a route status message
*/
int
sendRouteStat(unsigned short int dst_nid, int src_pnid)
{

/* src_pnid is supposed to represent a physical node id,
   that of a possible third party requesting the route-stat
   (i.e., we are responding to a ROUTE_REQ protocol msg),
   otherwise it should come in as -1
*/

#ifndef RTSCTS_OVER_ETHERNET
int page_idx;
#else
struct sk_buff *skb;
#endif
pkthdr_t *pkthdr;
unsigned char *data;
int msgID;
int rc;

#if 0
    printk("sendRouteStat: dst_nid= %d, src_pnid= %d\n", dst_nid, src_pnid);
#endif

    /* so when we do handleRouteAck(), we'll recall that
       there's a requestor and send them a ROUTE_REQ_REPLY
    */
    if (src_pnid != -1)   {
	route_requestor = src_pnid;
    }

    if (dst_nid >= MAX_NUM_ROUTES)   {
	return -1;
    }

    /*
    ** Get and prep a physical page
    ** 3rd arg = NULL = no Puma Portal header
    ** 5th arg = 0    = no data
    ** 8th arg = NULL = no Portal header in page
    */
    msgID= next_msgID();
#ifndef RTSCTS_OVER_ETHERNET
    page_idx= build_page(ROUTE_STAT, msgID, NULL, dst_nid, 0,
		&data, &pkthdr, NULL, ILLMSGNUM);
    if (page_idx < 0)   {
	printk("sendRouteStat() Could not allocate a page\n");
	return -1;
    }
#else
    skb= build_skb(ROUTE_STAT, msgID, NULL, dst_nid, 0,
		&data, &pkthdr, NULL, ILLMSGNUM);
#endif

    pkthdr->info= hstshmem->my_pnid;
    pkthdr->info2= dst_nid;
    route_status[dst_nid]= ROUTE_NO_ANSWER;
  
    mcpshmem->route_stat = (int)htonl(-1);

#ifndef RTSCTS_OVER_ETHERNET
    rc= myrPkt_xmit(dst_nid, page_idx, sizeof(pkthdr_t));
#else
    rc= skb_xmit(dst_nid, skb);
#endif
    if (rc < 0)   {
    #ifndef NO_ERROR_STATS
	rtscts_stat->PKTsendbad++;
    #endif /* NO_ERROR_STATS */
    } else   {
    #ifndef NO_STATS
	rtscts_stat->protoSent[ROUTE_STAT]++;
	rtscts_stat->PKTsend++;
	rtscts_stat->PKTsendlen += sizeof(pkthdr_t);
	if (sizeof(pkthdr_t) > rtscts_stat->PKTsendmaxlen)   {
	    rtscts_stat->PKTsendmaxlen= sizeof(pkthdr_t);
	}
	if (sizeof(pkthdr_t) < rtscts_stat->PKTsendminlen)   {
	    rtscts_stat->PKTsendminlen= sizeof(pkthdr_t);
	}
    #endif /* NO_STATS */
    }

    return 0;

}  /* end of sendRouteStat() */

/* >>>  ----------------------------------------------------------------- <<< */

/*
** Send a route acknowledgement message
*/
void
sendRouteAck(unsigned short dst_nid, unsigned short expected_pnid)
{

#ifndef RTSCTS_OVER_ETHERNET
int page_idx;
#else
struct sk_buff *skb;
#endif
pkthdr_t *pkthdr;
unsigned char *data;
int msgID;

int rc;


    /*
    ** Get and prep a physical page
    ** 3rd arg = NULL = no Puma Portal header
    ** 5th arg = 0    = no data
    ** 8th arg = NULL = no Portal header in page
    */
    msgID= next_msgID();
    #ifdef VERBOSE
	printk("sendRouteAck() sending msgID 0x%08x to %d (expected %d)\n",
	    msgID, dst_nid, expected_pnid);
    #endif /* VERBOSE */

#ifndef RTSCTS_OVER_ETHERNET
    page_idx= build_page(ROUTE_ACK, msgID, NULL, dst_nid, 0,
		&data, &pkthdr, NULL, ILLMSGNUM);
    if (page_idx < 0)   {
	printk("sendRouteAck() Could not allocate a page\n");
	return;
    }
#else
    skb= build_skb(ROUTE_ACK, msgID, NULL, dst_nid, 0,
		&data, &pkthdr, NULL, ILLMSGNUM);
#endif

    pkthdr->info= hstshmem->my_pnid;
    pkthdr->info2= expected_pnid;
#ifndef RTSCTS_OVER_ETHERNET
    rc= myrPkt_xmit(dst_nid, page_idx, sizeof(pkthdr_t));
#else
    rc= skb_xmit(dst_nid, skb);
#endif
    if (rc < 0)   {
    #ifndef NO_ERROR_STATS
	rtscts_stat->PKTsendbad++;
    #endif /* NO_ERROR_STATS */
    } else   {
    #ifndef NO_STATS
	rtscts_stat->protoSent[ROUTE_ACK]++;
	rtscts_stat->PKTsend++;
	rtscts_stat->PKTsendlen += sizeof(pkthdr_t);
	if (sizeof(pkthdr_t) > rtscts_stat->PKTsendmaxlen)   {
	    rtscts_stat->PKTsendmaxlen= sizeof(pkthdr_t);
	}
	if (sizeof(pkthdr_t) < rtscts_stat->PKTsendminlen)   {
	    rtscts_stat->PKTsendminlen= sizeof(pkthdr_t);
	}
    #endif /* NO_STATS */
    }

}  /* end of sendRouteAck() */

/* >>>  ----------------------------------------------------------------- <<< */
