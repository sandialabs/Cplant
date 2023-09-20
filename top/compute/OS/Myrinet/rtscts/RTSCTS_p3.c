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
** $Id: RTSCTS_p3.c,v 1.78.2.7 2002/09/16 17:29:48 jbogden Exp $
** Functions to handle P3 packets
*/
#include <linux/kernel.h>		/* For printk() etc. */
#include <sys/defines.h>	        /* For MIN() */
#include <base/machine.h>		/* For memcpy2() */
#include <load/sys_limits.h>	/* For MAX_NODES */
#include <p30/lib-types.h>		/* For ptl_hdr_t */
#include <p30/lib-nal.h>		/* For nal_cb_t */
#include <lib-p30.h>			/* For lib_parse(), lib_finalize() */
#include "../../portals/p3mod/cb_table.h"	/* For get_cb() */
#include "../../portals/p3mod/debug.h"	/* For p3_debug_add() */
#include "hstshmem.h"			/* For hstshmem */
#include "Pkt_module.h"			/* For MYRPKT_MTU */
#include "Pkt_send.h"			/* For free_snd_page() myrPkt_xmit() */
#include "RTSCTS_proc.h"		/* For rtscts_stat */
#include "RTSCTS_protocol.h"		/* For show_msgID() */
#include "RTSCTS_send.h"		/* For sendProtoMSG() */
#include "RTSCTS_recv.h"		/* For checksum() */
#include "RTSCTS_p3.h"

/* >>>  ----------------------------------------------------------------- <<< */
/*
** Globals
*/

typedef struct   {
    struct task_struct *task;
    void *pkt;
    unsigned long len;
    unsigned int msgID;
    unsigned int msgNum;
    pkt_type_t type;
    ptl_hdr_t *hdr;
    #ifdef MSGCKSUM
	unsigned int crc;
    #endif /* MSGCKSUM */
} xfer_t;

/* These are the state variables used in handling the P3 ping stuff.
 * These are globals so we can only deal with one outstanding ping
 * request at a time.
*/
static unsigned long p3_ping_done = FALSE;
static unsigned long p3_ping_nid;

#include "RTSCTS_self.h"

/* >>>  ----------------------------------------------------------------- <<< */

