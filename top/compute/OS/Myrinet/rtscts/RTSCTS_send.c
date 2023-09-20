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
** $Id: RTSCTS_send.c,v 1.81.2.3 2002/08/05 18:50:12 jbogden Exp $
** The functions to send a message
*/
#include <linux/sched.h>		/* For jiffies */
#include <linux/mm.h>			/* For verify_area() */
#include <asm/string.h>			/* For memcpy() */
#include <asm/byteorder.h>		/* For htons() */
#include <asm/segment.h>		/* For memcpy_fromfs() */

#include <defines.h>			/* For TRUE. FALSE, MIN, MAX */
#include <portals/base/machine.h>	/* For FROM_USER */

#include <p30/lib-types.h>		/* For ptl_hdr_t */
#include <p30/lib-nal.h>		/* For nal_cb_t */

#include "../../portals/p3mod/cb_table.h"	/* For get_cb() */
#include "../../portals/p3mod/debug.h"	/* For p3_debug_add() */

#include "MCPshmem.h"
#include "hstshmem.h"
#include "Pkt_module.h"
#include "queue.h"
#include "RTSCTS_proc.h"
#include "RTSCTS_send.h"
#include "RTSCTS_ioctl.h"		/* For ROUTE_NO_ANSWER, route_status */
#include "RTSCTS_protocol.h"		/* For msgID_* */
#include "RTSCTS_pkthdr.h"		/* For pkthdr_t */
#include "Pkt_send.h"

#ifdef RTSCTS_OVER_ETHERNET
#include "RTSCTS_sizeof.h"
#include "../../ptRXTX/ptRXTX.h"
extern struct NETDEV *ptRXTX_dev;
extern char macAddr[NODES][ETH_ALEN];
#endif


/* >>>  ----------------------------------------------------------------- <<< */
/*
** Globals
*/
#ifdef MSGCKSUM
unsigned int crc;
#endif /* MSGCKSUM */


/* >>>  ----------------------------------------------------------------- <<< */

unsigned int
next_msgID(void)
{

static unsigned int msgID= 0;
unsigned long flags;
unsigned int cnt;


    save_flags(flags);
    cli();

    if (msgID == 0)   {
	/* First time! */
	/*
	** The message ID in each packet and queue entry uniquely identifies
	** a message. The same msgID is used on the send side and the receive
	** side for a given message. A msgID looks like this:
	**
	**  11 bits     21 bits
	** +-----------+--------------+
	** | pnid  + 1 | cnt          |
	** +-----------+--------------+
	**
	** Where pnid is the physical node ID of the sender, and cnt is a
	** sequence number (that wraps around). The msgIDs are not truly
	** unique, since they repeat after 2^21 (2048 k) messages from a given
	** node. This has to be balanced with the GCH_TIMEOUT parameter
    ** so that msgIDs can't rollover before they would be ultimately be
    ** cleaned out by clean_recv_queues() in the worst case that all
    ** GCH packets were dropped (and thus all messages were hanging
    ** around in receive queues.
	*/
	msgID= ((hstshmem->my_pnid + 1) << msgID_SHIFT);
    } else   {
	/* Advance the message ID counter */
        cnt= msgID & msgID_MASK;
        cnt= (cnt + 1) & msgID_MASK;
        msgID= (msgID & ~msgID_MASK) | cnt;
        
        #if 0
        /* debug check for cnt wrapping */
        if ((msgID & msgID_MASK) == 0) {
            printk ("next_msgID() msgID cnt field wrapped\n");
        }
        #endif
    }

    restore_flags(flags);
    return msgID;

}  /* end of next_msgID() */

