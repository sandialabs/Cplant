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
** $Id: RTSCTS_recv.c,v 1.101.2.4 2002/07/08 16:11:15 jbogden Exp $
** The functions to receive a packet
*/
#include <linux/sched.h>		/* For jiffies */
#include <asm/system.h>			/* For cli(), save_flags(), etc. */
#include <asm/byteorder.h>		/* For htons() */
#include <lib-p30.h>			/* For lib_finalize() */
#include <portals/base/machine.h>	/* For TO_USER */
#include "../../portals/p3mod/debug.h"	/* For p3_debug_add() */
#include <sys/defines.h>                /* For MIN() */

#include "RTSCTS_proc.h"
#include "queue.h"
#include "RTSCTS_protocol.h"
#include "RTSCTS_ioctl.h"
#include "RTSCTS_recv.h"
#include "RTSCTS_route.h"
#include "RTSCTS_p3.h"
#include "RTSCTS_ip.h"
#include "RTSCTS_send.h"
#include "RTSCTS_info.h"		/* For handleInfoReq() */
#include "RTSCTS_cache.h"		/* For handleCacheX() */
#include "RTSCTS_debug.h"		/* For protocol_debug_add() */
#include "Pkt_recv.h"			/* For disp_pkthdr() */
#include "MCPshmem.h"           	/* for MAX_NUM_ROUTES */
#include "hstshmem.h"           	/* for hstshmem */

/* >>>  ----------------------------------------------------------------- <<< */
/*
** Debugging
** Keep a list of the last 8 packet headers to print in case of problems
*/
#ifdef RECENT_PKT_HDRS
    #define MAX_PKTHDR_LIST	(8)
    pkthdr_t last_pkthdr[MAX_PKTHDR_LIST];
    static unsigned int last_pkthdr_idx= 0;
#endif /* RECENT_PKT_HDRS */


/* >>>  ----------------------------------------------------------------- <<< */
/*
** Local functions
*/
static void handleMSGEND(unsigned int msgID);
static void handleMSGDROP(unsigned int msgID, int debug, int info2);
static void handleCTS(unsigned int msgID, unsigned int msglen, int num_pkts);
static void handleDATA(unsigned int msgID, pkthdr_t *pkthdr, char *buf,
		unsigned long len, int pkt_allowance, int retries);
static void handleGCH(unsigned int msgID);


