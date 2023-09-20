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
** $Id: Pkt_recv.c,v 1.47.2.1 2002/05/22 20:49:30 jbogden Exp $
** Receive a packet over Myrinet
*/

#include <linux/mm.h>		/* Import GFP_ATOMIC, get_free_page() */
#include <linux/kernel.h>	/* Import printk() */
#include <linux/netdevice.h>	/* Import struct device */
#include <asm/io.h>		/* Import virt_to_bus() */
#include <asm/byteorder.h>	/* Import htonl(), ntohl() */

#include <sys/defines.h>
#include "MCPshmem.h"
#include "hstshmem.h"
#include "RTSCTS_recv.h"	/* Import rtscts_recv(), reset_Qentry() */
#include "RTSCTS_send.h"	/* Import sendProtoMSG(), next_msgID() */
#include "RTSCTS_protocol.h"	/* Import pkthdr_t() */
#include "Pkt_recv.h"
#include "Pkt_module.h"		/* Import MYRPKT_MTU */
#include "Pkt_proc.h"
#include "printf.h"         /* for PRINTF() macro */

#ifdef LINUX24
#define NETDEV net_device
#else
#define NETDEV device
#endif

/******************************************************************************/
/*
** Globals
*/
static int last_pkt= 0;
static unsigned long page_list[MAX_RCV_PKT_ENTRIES];

/*
** Local functions
*/
static int check_hdr(pkthdr_t *hdr, int *warning);
static int do_recovery(pkthdr_t *hdr);


/******************************************************************************/

#ifdef BUFDEBUG
/*
** Return a pointer to the page that was received "back" packet receives ago.
** Return NULL if there is no such page. Used by /proc/rcvbuf.
*/
unsigned long *
getRcvPage(int back)
{

int idx;


    if ((last_pkt - back) < 0)   {
	/* Wrap around */
	idx= MAX_RCV_PKT_ENTRIES + (last_pkt - back);
    } else   {
	idx= last_pkt - back;
    }

    return (unsigned long *)page_list[idx];

}  /* end of getRcvPage() */
#endif /* BUFDEBUG */

/******************************************************************************/
int
pkt_init_recv(void)
{

int i;
static int first_time= TRUE;


    if (first_time)   {
	/* Allocate the pages */
	for (i= 0; i < MAX_RCV_PKT_ENTRIES; i++)   {
	    page_list[i]= get_free_page(GFP_ATOMIC);
	    if (page_list[i] == 0L)   {
		printk("Could only get %d (out of %d) receive pages!\n", i,
		    MAX_RCV_PKT_ENTRIES);
		pkt_free_rcv_pages();
		proc_stat->rcv_page_alloc_fail++;
		return -1;
	    }
	    proc_stat->rcv_page_alloc++;
	}
	PRINTF(2)("pkt_init_recv() pre-allocated %d recv pages\n", MAX_RCV_PKT_ENTRIES);
	first_time= FALSE;
    }

    /* Now tell the MCP what the physcial addresses are */
    for (i= 0; i < MAX_RCV_PKT_ENTRIES; i++)   {
	mcpshmem->rcv_pkt_list[i].phys_addr=
	    htonl(virt_to_bus((void *)page_list[i]));
	mcpshmem->rcv_pkt_list[i].len= -1;
    }
    last_pkt= 0;

    return 0;

}  /* end of pkt_init_recv() */

/******************************************************************************/

void
pkt_free_rcv_pages(void)
{

int i;
int cnt;


    cnt= 0;
    for (i= 0; i < MAX_RCV_PKT_ENTRIES; i++)   {
	if (page_list[i] != 0L)   {
	    free_page(page_list[i]);
	    cnt++;
	    page_list[i]= 0L;
	    proc_stat->rcv_page_free++;
	}
	if (mcpshmem != NULL)   {
	    mcpshmem->rcv_pkt_list[i].phys_addr= NO_PAGE;
	}
    }
    printk("pkt_free_rcv_pages() freed %d recv pages\n", cnt);

}  /* end of pkt_free_rcv_pages() */

/******************************************************************************/