/* >>>  ----------------------------------------------------------------- <<< */
/*
** Bewteen our node and any given other node, we need a message number
** to preserve message ordering. Sending a msgNum == 0 means this is
** the first message from this node to the receiver (since reboot).
*/
unsigned int
next_msgNum(int dest_nid, int read_only)
{

unsigned int ret;
unsigned long flags;


    save_flags(flags);
    cli();

    ret= send_msgNum[dest_nid];
    if (!read_only)   {
        send_msgNum[dest_nid]= incMsgNum(send_msgNum[dest_nid]);
        
        #ifdef DROP_P3_SEQ_NUM_TEST
        {
            static long cnt = 0;
            
	        if (((cnt >> 14) & 0x01) == 1)   {
		        cnt= 0;
		        printk("next_msgNum() Corrupting P3 seq num to nid %d, new=%d old=%d\n",
                       dest_nid,send_msgNum[dest_nid]+10,send_msgNum[dest_nid]);
                send_msgNum[dest_nid] = send_msgNum[dest_nid] += 10;
	        }
	        cnt++;
        }
        #endif /* DROP_P3_SEQ_NUM_TEST */
    }

    restore_flags(flags);
    return ret;

}  /* end of next_msgNum() */

/* >>>  ----------------------------------------------------------------- <<< */

unsigned int
incMsgNum(unsigned int num)
{

    num++;
    if ((num == BOOTMSGNUM) || (num == ILLMSGNUM))   {
        num++;
    }
    if ((num == BOOTMSGNUM) || (num == ILLMSGNUM))   {
        num++;
    }

    return num;

}  /* end of incMsgNum() */

/* >>>  ----------------------------------------------------------------- <<< */


/*
** Send a protocol message
*/
void
sendProtoMSG(pkt_type_t type, unsigned int msgID, unsigned short dst_nid,
	unsigned int info, unsigned short int info2,
        int payload, void *buf)
{

pkthdr_t *pkthdr;
#ifndef RTSCTS_OVER_ETHERNET
int page_idx;
#else
struct sk_buff *skb;
#endif
unsigned char *data;
int rc;
unsigned long flags;


    #ifdef VERBOSE
	printk("sendProtoMSG: type=    %d\n", type);
	printk("sendProtoMSG: msgID=   %d\n", msgID);
	printk("sendProtoMSG: dst_nid= %d\n", dst_nid);
	printk("sendProtoMSG: mcpshmem->test_route[0]= 0x%x\n",
	    mcpshmem->test_route[0]);
	printk("sendProtoMSG: mcpshmem->test_route_len= %d\n",
	    (int)ntohl(mcpshmem->test_route_len));
    #endif


    if (payload > (MYRPKT_MTU - sizeof(pkthdr_t)))   {
	printk("sendProtoMSG() payload of %d bytes too large!\n", payload);
	return;
    }

    save_flags(flags);
    cli();

    /*
    ** Get and prep a physical page
    ** 3rd arg = NULL = no Puma Portal header
    ** 5th arg = 0    = no data
    ** 8th arg = NULL = no Portal header in page
    */
#ifndef RTSCTS_OVER_ETHERNET
    page_idx= build_page(type, msgID, NULL, dst_nid, payload, &data, 
                         &pkthdr, NULL, ILLMSGNUM);
    if (page_idx < 0)   {
	#ifdef VERBOSE
	printk("sendProtoMSG() Could not allocate a page\n");
	#endif /* VERBOSE */
	restore_flags(flags);
	return;
    }
#else
    skb= build_skb(type, msgID, NULL, dst_nid, payload, &data, 
                         &pkthdr, NULL, ILLMSGNUM);
#endif

    if (payload && buf)   {
	memcpy(data, buf, payload);
    }

    pkthdr->info= info;
    pkthdr->info2= info2;
#ifndef RTSCTS_OVER_ETHERNET
    rc= myrPkt_xmit(dst_nid, page_idx, sizeof(pkthdr_t) + payload);
#else
    rc= skb_xmit(dst_nid, skb);
#endif
    if (rc < 0)   {
    #ifndef NO_ERROR_STATS
	rtscts_stat->PKTsendbad++;
    #endif /* NO_ERROR_STATS */
    } else   {
    #ifndef NO_STATS
	rtscts_stat->protoSent[type]++;
	rtscts_stat->PKTsend++;
	rtscts_stat->PKTsendlen += sizeof(pkthdr_t) + payload;
	if (sizeof(pkthdr_t) + payload > rtscts_stat->PKTsendmaxlen)   {
	    rtscts_stat->PKTsendmaxlen= sizeof(pkthdr_t) + payload;
	}
	if (sizeof(pkthdr_t) + payload < rtscts_stat->PKTsendminlen)   {
	    rtscts_stat->PKTsendminlen= sizeof(pkthdr_t) + payload;
	}
    #endif /* NO_STATS */
    }
    restore_flags(flags);

}  /* end of sendProtoMSG() */