/* >>>  ----------------------------------------------------------------- <<< */
/*
** This is the function that gets called for every incoming packet.
** We'll figure out what type of packet it is, and call the appropriate
** handler. The Ethernet header has already been stripped off, so page
** points to the packet header.
*/
int 
rtscts_recv(unsigned long page, struct NETDEV *dev)
{

unsigned long flags;
pkthdr_t *pkthdr;
#ifdef MSGCKSUM
static unsigned int RTScrc;
#endif /* MSGCKSUM */
unsigned int crc;
unsigned int talking;
int len;

    save_flags(flags);
    cli();    

    #ifdef DROP_PKT_TEST
    {

    static int cnt= 0;
    static int drop_next_one_too= FALSE;
    static int pattern= 0;
    static int dropped= 0;
    int drop_pattern[]= {1, 1, 1, 3, 1, 1, 10, 11, 13, 1, 2, 1, 1, 1, 1, 1, 1,
	7, 12, 11, 1, 1, 5, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 4,
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 3, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	22, 1, 6, 6, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 12, 1, 1, 1, 1, -1};

    #ifdef VERY_VERBOSE
	pkthdr_t *pkthdr;
    #endif /* VERY_VERBOSE */


	if ((++cnt > 500) || drop_next_one_too)   {
	    /* Fake a dropped packet */
	    cnt= 0;

	    pkthdr= (pkthdr_t *)page;
	    #ifdef VERY_VERBOSE
		printk("Dropping %s %s, seq %d, info %d\n",
		    protocol_name_str[pkthdr->type], show_msgID(pkthdr->msgID),
		    pkthdr->seq, pkthdr->info);
	    #endif /* VERY_VERBOSE */
	    rtscts_stat->dbg_drop_pkt++;

	    dropped++;
	    if (dropped >= drop_pattern[pattern])   {
		dropped= 0;
		drop_next_one_too= FALSE;
		pattern++;
		if (drop_pattern[pattern] < 0)   {
		    pattern= 0;
		}
	    } else   {
		drop_next_one_too= TRUE;
	    }

	    restore_flags(flags);
	    return 0;
	}
    }
    #endif /* DROP_PKT_TEST */

    pkthdr= (pkthdr_t *)page;
    len = (int) pkthdr->len;

    #ifndef NO_STATS
    /* Keep stats */
    rtscts_stat->PKTrecv++;
    rtscts_stat->PKTrecvlen += len;
    if (len > rtscts_stat->PKTrecvmaxlen)   {
	rtscts_stat->PKTrecvmaxlen= len;
    }
    if (len < rtscts_stat->PKTrecvminlen)   {
	rtscts_stat->PKTrecvminlen= len;
    }
    
    rtscts_stat->protoRcvd[pkthdr->type]++;
    #endif /* NO_STATS */

    protocol_debug_add(pkthdr->msgID, pkthdr->type, pkthdr->src_nid,
	pkthdr->info, pkthdr->info2, FALSE);

    #ifdef RECENT_PKT_HDRS
	memcpy(&(last_pkthdr[last_pkthdr_idx]), pkthdr, sizeof(pkthdr_t));
	last_pkthdr_idx= (last_pkthdr_idx + 1) % MAX_PKTHDR_LIST;
    #endif /* RECENT_PKT_HDRS */

    switch (pkthdr->type)   {
	case CTS:
	    talking= remote_status[pkthdr->src_nid].status & RMT_TALKING;
	    remote_status[pkthdr->src_nid].status= RMT_OK | talking;
	    remote_status[pkthdr->src_nid].first_ack= TRUE;
	    remote_status[pkthdr->src_nid].last_update= jiffies;
	    handleCTS(pkthdr->msgID, pkthdr->info, pkthdr->info2);
	    break;
	case DATA:
	case STOP_DATA:
	case LAST_DATA:
	    talking= remote_status[pkthdr->src_nid].status & RMT_TALKING;
	    remote_status[pkthdr->src_nid].status= RMT_OK | talking;
	    remote_status[pkthdr->src_nid].first_ack= TRUE;
	    remote_status[pkthdr->src_nid].last_update= jiffies;
	    handleDATA(pkthdr->msgID, pkthdr, (char *)(page + sizeof(pkthdr_t)),
		len - sizeof(pkthdr_t), pkthdr->info, pkthdr->info2);
	    break;
	case MSGEND:
	    talking= remote_status[pkthdr->src_nid].status & RMT_TALKING;
	    remote_status[pkthdr->src_nid].status= RMT_OK | talking;
	    remote_status[pkthdr->src_nid].first_ack= TRUE;
	    remote_status[pkthdr->src_nid].last_update= jiffies;
	    handleMSGEND(pkthdr->msgID);
	    break;
	case MSGDROP:
	    talking= remote_status[pkthdr->src_nid].status & RMT_TALKING;
	    remote_status[pkthdr->src_nid].status= RMT_OK | talking;
	    remote_status[pkthdr->src_nid].first_ack= TRUE;
	    remote_status[pkthdr->src_nid].last_update= jiffies;
	    handleMSGDROP(pkthdr->msgID, pkthdr->info, pkthdr->info2);
	    break;
        case PING:
	    remote_status[pkthdr->src_nid].status |= RMT_TALKING;
	    remote_status[pkthdr->src_nid].last_update= jiffies;
            handlePING(msgID2pnid(pkthdr->msgID));
            break;
        case PINGA:
	    remote_status[pkthdr->src_nid].status |= RMT_TALKING;
	    remote_status[pkthdr->src_nid].last_update= jiffies;
            handlePINGA(msgID2pnid(pkthdr->msgID));
            break;
        case PINGR:
	    remote_status[pkthdr->src_nid].status |= RMT_TALKING;
	    remote_status[pkthdr->src_nid].last_update= jiffies;
            handlePINGR(msgID2pnid(pkthdr->msgID),
                        (char*)(page + sizeof(pkthdr_t)),
                        len-sizeof(pkthdr_t));
            break;
        case ROUTE_REQ:
	    remote_status[pkthdr->src_nid].status |= RMT_TALKING;
	    remote_status[pkthdr->src_nid].last_update= jiffies;
            sendRouteStat(pkthdr->info, msgID2pnid(pkthdr->msgID));
            break;
        case ROUTE_REQ_REPLY:
	    remote_status[pkthdr->src_nid].status |= RMT_TALKING;
	    remote_status[pkthdr->src_nid].last_update= jiffies;
            handleROUTE_REQ_REPLY(msgID2pnid(pkthdr->msgID),
		(route_status_t) pkthdr->info);
            break;
        case ROUTE_STAT:
	    remote_status[pkthdr->src_nid].status |= RMT_TALKING;
	    remote_status[pkthdr->src_nid].last_update= jiffies;
            handleRouteStat(pkthdr->msgID, pkthdr->info, pkthdr->info2);
            break;
        case ROUTE_ACK:
	    remote_status[pkthdr->src_nid].status |= RMT_TALKING;
	    remote_status[pkthdr->src_nid].last_update= jiffies;
            handleRouteAck(pkthdr->msgID, pkthdr->info, pkthdr->info2);
            break;
        case IP:
	    remote_status[pkthdr->src_nid].status |= RMT_TALKING;
	    remote_status[pkthdr->src_nid].last_update= jiffies;
	    handleIP(pkthdr->msgID, pkthdr, (char *)(page + sizeof(pkthdr_t)),
		len - sizeof(pkthdr_t));
            break;

        case P3_RTS:
	    #ifdef MSGCKSUM
		crc= pkthdr->crc;
	    #else
		crc= 0;
	    #endif /* MSGCKSUM */
	    remote_status[pkthdr->src_nid].status |= RMT_TALKING;
	    remote_status[pkthdr->src_nid].last_update= jiffies;
	    handleP3(pkthdr->msgID, pkthdr->msgNum,
		(ptl_hdr_t *)(page + sizeof(pkthdr_t)),
		(char *)(page + sizeof(pkthdr_t) + sizeof(ptl_hdr_t)),
		len - sizeof(pkthdr_t) - sizeof(ptl_hdr_t), P3_RTS, dev,
		crc);
	    break;

        case P3_LAST_RTS:
	    #ifdef MSGCKSUM
		crc= pkthdr->crc;
	    #else
		crc= 0;
	    #endif /* MSGCKSUM */
	    remote_status[pkthdr->src_nid].status |= RMT_TALKING;
	    remote_status[pkthdr->src_nid].last_update= jiffies;
	    handleP3(pkthdr->msgID, pkthdr->msgNum,
		(ptl_hdr_t *)(page + sizeof(pkthdr_t)),
		(char *)(page + sizeof(pkthdr_t) + sizeof(ptl_hdr_t)),
		len - sizeof(pkthdr_t) - sizeof(ptl_hdr_t), P3_LAST_RTS,
		dev, crc);
	    break;

        #ifdef P3_PING
        case P3_PING_REQ:
        case P3_PING_ACK:
	    #ifdef MSGCKSUM
		crc= pkthdr->crc;
	    #else
		crc= 0;
	    #endif /* MSGCKSUM */
	    remote_status[pkthdr->src_nid].status |= RMT_TALKING;
        remote_status[pkthdr->src_nid].first_ack= TRUE;
	    remote_status[pkthdr->src_nid].last_update= jiffies;
	    handleP3(pkthdr->msgID, pkthdr->msgNum,
		(ptl_hdr_t *)(page + sizeof(pkthdr_t)),
		(char *)(page + sizeof(pkthdr_t) + sizeof(ptl_hdr_t)),
		len - sizeof(pkthdr_t) - sizeof(ptl_hdr_t), pkthdr->type,
		dev, crc);
	    break;
        #endif

        #if defined(DO_TIMEOUT_PROTOCOL) && defined(REQUEST_RESENDS)
	    case P3_RESEND:
		    handleP3resend(pkthdr->msgID, pkthdr->msgNum);
		break;
        #endif /* DO_TIMEOUT_PROTOCOL && REQUEST_RESENDS */

        #if defined(EXTENDED_P3_RTSCTS) && defined(DO_TIMEOUT_PROTOCOL)
        case P3_NULL:
	    #ifdef MSGCKSUM
		crc= pkthdr->crc;
	    #else
		crc= 0;
	    #endif /* MSGCKSUM */
	    remote_status[pkthdr->src_nid].status |= RMT_TALKING;
	    remote_status[pkthdr->src_nid].last_update= jiffies;
	    handleP3(pkthdr->msgID, pkthdr->msgNum,
		(ptl_hdr_t *)(page + sizeof(pkthdr_t)),
		(char *)(page + sizeof(pkthdr_t) + sizeof(ptl_hdr_t)),
		len - sizeof(pkthdr_t) - sizeof(ptl_hdr_t), pkthdr->type,
		dev, crc);
	    break;
        
        case P3_SYNC:
	    remote_status[pkthdr->src_nid].status |= RMT_TALKING;
	    remote_status[pkthdr->src_nid].last_update= jiffies;
        if ((lastMsgNum[pkthdr->src_nid] == BOOTMSGNUM) ||
            (pkthdr->msgNum == BOOTMSGNUM) ) {

            /* nice for debugging but not really needed
            printk("rtscts_recv: ignoring P3_SYNC src=%d msgNum=%d lastMsgNum=%d\n",
                   pkthdr->src_nid,pkthdr->msgNum - 1,lastMsgNum[pkthdr->src_nid]);
            */
        }
        else {
            /* We only need to process the P3_SYNC for resyncing
             * if there was not rebooting involved with either
             * node. If there was, the standard msg sequence logic
             * for the normal P3 packets should work fine.
            */
            
            /* This provides really nice debug info, but when things are going
             * badly on the system this tends to really fill up the logs.
             * Can get the same basic info from the infoprotocol utility.
            printk("rtscts_recv: P3 resync %d -> %d with msgNum %d (was %d), msgID %08x\n",
                   pkthdr->src_nid,hstshmem->my_pnid,pkthdr->msgNum - 1,
                   lastMsgNum[pkthdr->src_nid],pkthdr->msgID);
            */
                   
            lastMsgNum[pkthdr->src_nid] = pkthdr->msgNum - 1;

            #ifndef NO_ERROR_STATS
            rtscts_stat->protoP3resynced++;
            #endif /* NO_ERROR_STATS */
        }
        break;
        #endif /* EXTENDED_P3_RTSCTS && DO_TIMEOUT_PROTOCOL */

        case GCH:
	    remote_status[pkthdr->src_nid].status |= RMT_TALKING;
	    remote_status[pkthdr->src_nid].last_update= jiffies;
	    handleGCH(pkthdr->msgID);
	    break;
        case INFO_REQ:
	    remote_status[pkthdr->src_nid].status |= RMT_TALKING;
	    remote_status[pkthdr->src_nid].last_update= jiffies;
	    handleInfoReq(pkthdr->src_nid, pkthdr->msgID, pkthdr->info,
		pkthdr->info2);
	    break;
        case INFO_DATA:
	    remote_status[pkthdr->src_nid].status |= RMT_TALKING;
	    remote_status[pkthdr->src_nid].last_update= jiffies;
	    handleInfoData(pkthdr->src_nid, pkthdr->msgID,
		(char *)(page + sizeof(pkthdr_t)), pkthdr->info, pkthdr->info2);
	    break;
        case CACHE_REQ:
	    remote_status[pkthdr->src_nid].status |= RMT_TALKING;
	    remote_status[pkthdr->src_nid].last_update= jiffies;
            handleCacheReq(pkthdr->src_nid);
            break;
        case CACHE_DATA:
	    remote_status[pkthdr->src_nid].status |= RMT_TALKING;
	    remote_status[pkthdr->src_nid].last_update= jiffies;
            handleCacheData(pkthdr->src_nid, (char*)(page+sizeof(pkthdr_t)));
            break;
	default:
	    printk("Unknown packet %d\n", pkthdr->type);
	    break;
    }

    restore_flags(flags);

    return 0; 

}  /* end of rtscts_recv() */