void
Pkt_rcvLANai_start(struct NETDEV *dev)
{

int escape_cnt;
int rcvlen;
int error;
int warning;
int recover;
static unsigned int num_pkts= 0;
int num_restarts;
pkthdr_t *pkthdr;


    num_restarts= 0;
restart:
    mb();

    /*
    ** Setting error means not to pass this packet up to the upper layers.
    ** Setting recover means to take appropriate action because this packet
    ** is bad.
    */
    recover= FALSE;
    error= FALSE;

    /* Make sure the Packet is already here. */
    escape_cnt= 0;
    pkthdr= (pkthdr_t *)page_list[last_pkt];
    while (TRUE)   {
	if (ntohl(mcpshmem->rcv_pkt_list[last_pkt].phys_addr) == GOOD_PKT)   {
	    error= FALSE;
	    break;
	} else if (ntohl(mcpshmem->rcv_pkt_list[last_pkt].phys_addr) ==
		LONG_PKT)   {
	    /* MCP got a pkt that was longer than the buffer we handed down. */
	    #ifdef VERBOSE
		printk("Long packet received!\n");
		disp_pkthdr(pkthdr);
	    #endif /* VERBOSE */
	    error= TRUE;
	    #ifdef DO_TIMEOUT_PROTOCOL
		recover= FALSE;
	    #else
		recover= TRUE;
	    #endif
        #ifndef NO_ERROR_STATS
	    proc_stat->rcv_dropped++;
	    proc_stat->rcv_len3++;
        #endif /* NO_STATS */
	    break;
	} else if (ntohl(mcpshmem->rcv_pkt_list[last_pkt].phys_addr) ==
		BAD_CRC_PKT)   {
	    /* Packet had a bad CRC */
	    #ifdef VERBOSE
		printk("Packet with bad CRC received!\n");
		disp_pkthdr(pkthdr);
	    #endif /* VERBOSE */
	    error= TRUE;
	    recover= TRUE;
        #ifndef NO_ERROR_STATS
	    proc_stat->rcv_dropped++;
	    proc_stat->rcv_dropped_crc++;
        #endif /* NO_ERROR_STATS */
	    break;
	} else if (ntohl(mcpshmem->rcv_pkt_list[last_pkt].phys_addr) ==
		TRUNC_PKT)   {
	    /* Packet was truncated */
	    #ifdef VERBOSE
		printk("Truncated packet received!\n");
		disp_pkthdr(pkthdr);
	    #endif /* VERBOSE */
	    error= TRUE;
	    recover= FALSE;
        #ifndef NO_ERROR_STATS
	    proc_stat->rcv_dropped++;
	    proc_stat->rcv_dropped_trunc++;
        #endif /* NO_ERROR_STATS */
	    break;
	}
	if (escape_cnt++ > 100)   {
        #ifndef NO_ERROR_STATS
	    proc_stat->rcv_timeout++;
        #endif /* NO_ERROR_STATS */
	    return;
	}
    } 


    mb();
    rcvlen= ntohl(mcpshmem->rcv_pkt_list[last_pkt].len);

    /* The packet can't be larger than the MTU! */
    if (!error && (rcvlen > MYRPKT_MTU))   {
	printk("Received len %d > MYRPKT_MTU %ld\n", rcvlen, MYRPKT_MTU);
	error= TRUE;
	recover= TRUE;
    #ifndef NO_ERROR_STATS
	proc_stat->rcv_dropped++;
	proc_stat->rcv_len1++;
    #endif /* NO_ERROR_STATS */
    }

    /* The packet needs to be at least as long as the packet header */
    if (!error && (rcvlen < sizeof(pkthdr_t)))   {
	#ifdef VERBOSE
	    printk("Received len %d < %ld\n", rcvlen, sizeof(pkthdr_t));
	#endif /* VERBOSE */
	error= TRUE;
	recover= FALSE;
    #ifndef NO_ERROR_STATS
	proc_stat->rcv_dropped++;
	proc_stat->rcv_len4++;
    #endif /* NO_ERROR_STATS */
    }

    /* The out-of-band Myrinet length should match the length encoded in the packet header */
    if (!error && (rcvlen != pkthdr->len1))   {
	printk("Received len %d != pkt->len %d\n", rcvlen, pkthdr->len1);
	disp_pkthdr(pkthdr);
	error= TRUE;
	recover= FALSE;
    #ifndef NO_ERROR_STATS
	proc_stat->rcv_dropped++;
	proc_stat->rcv_len5++;
    #endif /* NO_ERROR_STATS */
    }

    /* Make sure the sender speaks the same protocol */
    if (!error && (pkthdr->version != (RTSCTS_PROTOCOL_version | MCP_version))){
	printk("Received packet with wrong version 0x%08x != 0x%08x, len %d\n",
	    pkthdr->version, RTSCTS_PROTOCOL_version | MCP_version, rcvlen);
	printk("    num_restarts %d, last_pkt %d, phys addr 0x%08lx\n",
	    num_restarts, last_pkt,
	    (unsigned long)ntohl(mcpshmem->rcv_pkt_list[last_pkt].phys_addr));
	if ((pkthdr->version & 0xffff0000) != RTSCTS_PROTOCOL_version)   {
	    /* We really only care about the protocol version, not the MCP */
	    error= TRUE;
	    recover= FALSE;
	}
    }


    #undef FAULT_INJECTION
    #ifdef FAULT_INJECTION
    /*
    ** Randomly select packets and mark them as bad. This will slow
    ** down receives because of the call to get_random_bytes() for
    ** each packet.
    ** If you want more faulty packets, lower the number of bits
    ** that have to match below. E.g. ((rnd & 0x00000fff) == 0x00000aaa)
    ** If you want fewer faulty packets (larger number of nodes on a
    ** data intensive application), then up the number of bits that
    ** have to match: e.g. ((rnd & 0x03ffffff) == 0x03aaaaaa)
    */
    {
	#include <linux/random.h>
	unsigned int rnd;

	get_random_bytes(&rnd, sizeof(rnd));
	if ((rnd & 0x000fffff) == 0x000aaaaa)   {
	    printk("Injecting fault into a packet of type %2d! rnd is "
		"0x%08x\n", pkthdr->type, rnd);
	    error= TRUE;
	    recover= TRUE;
	}
    }
    #endif /* FAULT_INJECTION */


    /* Check the integrity of the header */
    if (!error)   {
	if (check_hdr(pkthdr, &warning) != 0)   {
	    /*
	    ** Header was corrupted and could not be fixed. We can't
	    ** request that packet again!
	    */
	    error= TRUE;
	    recover= FALSE;
        #ifndef NO_ERROR_STATS
	    proc_stat->rcv_dropped++;
        #endif /* NO_ERROR_STATS */
	} else if (warning > 0)   {
	    /*
	    ** Even if we were able to fix the header, we don't trust
	    ** the rest of the packet. Take corrective action.
	    */
	    recover= TRUE;
	}
    }

    /*
    ** We have decided that a recovery is needed. We may have had to
    ** fix the header, but it was successful (for type, src, and msgID).
    ** Depending on the packet type, we need to decide whether we can
    ** have a packet resent. do_recovery() does that.
    */
    if (recover)   {
	error= do_recovery(pkthdr);
    }

    if (!error)   {
	/* Pass it on up! */
    #ifndef NO_STATS
	hstshmem->total_rcv_bytes += rcvlen;
    #endif /* NO_STATS */

	rtscts_recv(page_list[last_pkt], dev);
    #ifndef NO_STATS
	proc_stat->rcv_rtscts++;
    #endif /* NO_STATS */

	#ifdef BUFDEBUG
    /* Mark the packet as having been processed */
    pkthdr->version= 0x80000000;
	#endif /* BUFDEBUG */
    }

    /*
    ** Clear the header portion of the packet to make sure we detect
    ** bad ones. To make /proc/cplant/rcvbuf work, copy the packet
    ** header first into the body of the message, right behind the
    ** original location of the header.
    */
    #ifdef PKTHDR_CLEAR
	#ifdef BUFDEBUG
	    memcpy((pkthdr + 1), pkthdr, sizeof(pkthdr_t));
	#endif /* BUFDEBUG */
	memset((void *)page_list[last_pkt], 0, sizeof(pkthdr_t));
    #endif /* PKTHDR_CLEAR */

    /* Tell the MCP to reuse the page */
    mcpshmem->rcv_pkt_list[last_pkt].phys_addr=
	htonl(virt_to_bus((void *)page_list[last_pkt]));
    mcpshmem->rcv_pkt_list[last_pkt].len= htonl(MYRPKT_MTU);

    num_pkts++;
    mcpshmem->num_rcv_pkts= htonl(num_pkts);
    mb();

    last_pkt= (last_pkt + 1) % MAX_RCV_PKT_ENTRIES;

    /* See if the next one is already here */
    if ((mcpshmem->rcv_pkt_list[last_pkt].phys_addr == GOOD_PKT) ||
	    (ntohl(mcpshmem->rcv_pkt_list[last_pkt].phys_addr) == LONG_PKT) ||
	    (ntohl(mcpshmem->rcv_pkt_list[last_pkt].phys_addr) == BAD_CRC_PKT)||
	    (ntohl(mcpshmem->rcv_pkt_list[last_pkt].phys_addr) == TRUNC_PKT))  {
	num_restarts++;
	goto restart;
    }

    #ifndef NO_STATS
    if (num_restarts > 0)   {
	/* Count how many times we handled more than 1 packet per interrupt */
	proc_stat->rcv_restart++;
    }
    #endif /* NO_STATS */
    
    num_restarts++;
    
    #ifndef NO_STATS
    if (num_restarts > proc_stat->rcv_max_restart)   {
	/* Record the largest number of packets handled in a single interrupt */
	proc_stat->rcv_max_restart= num_restarts;
    }
    #endif /* NO_STATS */
    
    return;

}  /* end of Pkt_rcvLANai_start() */