/* >>>  ----------------------------------------------------------------- <<< */

void
sendDATA(Qentry_t *entry, int num_pkts)
{

#ifndef RTSCTS_OVER_ETHERNET
int page_idx;
#else
struct sk_buff *skb;
#endif
int i;
unsigned int pktlen;
unsigned int remain;
unsigned int offset;
unsigned int headers;
unsigned int payload;
pkthdr_t *pkthdr;
unsigned char *data;
int rc;
pkt_type_t pkttype;
entry_event_t last_event;

    #ifdef VERBOSE
    printk("sendDATA() entry=%08x offset=%d\n",entry,entry->datasent);
    #endif /* VERBOSE */
    
    remain= entry->len - entry->datasent;
    offset= entry->datasent;
    headers= sizeof(pkthdr_t);
    last_event= entry->event;
    entry->idle= jiffies;

    /*
    ** We were given permission by the receiver to send up to num_pkts packets
    */
    for (i= 0; i < num_pkts; i++)   {
	payload= MYRPKT_MTU - headers;
	payload= MIN(payload, remain);
	pktlen= headers + payload;

	if (payload == remain)   {
	    /* This is the last packet for this message */
	    pkttype= LAST_DATA;
	    last_event= E_SENT_LAST_DATA;
	} else   {
	    /* There will be more data packets. */
	    if (i >= (num_pkts - 1))   {
		/*
		** We got more data to send, but will stop after this one
		** because we sent all the packets we were given permission
		** for. STOP_DATA will signal the receiver that we need
		** another "quota" to send the rest of the data.
		*/
		pkttype= STOP_DATA;
		last_event= E_SENT_STOP_DATA;
	    } else   {
		pkttype= DATA;
		last_event= E_SENT_DATA;
	    }
	}

	/*
	** 3rd arg = NULL = no Puma Portal header
	** 8th arg = NULL = no Portal header in page
	*/
#ifndef RTSCTS_OVER_ETHERNET
	page_idx= build_page(pkttype, entry->msgID, NULL, 
                  entry->dst_nid,
                  payload, &data, &pkthdr, NULL, entry->msgNum);
	if (page_idx < 0)   {
	    #ifdef DO_TIMEOUT_PROTOCOL
		/* fake a successful send for timeout_send_queues() */
		last_event= E_SENT_STOP_DATA;
		break;
	    #else
		if (!entry->nal)   {
		    printk("sendDATA() 1 This should not happen...\n");
		}
        #ifndef NO_PROTO_DEBUG
		p3_debug_add(entry->msgID, (ptl_hdr_t *)&(entry->phdr),
		    SND_FINISHED, P3STAT_BUILDPG);
        #endif /* NO_PROTO_DEBUG */
		entry->datasent= offset;
		entry->event= last_event;
		return;
	    #endif
	}
#else
	skb= build_skb(pkttype, entry->msgID, NULL, 
                       entry->dst_nid,
                       payload, &data, &pkthdr, NULL, entry->msgNum);
#endif

#ifdef KERNEL_ADDR_CACHE
	if ( memcpy3_from_user(data, entry->buf + offset, payload, 
                                                        entry->addr_key, 
                                                        entry->task) <0 ) {
	    #ifdef VERBOSE
	    printk("sendDATA() i %d, buf %p, off %d, addr %p\n", i,
		entry->buf, offset, entry->buf + offset);
	    #endif /* VERBOSE */
	    if (!entry->nal)   {
		printk("sendDATA() 2 This should not happen...\n");
	    }
        #ifndef NO_PROTO_DEBUG
	    p3_debug_add(entry->msgID, (ptl_hdr_t *)&(entry->phdr),
		SND_FINISHED, P3STAT_BADCPY);
        #endif /* NO_PROTO_DEBUG */
	    send_sig(SIGPIPE, entry->task, 1);
        #ifndef NO_ERROR_STATS
	    rtscts_stat->badcpy++;
        #endif /* NO_ERROR_STATS */
#ifndef RTSCTS_OVER_ETHERNET
	    free_snd_page(page_idx);
#else
        kfree_skb(skb);
#endif
	    dequeue(Qsending, entry->msgID);
	    free_entry(entry);
	    return;
        }
#else
	if (memcpy2(FROM_USER, entry->task, data, entry->buf + offset, payload)
		!= 0)   {
	    #ifdef VERBOSE
	    printk("sendDATA() i %d, buf %p, off %d, addr %p\n", i,
		entry->buf, offset, entry->buf + offset);
	    #endif /* VERBOSE */
	    if (!entry->nal)   {
		printk("sendDATA() 2 This should not happen...\n");
	    }
        #ifndef NO_PROTO_DEBUG
	    p3_debug_add(entry->msgID, (ptl_hdr_t *)&(entry->phdr),
		SND_FINISHED, P3STAT_BADCPY);
        #endif /* NO_PROTO_DEBUG */
	    send_sig(SIGPIPE, entry->task, 1);
        #ifndef NO_ERROR_STATS
	    rtscts_stat->badcpy++;
        #endif /* NO_ERROR_STATS */
#ifndef RTSCTS_OVER_ETHERNET
	    free_snd_page(page_idx);
#else
        kfree_skb(skb);
#endif
	    dequeue(Qsending, entry->msgID);
	    free_entry(entry);
	    return;
	}
#endif

	#ifdef CORRUPT_P3_SEND_TEST
	/*
	** If you want to occasionaly corrupt messages, turn this on.
	*/
	{
	static long cnt= 0;

	    if (((cnt >> 15) & 0x01) == 1)   {
		    cnt= 0;
		    printk("p3_send() Corrupting data byte 0x%2x\n", *(data + 6000));
		    *(data + 6000)= 0x55;
	    }
	    cnt++;
	}
	#endif /* CORRUPT_P3_SEND_TEST */

	pkthdr->seq= ++(entry->seq);
	/*
	** Tell receiver how many packet we were allowed to send. This
	** is needed for bookkeeping on the receiver side.
	*/
	pkthdr->info= num_pkts;
	pkthdr->info2= entry->retries;
#ifndef RTSCTS_OVER_ETHERNET
	rc= myrPkt_xmit(entry->dst_nid, page_idx, pktlen);
#else
	rc= skb_xmit(entry->dst_nid, skb);
#endif
	if (rc < 0)   {
	    /* We'll try again later */
        #ifndef NO_ERROR_STATS
	    rtscts_stat->PKTsendbad++;
        #endif /* NO_ERROR_STATS */
	    #ifdef DO_TIMEOUT_PROTOCOL
		/* fake a successful send for timeout_send_queues() */
		last_event= E_SENT_STOP_DATA;
		break;
	    #else
		entry->datasent= offset;
		entry->event= last_event;
		return;
	    #endif
	}
    
    #ifndef NO_STATS
	rtscts_stat->protoSent[pkttype]++;
	rtscts_stat->PKTsend++;
	rtscts_stat->PKTsendlen += pktlen;
	if (pktlen > rtscts_stat->PKTsendmaxlen)   {
	    rtscts_stat->PKTsendmaxlen= pktlen;
	}
	if (pktlen < rtscts_stat->PKTsendminlen)   {
	    rtscts_stat->PKTsendminlen= pktlen;
	}
    #endif /* NO_ERROR_STATS */
    
	remain= remain - payload;
	offset= offset + payload;
	if (remain == 0)   {
	    break;
	}
    }

    entry->datasent= offset;
    entry->event= last_event;

}  /* end of sendDATA() */