/* >>>  ----------------------------------------------------------------- <<< */
/*
** We received an ack for a sent message. Find the message (it could be
** on the pending queue, if it was a one packet message, or on the sending
** queue, if it was a bigger message) and finisg it.
*/
static void
handleMSGEND(unsigned int msgID)
{

Qentry_t *entry;
int rc;


    entry= dequeue(Qsnd_pending, msgID);
    if (!entry)   {
	/* Maybe it is on the sending queue (msg larger than 1 packet) */
	entry= dequeue(Qsending, msgID);
    }

    if (!entry)   {
	#ifdef VERBOSE
	    printk("handleMSGEND() No entry found for %s\n", show_msgID(msgID));
	    disp_queues();
	#endif /* VERBOSE */
    #ifndef NO_ERROR_STATS
	rtscts_stat->protoErr3++;
    #endif /* NO_ERROR_STATS */
	return;
    }

    if (entry->nal)   {
	#ifdef VERBOSE
	printk("(3) telling NAL to set PTL_EVENT_SENT\n");
	#endif /* VERBOSE */

    #ifdef DO_TIMEOUT_PROTOCOL
	if (1)   {
    #else
	if (entry->event != E_SENT_P3_LAST_RTS)   {
	    /* If not already done in p3_send() */
    #endif /* DO_TIMEOUT_PROTOCOL */
	    rc= 0;
	    if (!entry->lib_finalize_called)   {
		rc= lib_finalize(entry->nal, (void *)3, entry->cookie);
        #ifndef NO_STATS
		rtscts_stat->P3eventsent++;
        #endif /* NO_STATS */
		entry->lib_finalize_called= TRUE;
	    }
	    if (rc != 0)   {
		#ifdef VERBOSE
		printk("(3) lib_finalize() failed for PTL_EVENT_SENT\n");
		#endif /* VERBOSE */
		send_sig(SIGPIPE, entry->task, 1);
        #ifndef NO_PROTO_DEBUG
		p3_debug_add(entry->msgID, (ptl_hdr_t *)&(entry->phdr),
		    SND_FINISHED, P3STAT_FINALIZE);
        #endif /* NO_PROTO_DEBUG */
	    } else   {
        #ifndef NO_PROTO_DEBUG
		p3_debug_add(entry->msgID, (ptl_hdr_t *)&(entry->phdr),
		    SND_FINISHED, P3STAT_OK);
        #endif /* NO_PROTO_DEBUG */
	    }
	} else   {
        #ifndef NO_PROTO_DEBUG
	    p3_debug_add(entry->msgID, (ptl_hdr_t *)&(entry->phdr),
		SND_FINISHED, P3STAT_OK);
        #endif /* NO_PROTO_DEBUG */
	}

    #ifndef NO_STATS
	rtscts_stat->P3sendlen+= entry->datasent;
	if (entry->datasent > rtscts_stat->P3sendmaxlen)   {
	    rtscts_stat->P3sendmaxlen= entry->datasent;
	}
	if (entry->datasent < rtscts_stat->P3sendminlen)   {
	    rtscts_stat->P3sendminlen= entry->datasent;
	}
    #endif /* NO_STATS */
    }

    /* Send the garbage collection hint */
    sendProtoMSG(GCH, entry->msgID, entry->dst_nid, 0x0011, 0x2233, 0, NULL);

    /* Free everything */
    free_entry(entry);

}  /* end of handleMSGEND() */

/* >>>  ----------------------------------------------------------------- <<< */

static void
handleMSGDROP(unsigned int msgID, int debug, int info2)
{
Qentry_t *entry;
int rc;

    #ifdef VERBOSE
    printk("handleMSGDROP() start msgID=%08x info1=%08x info2=%08x\n",
           msgID,debug,info2);
    #endif
    
    entry= dequeue(Qsnd_pending, msgID);
    if (!entry)   {
	entry= dequeue(Qsending, msgID);
    }

    if (!entry)   {
	    #ifdef VERBOSE
	    printk("handleMSGDROP() No entry found for %s\n", show_msgID(msgID));
	    disp_queues();
	    #endif /* VERBOSE */

        #ifndef NO_ERROR_STATS
	    rtscts_stat->protoErr4++;
        #endif /* NO_ERROR_STATS */
	    return;
    }

    if (entry->nal)   {
	    #ifdef VERBOSE
	    printk("(4) telling NAL to set PTL_EVENT_SENT\n");
	    #endif /* VERBOSE */

        #ifdef DO_TIMEOUT_PROTOCOL
	    if (1)   {
        #else
	    if (entry->event != E_SENT_P3_LAST_RTS)   {
	        /* If not already done in p3_send() */
        #endif /* DO_TIMEOUT_PROTOCOL */
	        rc= 0;
	        if (!entry->lib_finalize_called)   {
		        rc= lib_finalize(entry->nal, (void *)4, entry->cookie);

                #ifndef NO_STATS
		        rtscts_stat->P3eventsent++;
                #endif /* NO_STATS */

		        entry->lib_finalize_called= TRUE;
	        }
	        if (rc != 0)   {
		        #ifdef VERBOSE
		        printk("(4) lib_finalize() for set PTL_EVENT_SENT failed\n");
		        #endif /* VERBOSE */

		        send_sig(SIGPIPE, entry->task, 1);
	        }
	    }

        #ifndef NO_PROTO_DEBUG
	    p3_debug_add(entry->msgID, (ptl_hdr_t *)&(entry->phdr),
                     SND_FINISHED, P3STAT_DROPPED);
        #endif /* NO_PROTO_DEBUG */

        #ifndef NO_STATS
	    rtscts_stat->P3sendlen+= entry->datasent;
	    if (entry->datasent > rtscts_stat->P3sendmaxlen)   {
	        rtscts_stat->P3sendmaxlen= entry->datasent;
	    }
	    if (entry->datasent < rtscts_stat->P3sendminlen)   {
	        rtscts_stat->P3sendminlen= entry->datasent;
	    }
        #endif /* NO_STATS */
    }

    #ifdef EXTENDED_P3_RTSCTS
    /* Send the garbage collection hint only if the info2 paramter
     * we were called with (which was the info2 field in the incoming
     * packet) is equal to 0xd5d5.  The 0xd5d5 value indicates
     * that the MSGDROP was sent in reponse to a P3_NULL from us.
    */
    if (info2 != 0xd5d5) {
        sendProtoMSG(GCH,entry->msgID,entry->dst_nid,0x0011,0x2233,0,NULL);
    }
    #else
    /* Send the garbage collection hint */
    sendProtoMSG(GCH, entry->msgID,entry->dst_nid,0x0011,0x2233,0,NULL);
    #endif /* EXTENDED_P3_RTSCTS */
    
    if (debug)   {
	    printk("handle_MSGDROP(debug)\n");
	    disp_Qentry(entry);
    }

    /* Free everything */
    free_entry(entry);
}  /* end of handleMSGDROP() */

/* >>>  ----------------------------------------------------------------- <<< */
/*
** We received an ack for a sent message. Find the message in the pending
** queue. num_pkts is the number of packets we were given permission to
** send with this CTS.
*/
static void
handleCTS(unsigned int msgID, unsigned int msglen, int num_pkts)
{

Qentry_t *entry;

    #ifdef VERBOSE
    printk("handleCTS() msgID %08x\n",msgID);
    #endif /* VERBOSE */
    
    entry= dequeue(Qsnd_pending, msgID);
    if (!entry) {
	    /* If this is a second or later CTS, then it is on the sending Q */
	    entry= dequeue(Qsending, msgID);
	    if (!entry) {
	        /* It should be here! */
	        #ifdef VERBOSE
	        printk("handleCTS() No entry found for %s\n", show_msgID(msgID));
	        disp_queues();
	        #endif /* VERBOSE */
            #ifndef NO_ERROR_STATS
	        rtscts_stat->protoErr5++;
            #endif /* NO_ERROR_STATS */
	        return;
	    }
    } else {
	    if (entry->nal) {
            #ifndef NO_PROTO_DEBUG
	        p3_debug_add(entry->msgID, (ptl_hdr_t *)&(entry->phdr),
		    SND_DEQUEUED, P3STAT_OK);
            #endif /* NO_PROTO_DEBUG */
	    } else if (entry->event == E_SENT_P3_NULL) {
            printk("handleCTS() Orphaned CTS due to P3_NULL\n");
        } else {
	        printk("handleCTS() This is weird %s\n", show_msgID(msgID));
	    }
    }

    /*
    ** If the receiver wants to receive less than what the sender wants
    ** to send, then truncate at the send side.
    */
    if (msglen < entry->len)   {
    #ifndef NO_ERROR_STATS
	rtscts_stat->truncate++;
	rtscts_stat->truncatelen += entry->len - msglen;
    #endif /* NO_ERROR_STATS */
	entry->len= msglen;
    }

    /* Move it to the sending queue and try to send the data*/
    enqueue_entry(Qsending, entry);
    entry->event= E_RCVD_CTS;
    entry->idle= jiffies;
    sendDATA(entry, num_pkts);

}  /* end of handleCTS() */

/* >>>  ----------------------------------------------------------------- <<< */
/*
** Garbage collection hint. The sender got MSGEND or MSGDROP, and we
** can free the entry in Qreceiving.
*/
static void
handleGCH(unsigned int msgID)
{

Qentry_t *entry;

    entry= dequeue(Qreceiving, msgID);
    if (entry)   {
	/* Done with this message */
    #ifndef NO_STATS
	rtscts_stat->protoGCHdone++;
    #endif /* NO_STATS */
	free_entry(entry);
    } else   {
	/* It should have been here... */
    #ifndef NO_ERROR_STATS
	rtscts_stat->protoGCHdoneErr++;
    #endif /* NO_ERROR_STATS */
    }

}  /* end of handleGCH() */

/* >>>  ----------------------------------------------------------------- <<< */

static void
msgDone(pkt_type_t ptype, Qentry_t *entry, p3_status_t stat)
{

int rc;


    if (ptype == MSGEND)   {
	sendProtoMSG(ptype, entry->msgID, entry->src_nid, 0, 0, 0, NULL);
	entry->event= E_SENT_MSGEND;
    } else if (ptype == MSGDROP)   {
	/* entry->event= E_SENT_MSGDROP; */
    } else   {
	printk("msgDone() Invalid ptype %d\n", ptype);
	entry->event= E_INVALID;
    }

    #ifdef MSGCKSUM
	if (!checksum(entry->task, entry->buf, entry->len,
		entry->datasent, entry->crc))   {
	    printk("buf %p, active_cnt %p, msgID 0x%08x, spid %d, "
		"seq %d, snid %d, ptl %d\n", entry->buf, entry->flag,
		entry->msgID, entry->task->pid, entry->seq,
		entry->src_nid, entry->portal);
	    if (entry->nal)   {
        #ifndef NO_PROTO_DEBUG
		p3_debug_add(entry->msgID, (ptl_hdr_t *)&(entry->phdr),
		    RCV_FINISHED, P3STAT_CHKSUM);
        #endif /* NO_PROTO_DEBUG */
	    }
	}
    #endif /* MSGCKSUM */

    if (entry->nal == NULL)   {
	printk("msgDone() That's not good...\n");
    }

    #ifndef NO_PROTO_DEBUG
    p3_debug_add(entry->msgID, (ptl_hdr_t *)&entry->phdr, RCV_FINISHED, stat);
    #endif /* NO_PROTO_DEBUG */
    
    if (ptype == MSGEND)   {
	/*
	** Only call lib_finalize() for complete messages. The
	** process will be killed for others anyway.
	*/
	rc= 0;
	if (!entry->lib_finalize_called)   {
	    rc= lib_finalize(entry->nal, (void *)7, entry->cookie);
        #ifndef NO_STATS
	    rtscts_stat->P3eventrcvd++;
        #endif /* NO_STATS */
	    entry->lib_finalize_called= TRUE;
	}
	if (rc != 0)   {
	    #ifdef VERBOSE
	    printk("msgDone() lib_finalize() failed for long msg\n");
	    #endif /* VERBOSE */
	    send_sig(SIGPIPE, entry->task, 1);
	}
    }

}  /* end of msgDone() */

/* >>>  ----------------------------------------------------------------- <<< */

static void
handleDATA(unsigned int msgID, pkthdr_t *pkthdr, char *buf, unsigned long len,
	int pkt_allowance, int retries)
{

int num_pkts;
Qentry_t *entry;
unsigned long remain;
static unsigned long instance= 0;


    instance++;

    entry= find_queue(Qreceiving, msgID);
    if (!entry)   {
	/*
	** This could happen if the portal module wants less data than the
	** sender is about to send. We'll receive the first few data packets
	** and then send a MSGEND, while the other packets are still coming in.
	**
	** Or if we sent a drop message, and we're still receiving the
	** remaining data packets.
	**
	** Or if we requested a resend and this is one of the data packets
	** that was already in flight.
	*/
	#ifdef VERBOSE
	    printk("handleDATA() No entry found for %s\n", show_msgID(msgID));
	    disp_queues();
	#endif /* VERBOSE */
    #ifndef NO_ERROR_STATS
	rtscts_stat->protoErr6++;
    #endif /* NO_ERROR_STATS */
	return;
    }

/*
if (retries > 0)   {
    printk("\nhandleDATA(%ld, retries %d) %s, entry->retires %d, seq %d, pkt seq %d\n", instance, retries, show_msgID(msgID), entry->retries, entry->seq, pkthdr->seq);
}
*/
    if (((pkthdr->seq < entry->seq) || (retries > entry->retries)) &&
	    (pkthdr->seq == 1))  {
	/* Looks like the sender is trying to restart this message */
	#ifdef VERBOSE
	    printk("handleDATA(%ld) Sender is restarting, seq %d, len %ld, "
		"allowance %d\n", instance, pkthdr->seq, len, pkt_allowance);
	    disp_Qentry(entry);
	#endif /* VERBOSE */
	entry->seq= 1;
	entry->datasent= 0;
	entry->retries = retries;
	/* Take the ones back we granted earlier and are now lost */
	rtscts_stat->outstanding_pkts -= entry->granted_pkts;
	entry->granted_pkts= 0;
	/* Ignore the stolen allowance */
	pkt_allowance= 0;

	#ifdef KEEP_TRACK
	    if (rtscts_stat->outstanding_pkts < 0)   {
		printk("handleDATA(%ld) 4) outstanding_pkts %ld < 0!\n",
		    instance, rtscts_stat->outstanding_pkts);
	    }
	#endif /* KEEP_TRACK */

	/* Assertion */
	if ((pkthdr->type != STOP_DATA) && (pkthdr->type != LAST_DATA))   {  
	    printk("handleDATA(%ld) Assertion 1: len %ld, allow %d, retries %d\n",
		instance, len, pkt_allowance, retries);
	    disp_Qentry(entry);
	    disp_pkthdr(pkthdr);
	}

    } else if (pkthdr->seq < entry->seq)  {
	/* This is a packet of a stream that we're restarting. Drop it. */
	#ifdef VERBOSE
	    printk("handleDATA(%ld) Drop pkt %d (0x%08x): Restarting Stream\n",
		instance, pkthdr->seq, entry->seq);
	#endif /* VERBOSE */
	entry->seq= 0x7fffffff;
	return;
    } else if ((pkthdr->seq > entry->seq) || (retries > entry->retries))  {
	/* A data packet got dropped somewhere. Ignore subsequent pkts */
	#ifdef VERBOSE
	    printk("handleDATA(%ld) Pkt out of sequence. Got %d, expected %d.\n",
		instance, pkthdr->seq, entry->seq);
	#ifdef RECENT_PKT_HDRS
	    {
	    unsigned int i;
		printk("handleDATA(%ld) ===== Last %d packet headers ====\n",
		    instance, MAX_PKTHDR_LIST);
		i= (last_pkthdr_idx + 1) % MAX_PKTHDR_LIST;
		while (i < MAX_PKTHDR_LIST)   {
		    printk("---- Packet header %2d ----\n", i);
		    disp_pkthdr(&(last_pkthdr[i]));
		    i++;
		}
		i= 0;
		while (i <= last_pkthdr_idx)   {
		    printk("---- Packet header %2d ----\n", i);
		    disp_pkthdr(&(last_pkthdr[i]));
		    i++;
		}
	    }
	#else
	    disp_pkthdr(pkthdr);
	#endif /* RECENT_PKT_HDRS */
	disp_Qentry(entry);
	#endif /* VERBOSE */

	entry->seq= 0x7fffffff;
    #ifndef NO_ERROR_STATS
	rtscts_stat->badseq++;
    #endif /* NO_ERROR_STATS */
	return;
    } else   {
	/* Take our allowance back, no matter what we do with the pkt */
	if ((pkthdr->type == STOP_DATA) || (pkthdr->type == LAST_DATA))   {
/*
if (entry->granted_pkts != pkt_allowance)   {
    printk("handleDATA(%ld 2a) entry->granted_pkts %ld != allowance %d\n", instance, entry->granted_pkts, pkt_allowance);
}
*/
	    rtscts_stat->outstanding_pkts -= entry->granted_pkts;
	    entry->granted_pkts= 0;

	    #ifdef KEEP_TRACK
		if (rtscts_stat->outstanding_pkts < 0)   {
		    printk("handleDATA(%ld) 3) outstanding_pkts %ld < 0!\n",
			instance, rtscts_stat->outstanding_pkts);
		}
	    #endif /* KEEP_TRACK */
	}
    }


    /*
    ** Copy the data into user space
    ** We don't receive DATA packets of len = 0 (safe to call memcpy_tofs2())
    */
    remain= entry->len - entry->datasent;
    if (remain < len)   {
    #ifndef NO_ERROR_STATS
	rtscts_stat->extradata++;
	rtscts_stat->extradatalen += len - remain;
    #endif /* NO_ERROR_STATS */
    #ifdef VERBOSE
	printk("\nhandleDATA(%ld) tossing %ld bytes out of %ld, seq %d\n",
	    instance, len - remain, len, pkthdr->seq);
    #endif /* VERBOSE */
    }
    len= MIN(len, remain);

    if ((len > 0) && (!entry->lib_finalize_called))   {
	char *dest = entry->buf + entry->datasent;

#ifdef KERNEL_ADDR_CACHE
        /* check nal to ensure this is a P3 message */ 
        if ( entry->nal != NULL ) {
	    if( memcpy3_to_user(dest, buf, len, entry->addr_key, entry->task) < 0 ) {
		#ifdef VERBOSE
		printk( "handleDATA(%ld) memcpy3 failed? pid=%d: %p -> %p len "
		    "%ld\n", instance, entry->task->pid, buf, dest, len );
		#endif /* VERBOSE */
        #ifndef NO_ERROR_STATS
		rtscts_stat->badcpy++;
        #endif /* NO_ERROR_STATS */
		msgDone(MSGDROP, entry, P3STAT_MEMCPYT);
		send_sig(SIGPIPE, entry->task, 1);
		return;
	    }
        } else {
	    if( memcpy2(TO_USER, entry->task, dest, buf, len) ) {
		#ifdef VERBOSE
		printk( "handleDATA(%ld) memcpy2 failed? pid=%d: %p -> %p len "
		    "%ld\n", instance, entry->task->pid, buf, dest, len );
		#endif /* VERBOSE */
        #ifndef NO_ERROR_STATS
		rtscts_stat->badcpy++;
        #endif /* NO_ERROR_STATS */
		msgDone(MSGDROP, entry, P3STAT_MEMCPYT);
		send_sig(SIGPIPE, entry->task, 1);
		return;
	    }
        }
#else
	if( memcpy2(TO_USER, entry->task, dest, buf, len) ) {
	    #ifdef VERBOSE
	    printk( "handleDATA(%ld) memcpy2 failed? pid=%d: %p -> %p len "
		"%ld\n", instance, entry->task->pid, buf, dest, len );
	    #endif /* VERBOSE */
        #ifndef NO_ERROR_STATS
	    rtscts_stat->badcpy++;
        #endif /* NO_ERROR_STATS */
	    msgDone(MSGDROP, entry, P3STAT_MEMCPYT);
	    send_sig(SIGPIPE, entry->task, 1);
	    return;
	}
#endif
    }

    entry->datasent += len;
    entry->seq++;

    if (pkthdr->type == DATA)   {
	entry->event= E_RCVD_DATA;
    } else if (pkthdr->type == STOP_DATA)   {
	entry->event= E_RCVD_STOP_DATA;
    } else   {
	entry->event= E_RCVD_LAST_DATA;
    }
    entry->idle= jiffies;

    if (pkthdr->type == STOP_DATA)   {
	/*
	** We need to generate a CTS here, so the sender will send us
	** some more packets.
	*/
	num_pkts= MAX_RCV_PKT_ENTRIES - OTHER_NODES -
	    rtscts_stat->outstanding_pkts;
	if (num_pkts > MAX_DATA_PKTS)   {
	    num_pkts= MAX_DATA_PKTS;
	} else if (num_pkts < 1)   {
	    num_pkts= 1;
	}
	sendProtoMSG(CTS, msgID, entry->src_nid, entry->len, num_pkts,
                     0, NULL);
	entry->event= E_SENT_CTS;
	rtscts_stat->outstanding_pkts += num_pkts;

	#ifdef KEEP_TRACK
	    if (rtscts_stat->outstanding_pkts < 0)   {
		printk("handleDATA(%ld) C) outstanding_pkts %ld < 0!\n",
		    instance, rtscts_stat->outstanding_pkts);
	    }
	#endif /* KEEP_TRACK */

if (entry->granted_pkts != 0)   {
    printk("handleDATA(%ld Ca) entry->granted_pkts %ld != 0\n", instance, entry->granted_pkts);
}
	entry->granted_pkts= num_pkts;
    }


    /*
    ** If we're done with this message, update the active count and dequeue
    ** and free the entry.
    */
    if ((entry->datasent >= entry->len) || (pkthdr->type == LAST_DATA))   {

	if (pkthdr->type != LAST_DATA)   {
	    printk("\nhandleDATA(%ld) type %d != LAST_DATA, seq %d, allow %d\n",
		instance, pkthdr->type, pkthdr->seq, pkt_allowance);
	    printk("    data sent %d, len %d, last len %ld, retries %d\n",
		entry->datasent, entry->len, len, retries);
        #ifndef NO_ERROR_STATS
	    rtscts_stat->protoErr9++;
        #endif /* NO_ERROR_STATS */
	    disp_Qentry(entry);
	    disp_pkthdr(pkthdr);
        #ifndef NO_PROTO_DEBUG
	    p3_debug_add(entry->msgID, (ptl_hdr_t *)&entry->phdr, RCV_FINISHED,
		P3STAT_PROTO);
        #endif /* NO_PROTO_DEBUG */
	    msgDone(MSGDROP, entry, P3STAT_PROTO);
	    send_sig(SIGUSR2, entry->task, 1);
	    sendProtoMSG(MSGDROP, msgID, entry->src_nid, TRUE, 0, 0, NULL);
	    entry->event= E_SENT_MSGEND;
	} else   {
	    msgDone(MSGEND, entry, P3STAT_OK);
	}
    }

}  /* end of handleDATA() */

/* >>>  ----------------------------------------------------------------- <<< */

#ifdef DO_TIMEOUT_PROTOCOL

void
handleDataResend(unsigned int msgID)
{

Qentry_t *entry;


    /* It has to be on the sending Q */
    entry= find_queue(Qsending, msgID);
    if (!entry)   {
	/* It should be here! */
	printk("handleDataResend() No entry found for %s\n",
	    show_msgID(msgID));
	disp_queues();
    #ifndef NO_ERROR_STATS
	rtscts_stat->protoErr16++;
    #endif /* NO_ERROR_STATS */
	return;
    }

    #ifdef VERBOSE
	printk("handleDataResend(%s), len %d\n", show_msgID(msgID), entry->len);
    #endif /* VERBOSE */
/*
printk("handleDataResend(%s), len %d, retries %d, last seq %d, idle %ldms\n",
    show_msgID(msgID), entry->len, entry->retries, entry->seq, ((jiffies - entry->idle) * 1000) / HZ);
*/

    entry->idle= jiffies;
    entry->datasent= 0;		/* start over */
    entry->seq= 0;		/* start over */
    entry->retries++;
    sendDATA(entry, 1);	/* Just one packet. More later */

}  /* end of handleDataResend() */

#endif /* DO_TIMEOUT_PROTOCOL */

/* >>>  ----------------------------------------------------------------- <<< */

#ifdef MSGCKSUM
#define CKBUFSIZE	(4 * 1024)
static unsigned int tmp_data[CKBUFSIZE / sizeof(unsigned int)];

int
checksum(struct task_struct *task, char *buf, int len, int sndlen,
    unsigned int orig_crc)
{

int i;
char *usr;
int rest_len;
int cur_len;
int rc;
unsigned int crc;


    usr= buf;
    rest_len= len;
    crc= 0x5555AAAA;
    while (rest_len > 0)   {
	cur_len= MIN(CKBUFSIZE, rest_len);
	rc= memcpy2(FROM_USER, task, tmp_data, usr, cur_len);
	if (rc != PERR_NOTHING)   {
	    printk("Receive side checksum calculation failed\n");
	    printk("    task pid %d, buf %p, len %d, sndlen %d, orig_crc "
		"0x%08x\n", task->pid, buf, len, sndlen, orig_crc);
	    return FALSE;
	}
	for (i= 0; i < (cur_len / sizeof(unsigned int)); i++)   {
	    crc= crc ^ tmp_data[i];
	}
	rest_len -= cur_len;
	usr += cur_len;
    }
    /*
    printk("rcv (short) 0x%08x 0x%08x %d\n", orig_crc, crc, len);
    */
    if (orig_crc != crc)   {
	if (sndlen != len)   {
	    if (len != 0)   {
		/*
		** The sender sent more than the receiver wanted. That means
		** the receiver does the CRC on less data, so they wont
		** match. It's just a warning, because we don't know whether
		** the protion we received is correct or not.
		*/
		printk("WARNING Cannot compute CRC: snd 0x%08x, snd len %d, "
		    "rcv 0x%08x, rcv len %d\n", orig_crc, sndlen, crc, len);
	    } else   {
		/* The receiver wanted 0 bytes, so we pretend the CRC is OK */
	    }
	} else   {
	    printk("ERROR: snd 0x%08x, snd len %d, rcv 0x%08x, rcv len %d\n",
		orig_crc, sndlen, crc, len);
	    return FALSE;
	}
    }

    /*
    printk("GOOD: check sum for msg len %d (0x%08x) OK\n", len, crc);
    */
    return TRUE;
}  /* end of checksum() */

/* >>>  ----------------------------------------------------------------- <<< */

#endif /* MSGCKSUM */