/* ************************************************************************** */

void
disp_pkthdr(pkthdr_t *hdr)
{

    printk("Packet Header at %p\n", hdr);
    printk("    version    0x%08x\n", hdr->version);
    printk("    type/src   0x%04x%04x\n", hdr->type, hdr->src_nid);
    printk("    len1/len2  0x%04x%04x (%d bytes)\n", hdr->len1, hdr->len2,
	hdr->len1);
    printk("    msgID      0x%08x\n", hdr->msgID);
    printk("    type2/src2 0x%04x%04x\n", hdr->type2, hdr->src_nid2);
    printk("    msgID2     0x%08x\n", hdr->msgID2);
    printk("    seq        0x%08x\n", hdr->seq);
    printk("    info       0x%08x\n", hdr->info);
    printk("    info2      0x%08x\n", hdr->info2);
    printk("    msgNum     0x%08x\n", hdr->msgNum);
    printk("    type3/src3 0x%04x%04x\n", hdr->type3, hdr->src_nid3);
    printk("    msgID3     0x%08x\n", hdr->msgID3);

}  /* end of disp_pkthdr() */

/* ************************************************************************** */
/*
** Check and fix header
*/
static int
check_hdr(pkthdr_t *hdr, int *warning)
{

int error;
unsigned int snid;


    error= 0;
    *warning= 0;
    mb();

    /*
    ** Check the length
    */
    if (hdr->len1 != hdr->len2)   {
	/* Can't fix it! */
	printk("CORRUPTION: length bad. Can't fix it!\n");
	if (!error && !*warning)   disp_pkthdr(hdr);
	error++;
    #ifndef NO_ERROR_STATS
	proc_stat->hdr_err_len++;
    #endif /* NO_ERROR_STATS */
    }

    /*
    ** Check the type
    */
    if ((hdr->type != hdr->type2) || (hdr->type != hdr->type3) ||
	    (hdr->type2 != hdr->type3))   {
	/* Bad type field! Can we fix it? */
	if ((hdr->type == hdr->type2) && (hdr->type != hdr->type3))   {
	    printk("CORRUPTION: type3 bad. Got 0x%04x, wanted 0x%04x\n",
		hdr->type3, hdr->type);
	    if (!error && !*warning)   disp_pkthdr(hdr);
	    hdr->type3= hdr->type;
	    (*warning)++;
        #ifndef _NO_ERROR_STATS
	    proc_stat->hdr_wrn_type++;
        #endif /* NO_ERROR_STATS */
	} else if ((hdr->type == hdr->type3) && (hdr->type != hdr->type2))   {
	    printk("CORRUPTION: type2 bad. Got 0x%04x, wanted 0x%04x\n",
		hdr->type2, hdr->type);
	    if (!error && !*warning)   disp_pkthdr(hdr);
	    hdr->type2= hdr->type;
	    (*warning)++;
        #ifndef NO_ERROR_STATS
	    proc_stat->hdr_wrn_type++;
        #endif /* NO_ERROR_STATS */
	} else if ((hdr->type2 == hdr->type3) && (hdr->type != hdr->type2))   {
	    printk("CORRUPTION: type1 bad. Got 0x%04x, wanted 0x%04x\n",
		hdr->type, hdr->type2);
	    if (!error && !*warning)   disp_pkthdr(hdr);
	    hdr->type= hdr->type2;
	    (*warning)++;
        #ifndef NO_ERROR_STATS
	    proc_stat->hdr_wrn_type++;
        #endif /* NO_ERROR_STATS */
	} else   {
	    /* Can't fix it! */
	    printk("CORRUPTION: type bad. Can't fix it!\n");
	    if (!error && !*warning)   disp_pkthdr(hdr);
	    error++;
        #ifndef NO_ERROR_STATS
	    proc_stat->hdr_err_type++;
        #endif /* NO_ERROR_STATS */
	}
    }


    /*
    ** Check the src_nid
    */
    if ((hdr->src_nid != hdr->src_nid2) || (hdr->src_nid != hdr->src_nid3) ||
	    (hdr->src_nid2 != hdr->src_nid3))   {
	/* Bad src_nid field! Can we fix it? */
	if ((hdr->src_nid == hdr->src_nid2) && (hdr->src_nid != hdr->src_nid3)){
	    printk("CORRUPTION: src_nid3 bad. Got 0x%04x, wanted 0x%04x\n",
		hdr->src_nid3, hdr->src_nid);
	    if (!error && !*warning)   disp_pkthdr(hdr);
	    hdr->src_nid3= hdr->src_nid;
	    (*warning)++;
        #ifndef NO_ERROR_STATS
	    proc_stat->hdr_wrn_src_nid++;
        #endif /* NO_ERROR_STATS */
	} else if ((hdr->src_nid == hdr->src_nid3) &&
		(hdr->src_nid != hdr->src_nid2))   {
	    printk("CORRUPTION: src_nid2 bad. Got 0x%04x, wanted 0x%04x\n",
		hdr->src_nid2, hdr->src_nid);
	    if (!error && !*warning)   disp_pkthdr(hdr);
	    hdr->src_nid2= hdr->src_nid;
	    (*warning)++;
        #ifndef NO_ERROR_STATS
	    proc_stat->hdr_wrn_src_nid++;
        #endif /* NO_ERROR_STATS */
	} else if ((hdr->src_nid2 == hdr->src_nid3) &&
		(hdr->src_nid != hdr->src_nid2))   {
	    printk("CORRUPTION: src_nid1 bad. Got 0x%04x, wanted 0x%04x\n",
		hdr->src_nid, hdr->src_nid2);
	    if (!error && !*warning)   disp_pkthdr(hdr);
	    hdr->src_nid= hdr->src_nid2;
	    (*warning)++;
        #ifndef NO_ERROR_STATS
	    proc_stat->hdr_wrn_src_nid++;
        #endif /* NO_ERROR_STATS */
	} else   {
	    /* Can't fix it! */
	    printk("CORRUPTION: src_nid bad. Can't fix it!\n");
	    if (!error && !*warning)   disp_pkthdr(hdr);
	    error++;
        #ifndef NO_ERROR_STATS
	    proc_stat->hdr_err_src_nid++;
        #endif /* NO_ERROR_STATS */
	}
    }


    /*
    ** Check the msgID
    */
    if ((hdr->msgID != hdr->msgID2) || (hdr->msgID != hdr->msgID3) ||
	    (hdr->msgID2 != hdr->msgID3))   {
	/* Bad msgID field! Can we fix it? */
	if ((hdr->msgID == hdr->msgID2) && (hdr->msgID != hdr->msgID3))   {
	    printk("CORRUPTION: msgID3 bad. Got 0x%04x, wanted 0x%04x\n",
		hdr->msgID3, hdr->msgID);
	    if (!error && !*warning)   disp_pkthdr(hdr);
	    hdr->msgID3= hdr->msgID;
	    (*warning)++;
        #ifndef NO_ERROR_STATS
	    proc_stat->hdr_wrn_msgID++;
        #endif /* NO_ERROR_STATS */
	} else if ((hdr->msgID == hdr->msgID3) && (hdr->msgID != hdr->msgID2)) {
	    printk("CORRUPTION: msgID2 bad. Got 0x%04x, wanted 0x%04x\n",
		hdr->msgID2, hdr->msgID);
	    if (!error && !*warning)   disp_pkthdr(hdr);
	    hdr->msgID2= hdr->msgID;
	    (*warning)++;
        #ifndef NO_ERROR_STATS
	    proc_stat->hdr_wrn_msgID++;
        #endif /* NO_ERROR_STATS */
	} else if ((hdr->msgID2 == hdr->msgID3) && (hdr->msgID != hdr->msgID2)){
	    printk("CORRUPTION: msgID1 bad. Got 0x%04x, wanted 0x%04x\n",
		hdr->msgID, hdr->msgID2);
	    if (!error && !*warning)   disp_pkthdr(hdr);
	    hdr->msgID= hdr->msgID2;
	    (*warning)++;
        #ifndef NO_ERROR_STATS
	    proc_stat->hdr_wrn_msgID++;
        #endif /* NO_ERROR_STATS */
	} else   {
	    /* Can't fix it! */
	    printk("CORRUPTION: msgID bad. Can't fix it!\n");
	    if (!error && !*warning)   disp_pkthdr(hdr);
	    error++;
        #ifndef NO_ERROR_STATS
	    proc_stat->hdr_err_msgID++;
        #endif /* NO_ERROR_STATS */
	}
    }

    if (*warning)   {
	printk("CORRUPTION: There were %d corrupted fields in packet header "
	    "that got fixed.\n", *warning);
    }

    /* Length sanity check is already done */

    /* Make sure the msgID is sane */
    snid= msgID2pnid(hdr->msgID);
    if ((snid < 0) || (snid >= MAX_NUM_ROUTES))   {
	printk("CORRUPTION: msgID is not sane: %s\n", show_msgID(hdr->msgID));
	if (!error && !*warning)   disp_pkthdr(hdr);
	error++;
    }

    /* Make sure the src field is sane */
    if (hdr->src_nid >= MAX_NUM_ROUTES)   {
	printk("CORRUPTION: src_nid is not sane: 0x%04x\n", hdr->src_nid);
	if (!error && !*warning)   disp_pkthdr(hdr);
	error++;
    }

    /* Make sure the type field is sane */
    if ((hdr->type < CTS) || (hdr->type >= LAST_ENTRY_DO_NOT_MOVE))   {
	printk("CORRUPTION: type is not sane: 0x%04x\n", hdr->type);
	if (!error && !*warning)   disp_pkthdr(hdr);
	error++;
    }

    if (error)   {
	printk("CORRUPTION: There were %d corrupted fields in packet header "
	    "that could not be fixed.\n", error);
    }

    return error;

}  /* end of check_hdr() */