void
handleP3(unsigned int msgID, unsigned int msgNum, ptl_hdr_t *hdr, void *pkt,
    unsigned long len, pkt_type_t type, struct NETDEV *dev, unsigned int crc)
{

nal_cb_t *nal;
xfer_t xfer;
#ifdef DO_TIMEOUT_PROTOCOL
Qentry_t *entry = NULL;
#endif /* DO_TIMEOUT_PROTOCOL */
unsigned int next_msgNum;
unsigned int orig_msgNum;
int rc;

    #ifdef VERY_VERBOSE
	printk("handleP3() %s, len %ld, gid %d, rid %d\n",
	show_msgID(msgID), len, hdr->src.gid, hdr->src.rid);
    if (dev) {
        printk("handleP3() dev %s\n", dev->name);
    else {
        printk("handleP3() dev UNKNOWN\n");
    }
	printk("handleP3() hdr %p:  From: %d/%d  Type: %d\n", hdr,
           hdr->src.gid, hdr->src.rid, hdr->type);
	print_hdr(nal, hdr);
    #endif /* VERY_VERBOSE */

    #ifdef DO_TIMEOUT_PROTOCOL
    /* See if this is a duplicate */
    entry= find_queue(Qreceiving, msgID);
    if (entry)   {
	/*
	** This is a duplicate RTS, figure out whether it needs a CTS
	** or a MSGEND
	*/
	#ifdef VERBOSE
	    printk("handleP3() Got duplicate RTS! %s, age %ld jiffies\n",
		show_msgID(msgID), (jiffies - entry->idle));
	    disp_Qentry(entry);
	#endif /* VERBOSE */

	switch (entry->event)   {
	    case E_SENT_MSGDROP:
		    printk("handleP3() resending MSGDROP, msgID %s\n",
			show_msgID(entry->msgID));
		sendProtoMSG(MSGDROP, entry->msgID, entry->src_nid,
		    0, 0xbbb2, 0, NULL);
		entry->event= E_SENT_MSGDROP;
        #ifndef NO_ERROR_STATS
		rtscts_stat->protoMSGDROPresend++;
        #endif /* NO_ERROR_STATS */
		break;
	    case E_SENT_MSGEND:
		#ifdef VERBOSE
		    printk("handleP3() resending MSGEND, msgID %s\n",
			show_msgID(entry->msgID));
		#endif /* VERBOSE */
		sendProtoMSG(MSGEND, entry->msgID, entry->src_nid,
		    0xccc2, 0xddd2, 0, NULL);
		entry->event= E_SENT_MSGEND;
        #ifndef NO_ERROR_STATS
		rtscts_stat->protoMSGENDresend++;
        #endif /* NO_ERROR_STATS */
		break;
	    case E_SENT_CTS:
		#ifdef VERBOSE
		    printk("handleP3() resending CTS, msgID %s\n",
			show_msgID(entry->msgID));
		#endif /* VERBOSE */
		sendProtoMSG(CTS, entry->msgID, entry->src_nid, entry->len, 1,
                             0, NULL);
		rtscts_stat->outstanding_pkts++;
		/* Take the ones back we granted earlier and are now lost */
		rtscts_stat->outstanding_pkts -= entry->granted_pkts;

		#ifdef KEEP_TRACK
		if (rtscts_stat->outstanding_pkts < 0)   {
		    printk("2) outstanding_pkts %ld < 0! We took %ld back\n",
			rtscts_stat->outstanding_pkts, entry->granted_pkts);
		}
		#endif /* KEEP_TRACK */

		entry->granted_pkts= 1;
		entry->event= E_SENT_CTS;
        #ifndef NO_ERROR_STATS
		rtscts_stat->protoCTSresend++;
        #endif /* NO_ERROR_STATS */
		break;
	    default:
		printk("handleP3() I'm confused. Existing msg (ID %08x) on recvQ with event %d\n",
                entry->msgID,entry->event);
        #ifndef NO_ERROR_STATS
		rtscts_stat->protoErr19++;
        #endif /* NO_ERROR_STATS */
		break;
	}
	return;
    }
    #endif /* DO_TIMEOUT_PROTOCOL */

    if ((lastMsgNum[hdr->src.nid] == BOOTMSGNUM) || (msgNum == BOOTMSGNUM))   {
    orig_msgNum = lastMsgNum[hdr->src.nid];
	if (msgNum == BOOTMSGNUM)   {
	    lastMsgNum[hdr->src.nid]= ILLMSGNUM;
	    #ifdef VERBOSE
		printk("MSG ordering: Node %d restarted\n", hdr->src.nid);
	    #endif /* VERBOSE */
	} else   {
	    lastMsgNum[hdr->src.nid]= msgNum;
	    #ifdef VERBOSE
		printk("MSG ordering: Hearing frm Node %d for the first time\n",
		    hdr->src.nid);
	    #endif /* VERBOSE */
	}
    } else   {
    /* Save the current msgNum from the source nid just in case we need to
     * rollback the msgNum after a failed call to lib_parse() below.
    */
    orig_msgNum = lastMsgNum[hdr->src.nid];
	next_msgNum = incMsgNum(lastMsgNum[hdr->src.nid]);
	if (msgNum == next_msgNum)   {
	    /* We're fine */
	    lastMsgNum[hdr->src.nid]= next_msgNum;
	} else   {
	    /* Message out of sequence. Drop it! */
	    #ifdef VERBOSE
	    printk("handleP3() Dropping msgID 0x%08x, snid %d, msgNum %d "
		"(expected %d) out of seq\n", msgID, hdr->src.nid, msgNum,
		next_msgNum);
	    #endif
        #ifndef NO_ERROR_STATS
	    rtscts_stat->protoWrongMsgNum++;
        #endif /* NO_ERROR_STATS */
	    return;
	}
    }

    #if defined(P3_PING) && defined(DO_TIMEOUT_PROTOCOL)
    /* OK, we've passed through the message sequencing logic. If this
     * is a P3_PING_REQ, send a P3_PING_ACK immediately. If this is
     * a P3_PING_ACK, then process the ping and send a MSGEND to the
     * sender so their send peding queue will be cleaned up.
     *
     * For the P3_PING_ACK, the portal header,hdr, has just enough
     * data for things to work, but we aren't really using the
     * portals header for portals.
     * 
     * hdr->src.nid has the sender's nid
     * hdr->src.pid has the msgID of the P3_PING_REQ we sent
    */
    if (type == P3_PING_REQ) {
        p3_send_ping_ack(hdr->src.nid,msgID);
        return;
    }
    else if (type == P3_PING_ACK) {
        if (hdr->src.nid == p3_ping_nid) {
            p3_ping_done = TRUE;
            
            /* The msgID of the P3_PING_REQ packet we sent is
             * tucked in the overloaded src.pid field of the
             * portals header.
            */
            entry = dequeue(Qsnd_pending,(unsigned int)hdr->src.pid);
            free_entry(entry);
            sendProtoMSG(MSGEND,msgID,hdr->src.nid,0xc555,0xd555,0,NULL);
            return;
        }
    }
    #endif
   
    #if defined(EXTENDED_P3_RTSCTS) && defined(DO_TIMEOUT_PROTOCOL)
    /* If this is a P3_NULL packet, we handle it specially. If we
     * are here, then the packet has successfully made it through
     * the Portals sequencing logic just before this. All we really
     * wanted the P3_NULL packet for was to run through the
     * sequencing logic anyway, so now just tell the sender to
     * drop this packet from its sending queue. The 0xdead parameter
     * is a key to handleMSGDROP on the sender to NOT send the GCH
     * to us since we didn't queue this packet on the receive queue.
    */
    if (type == P3_NULL) {
        sendProtoMSG(MSGDROP, msgID, hdr->src.nid, 0, 0xd5d5, 0, NULL);
        return;
    }
    #endif /* EXTENDED_P3_RTSCTS */
    
    nal= get_cb(hdr->nid, hdr->pid, (void *)(&xfer.task), NULL);
    if (nal == NULL)   {
	#ifdef VERBOSE
	    printk("handleP3() nid %d pid %d, has no NAL CB\n",
		hdr->nid, hdr->pid);
	#endif /* VERBOSE */
	sendProtoMSG(MSGDROP, msgID, hdr->src.nid, 0, 0xc3c3, 0, NULL);
	return;
    }

    #ifndef NO_PROTO_DEBUG
    p3_debug_add(msgID, hdr, RCV_STARTED, P3STAT_OK);
    #endif /* NO_PROTO_DEBUG */
    
    xfer.msgID= msgID;
    xfer.msgNum= msgNum;
    xfer.pkt= pkt;
    xfer.len= len;
    xfer.hdr= hdr;
    xfer.type= type;
    #ifdef MSGCKSUM
	xfer.crc= crc;
    #endif /* MSGCKSUM */

    rc = lib_parse(nal, hdr, (void *)&xfer);
    if (rc == 1) {
        /* lib_parse failed for some reason. When this happens, p3_recvBody
         * should NOT have been called. This probably indicates the
         * Portals library has hit a resource limit of some sort and is
         * unable to process an incoming message right now. We'll just
         * drop this message and the sender will continue to retry sending
         * the message to us until hopefully the Portals library is able
         * to accept the incoming message.
         *
         * The key here is to roll back to the correct msgNum for the source
         * nid (because we incremented earlier) so we don't get message
         * sequencing problems.
        */
        lastMsgNum[hdr->src.nid] = orig_msgNum;

        printk("handleP3: lib_parse() returned ERROR %d, %08x\n",rc,msgID);

        #ifndef NO_STATS
        rtscts_stat->P3parsebad++;
        #endif /* NO_STATS */
    }
    else if (rc < 0) {
        printk("handleP3: lib_parse() returned ERROR %d, %08x\n",rc,msgID);

        #ifndef NO_STATS
        rtscts_stat->P3parsebad++;
        #endif /* NO_STATS */
    }
    
    #ifndef NO_STATS
    rtscts_stat->P3parse++;
    #endif /* NO_STATS */

}  /* end of handleP3() */

/* >>>  ----------------------------------------------------------------- <<< */

/*
** rlen is the length of the incoming message.
** mlen is the length of the memory descriptor.
*/
int
p3_recvBody(nal_cb_t *nal, void *private, 
        void *data, size_t mlen, size_t rlen, lib_msg_t *cookie)
{

xfer_t *xfer;
size_t len;
int num_pkts;
entry_event_t last_event;
Qentry_t *entry;
int rc;
int assert1= FALSE;
int lib_finalize_called;
#ifdef KERNEL_ADDR_CACHE
void *addr_key = NULL;
#endif

    xfer= (xfer_t *)private;

#ifdef RTSCTS_OVER_ETHERNET
    if ( xfer->hdr->src.nid == hstshmem->my_pnid ) {
      return p3_self_recv(nal, private, data, mlen, rlen, cookie);
    }
#endif

    #ifdef KERNEL_ADDR_CACHE
	if (cookie) {
	    addr_key = cookie->md->addrkey;
	}
    #endif /* KERNEL_ADDR_CACHE */

    #ifdef VERY_VERBOSE
	printk("p3_recvBody() data %p, mlen %ld, rlen %ld\n", data, mlen, rlen);
    #endif /* VERY_VERBOSE */

    lib_finalize_called= FALSE;
    if (xfer->type == P3_RTS)   {
	last_event= E_RCVD_P3_RTS;
    } else if (xfer->type == P3_LAST_RTS)   {
	last_event= E_RCVD_P3_LAST_RTS;
    } else   {
	last_event= E_INVALID;
	printk("p3_recvBody() Invalid type %d! msgID %s\n", xfer->type,
	    show_msgID(xfer->msgID));
	return -1;
    }

    len= MIN(mlen, xfer->len);

    #ifndef NO_STATS
    if (mlen < xfer->len)   {
	rtscts_stat->extradata++;
	rtscts_stat->extradatalen += xfer->len - mlen;
    }

    rtscts_stat->P3recv++;
    rtscts_stat->P3recvlen += mlen;
    if (mlen > rtscts_stat->P3recvmaxlen)   {
	rtscts_stat->P3recvmaxlen= mlen;
    }
    if (mlen < rtscts_stat->P3recvminlen)   {
	rtscts_stat->P3recvminlen= mlen;
    }
    #endif /* NO_STATS */
    
    num_pkts= 0;
    if ((xfer->len < mlen) && (xfer->type != P3_LAST_RTS))   {
	/* There will be more packets */
	num_pkts= MAX_RCV_PKT_ENTRIES - OTHER_NODES -
	    rtscts_stat->outstanding_pkts;
	if (num_pkts > MAX_DATA_PKTS)   {
	    num_pkts= MAX_DATA_PKTS;
	} else if (num_pkts < 1)   {
	    num_pkts= 1;
	}
	sendProtoMSG(CTS, xfer->msgID, xfer->hdr->src.nid, mlen, num_pkts,
                     0, NULL);
    assert1= TRUE;
	rtscts_stat->outstanding_pkts += num_pkts;

	#ifdef KEEP_TRACK
	    if (rtscts_stat->outstanding_pkts < 0)   {
		printk("B) outstanding_pkts %ld < 0!\n",
		    rtscts_stat->outstanding_pkts);
	    }
	#endif /* KEEP_TRACK */

	last_event= E_SENT_CTS;
    }

    if (len > 0)   {
#ifdef KERNEL_ADDR_CACHE
        if( memcpy3_to_user(data, xfer->pkt, len, addr_key, xfer->task) < 0) {
	    #ifdef VERBOSE
	    printk( "p3_recvbody: memcpy3 failed! pid=%d: %p -> %p len "
		"%ld\n", xfer->task->pid, xfer->pkt, data, len );
	    #endif /* VERBOSE */
        #ifndef NO_ERROR_STATS
	    rtscts_stat->badcpy++;
        #endif /* NO_ERROR_STATS */
	    #ifdef THIS_IS_PROBABLY_ILLEGAL
		sendProtoMSG(MSGDROP, xfer->msgID, xfer->hdr->src.nid,
		    0, 0xa4a4, 0, NULL);
	    #endif /* THIS_IS_PROBABLY_ILLEGAL */
	    /* lib_finalize(nal, (void *)16, cookie); */
        #ifndef NO_PROTO_DEBUG
	    p3_debug_add(xfer->msgID, (ptl_hdr_t *)xfer->hdr, RCV_FINISHED,
		P3STAT_MEMCPYT);
        #endif /* NO_PROTO_DEBUG */
	    send_sig(SIGPIPE, xfer->task, 1);
	    last_event= E_SENT_MSGDROP;
        }
#else
	if( memcpy2(TO_USER, xfer->task, data, xfer->pkt, len) ) {
	    #ifdef VERBOSE
	    printk( "p3_recvbody: memcpy2 failed! pid=%d: %p -> %p len "
		"%ld\n", xfer->task->pid, xfer->pkt, data, len );
	    #endif /* VERBOSE */
        #ifndef NO_ERROR_STATS
	    rtscts_stat->badcpy++;
        #endif /* NO_ERROR_STATS */
	    #ifdef THIS_IS_PROBABLY_ILLEGAL
		sendProtoMSG(MSGDROP, xfer->msgID, xfer->hdr->src.nid,
		    0, 0xa4a4, 0, NULL);
	    #endif /* THIS_IS_PROBABLY_ILLEGAL */
	    /* lib_finalize(nal, (void *)16, cookie); */
        #ifndef NO_PROTO_DEBUG
	    p3_debug_add(xfer->msgID, (ptl_hdr_t *)xfer->hdr, RCV_FINISHED,
		P3STAT_MEMCPYT);
        #endif /* NO_PROTO_DEBUG */
	    send_sig(SIGPIPE, xfer->task, 1);
	    last_event= E_SENT_MSGDROP;
	}
#endif
    }

    if ((xfer->len >= mlen) || (xfer->type == P3_LAST_RTS))   {
	/* That was it, send a MSGEND */
	sendProtoMSG(MSGEND, xfer->msgID, xfer->hdr->src.nid, 0xc333, 0xd333,
                     0, NULL);
    if (assert1) printk("p3_recvBody() this should just not happen!\n");
	last_event= E_SENT_MSGEND;
	rc= lib_finalize(nal, (void *)15, cookie);
    #ifndef NO_STATS
	rtscts_stat->P3eventrcvd++;
    #endif /* NO_STATS */
	lib_finalize_called= TRUE;

	if (rc != 0)   {
	    /* lib_finalize() didn't work right. Send signal to app */
        #ifndef NO_PROTO_DEBUG
	    p3_debug_add(xfer->msgID, xfer->hdr, RCV_FINISHED, P3STAT_FINALIZE);
        #endif /* NO_PROTO_DEBUG */
	    #ifdef VERBOSE
		printk("p3_recvBody() lib_finalize() for short msg failed\n");
	    #endif /* VERBOSE */
	    send_sig(SIGPIPE, xfer->task, 1);
	} else   {
        #ifndef NO_PROTO_DEBUG
	    p3_debug_add(xfer->msgID, xfer->hdr, RCV_FINISHED, P3STAT_OK);
        #endif /* NO_PROTO_DEBUG */
	}

	#ifdef MSGCKSUM
	    if (!checksum(xfer->task, data, len, xfer->len, xfer->crc))   {
		printk("p3_recvBody() checksum failed: pid %d, spid %d, "
		    "data %p, mlen %ld, xfer->len %ld, xfer->crc 0x%08x\n",
		    xfer->hdr->pid, xfer->task->pid, data, len, xfer->len,
		    xfer->crc);
	    }
	#endif /* MSGCKSUM */
	/* Enqueue even short messages for retransmits */
    }


    /* enqueue this receive */
    entry= enqueue(Qreceiving, xfer->msgID);
    if (entry == NULL)   {
	printk("p3_recvBody() We're doomed\n");
    #ifndef NO_PROTO_DEBUG
	p3_debug_add(xfer->msgID, xfer->hdr, RCV_FINISHED, P3STAT_DOOM);
    #endif /* NO_PROTO_DEBUG */
	return -1;
    }
    #ifdef MSGCKSUM
	entry->crc= xfer->crc;
    #endif /* MSGCKSUM */
    memcpy(&(entry->phdr), xfer->hdr, sizeof(ptl_hdr_t));
    entry->event= last_event;
    entry->buf= data;
    entry->len= mlen;
    entry->pktlen= xfer->len;
    entry->datasent= len;
    entry->seq= 1;
    entry->task = xfer->task;
    entry->src_nid= xfer->hdr->src.nid;
    entry->nal= nal;
    entry->msgNum= xfer->msgNum;
    entry->cookie= cookie;
#ifdef KERNEL_ADDR_CACHE
    entry->addr_key = (addr_key_t*) addr_key;
#endif
    entry->granted_pkts= num_pkts;

    return rlen;

}  /* end of p3_recvBody() */

/* >>>  ----------------------------------------------------------------- <<< */

#ifdef DO_TIMEOUT_PROTOCOL

void
handleP3resend(unsigned int msgID, unsigned int msgNum)
{
    Qentry_t *entry = NULL;
    int rc;
    unsigned int opt = NOOP;

    #ifdef VERY_VERBOSE
	printk("handleP3resend() start\n");
    #endif /* VERBOSE */

    #ifdef NEW_P3_RESEND
    entry= find_queue(Qsnd_pending, msgID);
    #else
    entry= dequeue(Qsnd_pending, msgID);
    #endif /* NEW_P3_RESEND */

    #ifdef VERBOSE
	printk("handleP3resend() msgID 0x%08x entry=%08x buf=%08x\n",
           msgID,entry,entry->buf);
    #endif /* VERBOSE */
    
    if (!entry)   {
	    printk("handleP3resend() No entry found for %s\n", show_msgID(msgID));
	    disp_queues();
        
        #ifndef NO_ERROR_STATS
	    rtscts_stat->protoErr15++;
        #endif /* NO_ERROR_STATS */
	    return;
    }

    #ifdef NEW_P3_RESEND
    entry->p3msg_retries++;
    #endif/* NEW_P3_RESEND */

    #ifdef EXTENDED_P3_RTSCTS
    if (entry->event == E_SENT_P3_NULL)
        opt = SEND_P3_NULL;
    else
        opt = NOOP;
    #endif /* EXTENDED_P3_RTSCTS */
    
    #ifdef KERNEL_ADDR_CACHE
	rc= p3_send(entry->nal, entry->buf, entry->len, entry->dst_nid,
                (ptl_hdr_t *)&(entry->phdr), entry->cookie, msgID,
                msgNum, opt);
    #else
	rc= p3_send(entry->nal, entry->buf, entry->len, entry->dst_nid,
                (ptl_hdr_t *)&(entry->phdr), entry->cookie, msgID,
                msgNum, opt);
    #endif /* KERNEL_ADDR_CACHE */

    if (rc != P3_OK)   {
	#ifdef VERBOSE
	    printk("handleP3resend() resend failed, rc %d\n", rc);
	#endif /* VERBOSE */

	if ((rc == P3_NAL_ERROR) || (rc == P3_FAILED_BUF_MEMCPY))   {
	    /* The (local) process probably went away; stop trying to resend */
	    #ifdef VERBOSE
		printk("handleP3resend() Giving up on resend, rc %d\n", rc);
	    #endif /* VERBOSE */
        
        #ifndef NO_ERROR_STATS
	    rtscts_stat->protoP3giveUp++;
        #endif /* NO_ERROR_STATS */
        
        #ifdef NEW_P3_RESEND
        entry= dequeue(Qsnd_pending, msgID);
        #endif /* NEW_P3_RESEND */
        
	    free_entry(entry);
	} else if (rc == P3_INVALID_DEST)  {
        /* An invalid paramter was passed to p3_send(). Might as well
         * dump the packet since the parameters won't change.
        */
        #ifndef NO_ERROR_STATS
        rtscts_stat->protoP3giveUp++;
        #endif /* NO_ERROR_STATS */
        
        #ifdef NEW_P3_RESEND
        entry= dequeue(Qsnd_pending, msgID);
        #endif /* NEW_P3_RESEND */
        
	    free_entry(entry);
    } else   {
	    /* Try again later */
	    #ifdef VERBOSE
		printk("handleP3resend() Try later, msgID 0x%08x\n", msgID);
	    #endif /* VERBOSE */
        
        #ifndef NO_ERROR_STATS
	    rtscts_stat->protoErr21++;
        #endif /* NO_ERROR_STATS */
        
	    entry->idle= jiffies;	/* reset timer */

        #ifndef NEW_P3_RESEND
        enqueue_entry(Qsnd_pending, entry);
        #endif /* NEW_P3_RESEND */
	}
    } else   {
        /* there were no errors from p3_send() */
	    #ifdef VERBOSE
		printk("handleP3resend() OK, msgID 0x%08x\n", msgID);
	    #endif /* VERBOSE */

        #ifndef NO_ERROR_STATS
        rtscts_stat->protoP3resend++;
        #endif /* NO_ERROR_STATS */
        
        #ifdef NEW_P3_RESEND
        entry->idle= jiffies;	/* reset timer */
        #else
        free_entry(entry);
        #endif /* NEW_P3_RESEND */
    }

}  /* end of handleP3resend() */


/* >>>  ----------------------------------------------------------------- <<< */

#if defined(EXTENDED_P3_RTSCTS) && defined(NEW_P3_RESEND)
void
handleP3sync(unsigned int msgID, unsigned int msgNum)
{
    Qentry_t *entry = NULL;
    int rc;
    unsigned int opt = SEND_P3_SYNC;

    #ifdef VERY_VERBOSE
	printk("handleP3sync() start\n");
    #endif /* VERBOSE */

    entry= find_queue(Qsnd_pending, msgID);

    #ifdef VERBOSE
	printk("handleP3sync() msgID 0x%08x entry=%08x buf=%08x\n",
           msgID,entry,entry->buf);
    #endif /* VERBOSE */
    
    if (!entry)   {
	    printk("handleP3sync() No entry found for %s\n", show_msgID(msgID));
	    disp_queues();
        
        #ifndef NO_ERROR_STATS
	    rtscts_stat->protoErr15++;
        #endif /* NO_ERROR_STATS */
	    return;
    }

    entry->p3msg_retries++;   
    
	rc= p3_send(entry->nal, entry->buf, entry->len, entry->dst_nid,
                (ptl_hdr_t *)&(entry->phdr), entry->cookie, msgID,
                msgNum, opt);

    if (rc != P3_OK)   {
	#ifdef VERBOSE
	    printk("handleP3sync() resend failed, rc %d\n", rc);
	#endif /* VERBOSE */

	if ((rc == P3_NAL_ERROR) || (rc == P3_FAILED_BUF_MEMCPY))   {
	    /* The (local) process probably went away; stop trying to resend */
	    #ifdef VERBOSE
		printk("handleP3sync() Giving up on resend, rc %d\n", rc);
	    #endif /* VERBOSE */
        
        #ifndef NO_ERROR_STATS
	    rtscts_stat->protoP3giveUp++;
        #endif /* NO_ERROR_STATS */
        
        entry= dequeue(Qsnd_pending, msgID);
        
	    free_entry(entry);
	} else if (rc == P3_INVALID_DEST)  {
        /* An invalid paramter was passed to p3_send(). Might as well
         * dump the packet since the parameters won't change.
        */
        #ifndef NO_ERROR_STATS
        rtscts_stat->protoP3giveUp++;
        #endif /* NO_ERROR_STATS */
        
        entry= dequeue(Qsnd_pending, msgID);
        
	    free_entry(entry);
    } else   {
	    /* Try again later */
	    #ifdef VERBOSE
		printk("handleP3sync() Try later, msgID 0x%08x\n", msgID);
	    #endif /* VERBOSE */
        
        #ifndef NO_ERROR_STATS
	    rtscts_stat->protoErr21++;
        #endif /* NO_ERROR_STATS */
        
	    entry->idle= jiffies;	/* reset timer */
	}
    } else   {
        /* there were no errors from p3_send() */
	    #ifdef VERBOSE
		printk("handleP3sync() OK, msgID 0x%08x\n", msgID);
	    #endif /* VERBOSE */

        #ifndef NO_ERROR_STATS
        rtscts_stat->protoP3sync++;
        #endif /* NO_ERROR_STATS */
        
        entry->idle= jiffies;	/* reset timer */
    }    
} /* end of handleP3sync() */

#endif /* EXTENDED_P3_RTSCTS */

#endif /* DO_TIMEOUT_PROTOCOL */

/* >>>  ----------------------------------------------------------------- <<< */
/*
** Send a Portals 3.0 message.
*/
int
p3_send(nal_cb_t *nal, void *buf, size_t len, int dst_nid, ptl_hdr_t *hdr,
	lib_msg_t *cookie, unsigned int msgID, unsigned int msgNum,
    unsigned int opt)
{

unsigned long flags;
#ifndef RTSCTS_OVER_ETHERNET
int page_idx;
#else
struct sk_buff *skb= NULL;
#endif
int pktlen;
unsigned int payload;
unsigned int headers;
pkthdr_t *pkthdr_start;
ptl_hdr_t *ptlhdr_start;
unsigned char *data_start;
int rc;
unsigned int sendmsgID;
unsigned int sendmsgNum;
Qentry_t *entry = NULL;
int last_event = E_INVALID;
int type;
struct task_struct *send_task;
#ifdef KERNEL_ADDR_CACHE
    void *addr_key = NULL;
#endif
int build_page_failed;
int queue_only;
unsigned int talking;


    /* Have seen some strange instances (that seem to occur right after
     * boot or not at all) where the dest_nid is 65535 (0xffff). This
     * is weird and really shouldn't happen, but let's test for it and
     * drop packets without a valid dst_nid before we keep trying to
     * send them.
    */
    if (dst_nid > MAX_NODES && dst_nid != TEST_ROUTE_ID)   {
        #ifdef VERBOSE
        printk("p3_send() invalid dst_nid %d\n",dst_nid);
        #endif /* VERBOSE */
        
        #ifndef NO_PROTO_DEBUG
        if (msgID)
            p3_debug_add(msgID, hdr, SND_FINISHED, P3STAT_DSTNID);
        else
            p3_debug_add(0, hdr, SND_FINISHED, P3STAT_DSTNID);
        #endif /* NO_PROTO_DEBUG */
        
        lib_finalize(nal, (void *)24, cookie);
        
        #ifndef NO_ERROR_STATS
        rtscts_stat->sendErr10++;
        #endif /* NO_ERROR_STATS */
        #ifndef NO_STATS
        rtscts_stat->P3eventsent++;
        #endif /* NO_STATS */

        return P3_INVALID_DEST;
    }
    
    
#ifdef RTSCTS_OVER_ETHERNET
    if ( dst_nid == hstshmem->my_pnid ) {
      return p3_self_send(nal, buf, len, dst_nid, hdr, cookie);
    }
#endif

    save_flags(flags);
    cli();

    #ifdef KERNEL_ADDR_CACHE
	if (cookie) {
	    addr_key = cookie->md->addrkey;
	}
    #endif /* KERNEL_ADDR_CACHE */

    queue_only= FALSE;
    build_page_failed= FALSE;
#ifndef RTSCTS_OVER_ETHERNET
    page_idx= -1;
#endif
    if (msgID)   {
	/* This is a resend. Reuse the old msgID */
	sendmsgID= msgID;
	sendmsgNum= msgNum;
	if (sendmsgNum == BOOTMSGNUM)   {
	    /* We're still trying to get the first message through */
	    talking= remote_status[dst_nid].status & RMT_TALKING;
	    remote_status[dst_nid].status= RMT_PENDING | talking;
	    remote_status[dst_nid].last_update= jiffies;
	} else if (remote_status[dst_nid].first_ack == FALSE)   {
	    /*
	    ** Queue this message. We don't know yet whether we'll
	    ** be able to talk to the destination node
	    */
	    restore_flags(flags);
	    return P3_DEST_NOT_ACKED;
	}
    } else   {
	sendmsgID= next_msgID();

	/*
	** Get the next msg nummber, but don't commit to it
	** until we know we can send this message.
	*/
	sendmsgNum= next_msgNum(dst_nid, TRUE);
	if (sendmsgNum == BOOTMSGNUM)   {
	    /* This is the first time we're sending to that node */
	    talking= remote_status[dst_nid].status & RMT_TALKING;
	    remote_status[dst_nid].status= RMT_PENDING | talking;
	    remote_status[dst_nid].last_update= jiffies;
	    remote_status[dst_nid].first_ack= FALSE;
	} else if (remote_status[dst_nid].first_ack == FALSE)   {
	    /*
	    ** Queue this message. We don't know yet whether we'll
	    ** be able to talk to the destination node
	    */
	    queue_only= TRUE;
	}
    #ifndef NO_PROTO_DEBUG
	p3_debug_add(sendmsgID, hdr, SND_INITIATED, P3STAT_OK);
    #endif /* NO_PROTO_DEBUG */
    }

    if (mcpshmem == NULL)   {
	if (!msgID)   {
	    printk("p3_send: MCP is not loaded\n");
        #ifndef NO_PROTO_DEBUG
	    p3_debug_add(sendmsgID, hdr, SND_FINISHED, P3STAT_NOMCP);
        #endif /* NO_PROTO_DEBUG */
	    lib_finalize(nal, (void *)20, cookie);
        #ifndef NO_STATS
	    rtscts_stat->P3eventsent++;
        #endif /* NO_STATS */
	}
	restore_flags(flags);
	return P3_MCPSHMEM_ERROR;
    }

    #ifdef VERY_VERBOSE
	printk("p3_send() buf %p, len %ld, nid %d, msgID 0x%08x, hdr %p\n",
	    buf, len, dst_nid, sendmsgID, hdr);
    #endif /* VERY_VERBOSE */

    #ifndef NO_STATS
    /* Keep stats */
    rtscts_stat->P3send++;
    #endif /* NO_STATS */

    /*
    ** Error checking on parameters
    */
    if (len < 0)   {
	printk("p3_send() Invalid length %ld\n", len);
    #ifndef NO_ERROR_STATS
	rtscts_stat->sendErr8++;
    #endif /* NO_ERROR_STATS */
	if (!msgID)   {
        #ifndef NO_PROTO_DEBUG
	    p3_debug_add(sendmsgID, hdr, SND_FINISHED, P3STAT_LEN);
        #endif /* NO_PROTO_DEBUG */
	    lib_finalize(nal, (void *)21, cookie);
        #ifndef NO_STATS
	    rtscts_stat->P3eventsent++;
        #endif /* NO_STATS */
	}
	restore_flags(flags);
	return P3_INVALID_LEN;
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

    #ifdef MSGCKSUM
	compute_crc(hdr->src.pid, buf, len);
    #endif /* MSGCKSUM */
    headers= sizeof(pkthdr_t) + sizeof(ptl_hdr_t);
    payload= MYRPKT_MTU - headers;
    payload= MIN(payload, len);
    pktlen= headers + payload;

    #ifdef EXTENDED_P3_RTSCTS
    if (opt == SEND_P3_NULL) {
        type = P3_NULL;
        last_event = E_SENT_P3_NULL;
        /* Set payload to zero bytes so we don't have to do the
         * memcpy of the data buffer. For the P3_NULL packet, we
         * don't care about the data so let's not incur the
         * overhead. This means the length of the packet is really
         * just the headers.
        */
        payload = 0;
        pktlen = headers;
    }
    else if (opt == SEND_P3_SYNC) {
        type = P3_SYNC;
        /* Set payload to zero bytes so we don't have to do the
         * memcpy of the data buffer. For the P3_SYNC packet, we
         * don't care about the data so let's not incur the
         * overhead. This means the length of the packet is really
         * just the headers.
        */
        payload = 0;
        pktlen = headers;
    }
    else if (opt == SEND_P3_PING_REQ) {
    #else
    if (opt == SEND_P3_PING_REQ) {
    #endif /* EXTENDED_P3_RTSCTS */
        type = P3_PING_REQ;
        last_event = E_SENT_P3_PING_REQ;
    }
    else if (opt == SEND_P3_PING_ACK) {
        type = P3_PING_ACK;
        last_event = E_SENT_P3_PING_ACK;
    }
    else if (payload == len)   {
        type= P3_LAST_RTS;
        last_event= E_SENT_P3_LAST_RTS;
    } else   {
        type= P3_RTS;
        last_event= E_SENT_P3_RTS;
    }

    /* Get and prep a physical page */
    if (!queue_only)   {
#ifndef RTSCTS_OVER_ETHERNET
	page_idx= build_page(type, sendmsgID, hdr, dst_nid, payload, 
                             &data_start, &pkthdr_start, &ptlhdr_start, 
                             sendmsgNum);
	if (page_idx < 0)   {
	    #ifdef DO_TIMEOUT_PROTOCOL
		build_page_failed= TRUE;
		payload= 0;
	    #else
		if (!msgID)   {
            #ifndef NO_PROTO_DEBUG
		    p3_debug_add(sendmsgID, hdr, SND_FINISHED, P3STAT_NOPAGE);
            #endif /* NO_PROTO_DEBUG */
		    lib_finalize(nal, (void *)23, cookie);
            #ifndef NO_STATS
		    rtscts_stat->P3eventsent++;
            #endif /* NO_STATS */
		}
		restore_flags(flags);
		return P3_BUILD_PAGE_FAILED;
	    #endif
	} else   {
	    build_page_failed= FALSE;
	}
#else
	skb= build_skb(type, sendmsgID, hdr, dst_nid, payload, 
                             &data_start, &pkthdr_start, &ptlhdr_start, 
                             sendmsgNum);
#endif

    #ifdef EXTENDED_P3_RTSCTS
	if ((opt != SEND_P3_PING_REQ) && (opt != SEND_P3_PING_ACK) &&
        (opt != SEND_P3_NULL) && (opt != SEND_P3_SYNC) && 
        (get_cb(hdr->src.nid,hdr->src.pid,(void *)&send_task,NULL) != nal)) {
    #else
    if ((opt != SEND_P3_PING_REQ) && (opt != SEND_P3_PING_ACK) &&
        (get_cb(hdr->src.nid,hdr->src.pid,(void *)&send_task,NULL) != nal)) {
    #endif /* EXTENDED_P3_RTSCTS */
	    if (!msgID)   {
            #ifdef VERBOSE
            printk("p3_send(new) get_cb(nid %d, pid %d) != nal\n",
            hdr->src.nid, hdr->src.pid);
            #endif /* VERBOSE */
            #ifndef NO_PROTO_DEBUG
            p3_debug_add(sendmsgID, hdr, SND_FINISHED, P3STAT_NAL);
            #endif /* NO_PROTO_DEBUG */
            lib_finalize(nal, (void *)24, cookie);
            #ifndef NO_STATS
            rtscts_stat->P3eventsent++;
            #endif /* NO_STATS */
	    } else   {
            #ifdef VERBOSE
		    printk("p3_send(resend) get_cb(nid %d, pid %d) != nal\n",
			hdr->src.nid, hdr->src.pid);
            #endif /* VERBOSE */
	    }
#ifndef RTSCTS_OVER_ETHERNET
        free_snd_page(page_idx);
#else
        kfree_skb(skb);
#endif
	    restore_flags(flags);
	    return P3_NAL_ERROR;
	}

	if (payload > 0)   {
	    __gcc_barrier();
#ifdef KERNEL_ADDR_CACHE
	    rc= memcpy3_from_user(data_start, buf, payload, addr_key, send_task);
	    __gcc_barrier();
	    if (rc < 0)   {
        #ifndef NO_ERROR_STATS
		rtscts_stat->badcpy++;
        #endif /* NO_ERROR_STATS */
		if (!msgID)   {
            #ifndef NO_PROTO_DEBUG
		    p3_debug_add(sendmsgID, hdr, SND_FINISHED, P3STAT_MEMCPYF);
            #endif /* NO_PROTO_DEBUG */
		    #ifdef VERBOSE
			printk( "p3_send: memcpy3_from_user(%p, %p, %d) failed\n",
			    data_start, buf, payload);
		    #endif /* VERBOSE */
		    send_sig(SIGPIPE, send_task, 1);
		    /* lib_finalize(nal, (void *)25, cookie); */
		}
#ifndef RTSCTS_OVER_ETHERNET
		free_snd_page(page_idx);
#else
        kfree_skb(skb);
#endif
		restore_flags(flags);
		return P3_FAILED_BUF_MEMCPY;
	    }
#else
	    rc= memcpy2(FROM_USER, send_task, data_start, buf, payload);
	    __gcc_barrier();
	    if (rc != 0)   {
        #ifndef NO_ERROR_STATS
		rtscts_stat->badcpy++;
        #endif /* NO_ERROR_STATS */
		if (!msgID)   {
            #ifndef NO_PROTO_DEBUG
		    p3_debug_add(sendmsgID, hdr, SND_FINISHED, P3STAT_MEMCPYF);
            #endif /* NO_PROTO_DEBUG */
		    #ifdef VERBOSE
			printk( "p3_send: memcpy2(FROM_USER, %p, %p, %d) failed\n",
			    data_start, buf, payload);
		    #endif /* VERBOSE */
		    send_sig(SIGPIPE, send_task, 1);
		    /* lib_finalize(nal, (void *)25, cookie); */
		}
#ifndef RTSCTS_OVER_ETHERNET
		free_snd_page(page_idx);
#else
        kfree_skb(skb);
#endif
		restore_flags(flags);
		return P3_FAILED_BUF_MEMCPY;
	    }
#endif
	}
    }

    /*
    ** Now we enqueue this P3 RTS in the send pending list
    */
    #ifndef NEW_P3_RESEND
    abort_old_msg(sendmsgID);

    entry= enqueue(Qsnd_pending, sendmsgID);
    if (entry == NULL)   {
	if (!msgID)   {
        #ifndef NO_PROTO_DEBUG
	    p3_debug_add(sendmsgID, hdr, SND_FINISHED, P3STAT_NOMEM);
        #endif /* NO_PROTO_DEBUG */
	    lib_finalize(nal, (void *)27, cookie);
        #ifndef NO_STATS
	    rtscts_stat->P3eventsent++;
        #endif /* NO_STATS */
	}
#ifndef RTSCTS_OVER_ETHERNET
	free_snd_page(page_idx);
#else
    kfree_skb(skb);
#endif
	restore_flags(flags);
	return P3_ENQUEUE_FAILED;
    }
    entry->event= last_event;
    entry->nal= nal;
    entry->task= send_task;
    entry->cookie= cookie;
    entry->len= len;
    entry->datasent= payload;
    entry->dst_nid= dst_nid;
    entry->buf= buf;
    entry->seq= 0;
    if (!msgID)   {
	entry->msgNum= next_msgNum(dst_nid, FALSE);
	if (entry->msgNum != sendmsgNum)   {
	    printk("We have a serious problem!!!\n");
	}
    } else   {
	entry->msgNum= msgNum;
    }
    memcpy(&(entry->phdr), hdr, sizeof(ptl_hdr_t));
#ifdef KERNEL_ADDR_CACHE
    entry->addr_key = (addr_key_t*) addr_key;
#endif

    #else
    if (!msgID) {
        /* This is a NEW packet so we need to enqueue it onto the send
         * pending queue and fill in its queue entry info.
         *
         * We don't have to do this for packets that are being resent
         * because they are never taken off the send pending queue.
        */

        #if 0
        /* We shouldn't need to do this call because there shouldn't
         * be any duplicate messages ids with a new message!
        */
        abort_old_msg(sendmsgID);
        #endif

        entry= enqueue(Qsnd_pending, sendmsgID);

        if (entry == NULL)   {
            #ifndef NO_PROTO_DEBUG
	        p3_debug_add(sendmsgID, hdr, SND_FINISHED, P3STAT_NOMEM);
            #endif /* NO_PROTO_DEBUG */
            
	        lib_finalize(nal, (void *)27, cookie);
            
            #ifndef NO_STATS
	        rtscts_stat->P3eventsent++;
            #endif /* NO_STATS */
            
            #ifndef RTSCTS_OVER_ETHERNET
	        free_snd_page(page_idx);
            #else
            kfree_skb(skb);
            #endif
            
	        restore_flags(flags);
	        return P3_ENQUEUE_FAILED;
        }

	    entry->msgNum= next_msgNum(dst_nid, FALSE);
	    if (entry->msgNum != sendmsgNum)   {
	        printk("We have a serious problem!!!\n");
	    }

        entry->event= last_event;
        entry->nal= nal;
        entry->task= send_task;
        entry->cookie= cookie;
        entry->len= len;
        entry->datasent= payload;
        entry->dst_nid= dst_nid;
        entry->buf= buf;
        entry->seq= 0;

        memcpy(&(entry->phdr), hdr, sizeof(ptl_hdr_t));

        #ifdef KERNEL_ADDR_CACHE
        entry->addr_key = (addr_key_t*) addr_key;
        #endif
    }
    else {
        entry= find_queue(Qsnd_pending, sendmsgID);

        /* For reasons not completely clear, sometimes the sending task's
         * task_struct changes upon resends. So, we have to make sure we
         * have the right task_struct every time we resend. The proper
         * send_task comes from the get_cb() call above.
        */
        #ifdef VERBOSE
        if (entry->task != send_task) { 
            printk("p3_send() entry->task %08x != send_task %08x\n",entry->task,send_task);
            /*printk("p3_send() (entry->task->comm = %s)\n",entry->task->comm);*/
            /*printk("p3_send() (task->comm = %s)\n",send_task->comm);*/
        }
        #endif /* VERBOSE */
        entry->task= send_task;
    }
    #endif /* NEW_P3_RESEND */

    #ifdef MSGCKSUM
	pkthdr_start->crc= crc;
    #endif /* MSGCKSUM */

    if (!build_page_failed && !queue_only)   {
#ifndef RTSCTS_OVER_ETHERNET
	rc= myrPkt_xmit(dst_nid, page_idx, pktlen);
#else
	rc= skb_xmit(dst_nid, skb);
#endif
    
	if (rc < 0)   {
	    /*
	    ** The packet couldn't be sent! We'll try again later.
	    ** No need to call free_snd_page(), because myrPkt_xmit()
	    ** has already done that.
	    */
        #ifndef NO_ERROR_STATS
	    rtscts_stat->PKTsendbad++;
        #endif /* NO_ERROR_STATS */
	    #if !defined(DO_TIMEOUT_PROTOCOL)
		if (!msgID)   {
            #ifndef NO_PROTO_DEBUG
            p3_debug_add(sendmsgID, hdr, SND_FINISHED, P3STAT_BADXMIT);
            #endif /* NO_PROTO_DEBUG */
		    lib_finalize(nal, (void *)26, cookie);
            #ifndef NO_STATS
		    rtscts_stat->P3eventsent++;
            #endif /* NO_STATS */
		}
		restore_flags(flags);
		return P3_XMIT_FAILED;
	    #endif
	} else   {
        #ifndef NO_STATS
	    rtscts_stat->PKTsend++;
	    rtscts_stat->PKTsendlen += pktlen;
	    if (pktlen > rtscts_stat->PKTsendmaxlen)   {
		rtscts_stat->PKTsendmaxlen= pktlen;
	    }
	    if (pktlen < rtscts_stat->PKTsendminlen)   {
		rtscts_stat->PKTsendminlen= pktlen;
	    }
        #endif /* NO_STATS */
	}
    }

    #ifndef NO_PROTO_DEBUG
    if (!msgID)   {
	p3_debug_add(sendmsgID, hdr, SND_STARTED, P3STAT_OK);
    }
    #endif /* NO_PROTO_DEBUG */

    #ifndef NO_STATS
    rtscts_stat->protoSent[type]++;
    #endif /* NO_STATS */

    #ifdef DO_TIMEOUT_PROTOCOL
	/*
	** We have to keep the user space buffer around until we get a
	** MSGEND, in case we have to do a resend.
	*/
    #ifndef NO_PROTO_DEBUG
	if (!msgID)   {
	    p3_debug_add(sendmsgID, hdr, SND_QUEUED, P3STAT_OK);
	}
    #endif /* NO_PROTO_DEBUG */
    #else
	/*
	** If this message fits into a single packet, then we can increment the
	** send flag now. We're assuming all packets go through!
	*/
	if (len == payload)   {
	    if (!msgID)   {
		#ifdef VERY_VERBOSE
		    printk("(2) telling NAL to set PTL_EVENT_SENT\n");
		#endif /* VERY_VERBOSE */
		rc= 0;
		if (!entry->lib_finalize_called)   {
		    rc= lib_finalize(nal, (void *)28, cookie);
            #ifndef NO_STATS
		    rtscts_stat->P3eventsent++;
            #endif /* NO_STATS */
		    entry->lib_finalize_called= TRUE;
		}
		if (rc != 0)   {
		    #ifdef VERBOSE
		    printk("(2) lib_finalize() failed for PTL_EVENT_SENT\n");
		    #endif /* VERBOSE */
		    send_sig(SIGPIPE, send_task, 1);
		}
	    }
	} else if (!msgID)   {
	    (sendmsgID, hdr, SND_QUEUED, P3STAT_OK);
	}
    #endif /* DO_TIMEOUT_PROTOCOL */

    restore_flags(flags);
    return P3_OK;

}  /* end of p3_send() */

/* >>>  ----------------------------------------------------------------- <<< */

#define MAX_HOP_COUNT 256

/*
** Calculate and return the hop count
*/
int
p3_dist(nal_cb_t *nal, int nid, unsigned long *dist )
{
    unsigned long hopcount;

    *dist = 0;

    if ( nid >= MAX_NUM_ROUTES ) {
	return -1;
    }

    if ( (hopcount = strnlen( (char *)mcpshmem->route[nid], MAX_HOP_COUNT )) == MAX_HOP_COUNT ) {
	return -1;
    } else {
	*dist = hopcount;
	return 0;
    }


} /* end of p3_dist() */


/* >>>  ----------------------------------------------------------------- <<< */

void
p3_exit(void *task)
{

unsigned long flags;


    save_flags(flags);
    cli();

    clean_queue(Qsnd_pending, task);
    clean_queue(Qsending, task);
    clean_queue(Qreceiving, task);
    restore_flags(flags);

}  /* end of p3_exit() */


/* >>>  ----------------------------------------------------------------- <<< */

int
p3_send_ping(unsigned long nid)
{
    #ifdef P3_PING
    ptl_hdr_t dummy;
    
    p3_ping_done = FALSE;
    p3_ping_nid = nid;
    dummy.src.nid = hstshmem->my_pnid;
    
    return p3_send(NULL,NULL,0,nid,&dummy,NULL,0,0,SEND_P3_PING_REQ);
    #else
    return -1;
    #endif
}


/* >>>  ----------------------------------------------------------------- <<< */

int
p3_send_ping_ack(unsigned long nid, unsigned int msgID)
{
    #ifdef P3_PING
    ptl_hdr_t dummy;
    
    /* We need to put just enough data in the portals header to get
     * us by, but we aren't really using it. Also, we are using
     * the src.pid parameter to send the msgID info.
    */
    dummy.src.nid = hstshmem->my_pnid;
    dummy.src.pid = (int)msgID;
    
    /* In the payload of the P3_PING_ACK packet we send the msgID
     * of the P3_PING_REQ packet we are responding to. This way
     * the sender can clean up its send pending queue.
    */
    return p3_send(NULL,NULL,0,nid,&dummy,NULL,0,0,SEND_P3_PING_ACK);
    #else
    return -1;
    #endif
}


/* >>>  ----------------------------------------------------------------- <<< */

int
p3_get_ping(unsigned long nid)
{
    if (p3_ping_done  &&  nid == p3_ping_nid) {
        return 0;
    }
    else if (!p3_ping_done  &&  nid == p3_ping_nid) {
        return -1;
    }

    /* must be requesting ping status for a nid that wasn't pinged! */
    return -2;
}