/* >>>  ----------------------------------------------------------------- <<< */

#ifndef RTSCTS_OVER_ETHERNET
int
build_page(pkt_type_t type, unsigned int msgID, ptl_hdr_t *phdr,
	unsigned short dst_nid, int payload, unsigned char **data_start,
	pkthdr_t **pkthdr_start, ptl_hdr_t **ptlhdr_start,
	unsigned int msgNum)
{

int pktlen;
unsigned long offset;
unsigned int headers;
int page_idx;

    /* Get and prep a page */
    if (phdr)   {
	headers= sizeof(pkthdr_t) + sizeof(ptl_hdr_t);
    } else   {
	headers= sizeof(pkthdr_t);
    }
    pktlen= payload + headers;
    if (pktlen > MYRPKT_MTU)   {
	printk("build_page() packet %d too long for MTU %ld\n", pktlen,
	    MYRPKT_MTU);
    #ifndef NO_ERROR_STATS
	rtscts_stat->sendErr9++;
    #endif /* NO_ERROR_STATS */
	return -1;
    }

    page_idx= get_snd_page();
    if (page_idx < 0)   {
	#ifdef VERBOSE
	    printk("build_page() Could not allocate a send page\n");
	#endif /* VERBOSE */
    #ifndef NO_ERROR_STATS
	rtscts_stat->sendErr5++;
    #endif /* NO_ERROR_STATS */
	return -1;
    }

    #ifndef NO_STATS
    rtscts_stat->sendpagealloc++;
    #endif /* NO_STATS */
    
    *pkthdr_start= (pkthdr_t *)page_addr(page_idx);
    (*pkthdr_start)->version= RTSCTS_PROTOCOL_version | MCP_version;
    (*pkthdr_start)->type= type;
    (*pkthdr_start)->seq= 0;
    (*pkthdr_start)->msgID= msgID;
    (*pkthdr_start)->info= 0x01234567;
    (*pkthdr_start)->info2= 0x98765432;
    (*pkthdr_start)->msgNum= msgNum;

    offset= sizeof(pkthdr_t);

    if (phdr)   {
	*ptlhdr_start= (ptl_hdr_t *)(page_addr(page_idx) + offset);

	/*
	** memcpy() is OK (vs. copy_from_user()) because headers
	** are built in kernel space.
	*/
	memcpy(*ptlhdr_start, phdr, sizeof(ptl_hdr_t));
	offset += sizeof(ptl_hdr_t);
    }

    *data_start= (unsigned char *)(page_addr(page_idx) + offset);

    return page_idx;

}  /* end of build_page() */
#else
struct sk_buff*
build_skb(pkt_type_t type, unsigned int msgID, ptl_hdr_t *phdr,
          unsigned short dst_nid, int payload, 
          unsigned char **data_start,
          pkthdr_t **pkthdr_start, ptl_hdr_t **ptlhdr_start,
          unsigned int msgNum)
{

int pktlen, offset; 
struct sk_buff *skb;

    if (phdr) {
      offset = HDR_SZ;
    }
    else {
      offset = PKTHDR_SZ;
    }
    pktlen= payload + offset;

    skb = dev_alloc_skb(pktlen+ETH_HLEN);  

    skb->dev = ptRXTX_dev;
    skb->protocol = __constant_htons(PORTALS_PROT_ID);

    ptRXTX_dev->hard_header(skb, ptRXTX_dev,
                            ntohs(skb->protocol),
                            macAddr[(int) dst_nid], 
                            ptRXTX_dev->dev_addr,
                            skb->len);

    skb->len = pktlen + ETH_HLEN;

    /* do this after call to dev->hard_header... */
    /* this is not the final value...            */
   *data_start =  (unsigned char *) skb->data + ETH_HLEN;

    if (phdr) {
     *ptlhdr_start= (ptl_hdr_t *)( *data_start + PKTHDR_SZ);
      memcpy(*ptlhdr_start, phdr, PTLHDR_SZ);
    }

    *pkthdr_start= (pkthdr_t *) *data_start;
    (*pkthdr_start)->version= RTSCTS_PROTOCOL_version | MCP_version;
    (*pkthdr_start)->type= type;
    (*pkthdr_start)->len= pktlen;
    (*pkthdr_start)->src_nid= hstshmem->my_pnid;
    (*pkthdr_start)->seq= 0;
    (*pkthdr_start)->msgID= msgID;
    (*pkthdr_start)->info= 0x01234567;
    (*pkthdr_start)->info2= 0x98765432;
    (*pkthdr_start)->msgNum= msgNum;

    *data_start+= offset;
    return skb;

}  /* end of build_skb() */
#endif