/* ************************************************************************** */
/*
** There are three classes of packets (depending on the packet type):
**     class 1: We always drop them
**     class 2: We accept them even if we had to fix the header
**     class 3: We don't accept them and request a retransmit
** If warning > 0 then the header had to be fixed.
*/
static int
do_recovery(pkthdr_t *hdr)
{

pkt_type_t type;
int rc;


    mb();
    type= hdr->type;
    rc= TRUE;

    switch (type)   {
	/* class 1: We always drop them */
	case IP:
	case INFO_REQ:
	case INFO_DATA:
	case DATA:
	case STOP_DATA:
	case LAST_DATA:
	case PINGR:
	case CACHE_REQ:
	case CACHE_DATA:
	case PINGA:
    case P3_PING_REQ:
    case P3_PING_ACK:
	    rc= TRUE;
	    printk("RECOVERY: dropping a class 1 pkt (%s)\n",protocol_name_str[type]);
	    break;

	/* class 2: We accept them even if we had to fix the header */
	case CTS:
	case MSGEND:
	case MSGDROP:
	case PING:
	case ROUTE_STAT:
	case ROUTE_ACK:
	case ROUTE_REQ:
	case ROUTE_REQ_REPLY:
	#ifdef DO_TIMEOUT_PROTOCOL
	    case P3_RESEND:
	#else
	    case PAD2:
	#endif /* DO_TIMEOUT_PROTOCOL */
	case GCH:
	    rc= FALSE;
	    printk("RECOVERY: accepting a class 2 pkt (%s) with fixed header\n",
                protocol_name_str[type]);
	    break;

	/* class 3: We don't accept them and request a retransmit */
	case P3_RTS:
	case P3_LAST_RTS:
    #ifdef EXTENDED_P3_RTSCTS
    case P3_NULL:
    case P3_SYNC:
    #endif /* EXTENDED_P3_RTSCTS */
	    #if defined(DO_TIMEOUT_PROTOCOL) && defined(REQUEST_RESENDS)
		sendProtoMSG(P3_RESEND, hdr->msgID, hdr->src_nid, 0, 0, 0, NULL);
		printk("RECOVERY: request a retransmit of a P3 packet (%s)\n",
               protocol_name_str[type]);
	    #else
		printk("RECOVERY: dropping a class 3 pkt (%s) packet, msgID=%08x msgNum=%d seq=%d\n",
                protocol_name_str[type],hdr->msgID,hdr->msgNum,hdr->seq);
	    #endif /* DO_TIMEOUT_PROTOCOL */
	    rc= TRUE;
	    break;

	/* This type should never be received! ;-) */
	case LAST_ENTRY_DO_NOT_MOVE:
	    rc= TRUE;
	    break;
    }

    return rc;

}  /* end of do_recovery() */

/* ************************************************************************** */