/* >>>  ----------------------------------------------------------------- <<< */

#ifdef MSGCKSUM
#define CKBUFSIZE	(4 * 1024)
static unsigned int tmp_data[CKBUFSIZE / sizeof(unsigned int)];

void
compute_crc(int src_pid, char *buf, int len)
{

int i;
char *usr;
int rest_len;
int cur_len;
int rc;

    usr= buf;
    rest_len= len;
    crc= 0x5555AAAA;
    while (rest_len > 0)   {
	cur_len= MIN(CKBUFSIZE, rest_len);
	rc= memcpy_fromfs2(src_pid, tmp_data, usr, cur_len);
	if (rc != PERR_NOTHING)   {
	    printk("Send side checksum calculation failed\n");
	    printk("    src_pid %d, buf %p, len %d, rest_len %d\n",
		src_pid, buf, len, rest_len);
	    crc= 0;
	    return;
	}
	for (i= 0; i < (cur_len / sizeof(unsigned int)); i++)   {
	    crc= crc ^ tmp_data[i];
	}
	rest_len -= cur_len;
	usr += cur_len;
    }
    /*
    printk("snd 0x%08x %d\n", crc, len);
    */

}  /* end of compute_crc() */

#endif /* MSGCKSUM */

/* >>>  ----------------------------------------------------------------- <<< */

void
abort_old_msg(unsigned int msgID)
{
    Qentry_t *old_entry = dequeue(Qsnd_pending, msgID);

    if( !old_entry )
	return;


   /*
   ** Found another entry with the same msgID. This must be an old
   ** send that never got a CTS (e.g. because the destination wasn't
   ** running the rtscts module yet). Clear it out and make room for
   ** the new send.
   */
   #ifndef NO_ERROR_STATS
   rtscts_stat->old_pending++;
   #endif /* NO_ERROR_STATS */

   printk( KERN_DEBUG
	"Old P%c message %d was in send pending queue "
	"from snid %d to nid %d, entered %ld seconds ago\n",
	old_entry->nal ? '3' : '2',
	msgID,
	old_entry->src_nid,
	old_entry->dst_nid,
	(jiffies - old_entry->idle) / HZ
   );

   if( old_entry->nal ) {

	/* Check if this is still true! */

	/*
	 * Portals 3 message
	 * The call to lib_finalize() fails occasionally if the
	 * message was from a process that has since exited.
	 * This caused a nasty bug that Doug ran into with mpi_bw2.
	 * A better solution might be to dump all the pending entries
	 * from the send queue, but it doesn't really matter at this
	 * point.
	 */

        /*
        ** lib_finalize(old_entry->nal, (void *)1, old_entry->cookie);
	** rtscts_stat->P3eventsent++;
        */
    #ifndef NO_PROTO_DEBUG
	p3_debug_add(old_entry->msgID, (ptl_hdr_t *)&(old_entry->phdr),
		SND_DEQUEUED, P3STAT_ABORTED);
    #endif /* NO_PROTO_DEBUG */
    }
    free_entry(old_entry);

}  /* end of abort_old_msg() */

/* >>>  ----------------------------------------------------------------- <<< */

