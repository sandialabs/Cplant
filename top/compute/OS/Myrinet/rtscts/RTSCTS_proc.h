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
** $Id: RTSCTS_proc.h,v 1.27.2.3 2002/08/05 18:50:12 jbogden Exp $
** RTS/CTS module info in /proc/rtscts
*/

#ifndef RTSCTS_PROC_H
#define RTSCTS_PROC_H

#ifdef __KERNEL__
    #include <linux/proc_fs.h>
#endif /* __KERNEL__ */
#include "queue.h"
#include "RTSCTS_protocol.h"

#define MAX_PRINT_BUF		(64 * 1024)

typedef struct   {

    /* P3 messages */
    unsigned long P3recv;		/* P3 message receives */
    unsigned long P3recvlen;		/* Total bytes received in P3 msgs */
    unsigned long P3recvminlen;		/* Smallest P3 pkt received */
    unsigned long P3recvmaxlen;		/* Largest P3 msg received */
    unsigned long P3send;		/* Attempted P3 msg sends */
    unsigned long P3sendlen;		/* Total bytes sent in P3 msg */
    unsigned long P3sendminlen;		/* Smallest P3 msg sent */
    unsigned long P3sendmaxlen;		/* Largest P3 msg sent */

    unsigned long P3eventsent;		/* PTL_EVENT_SENT Put or Get */
    unsigned long P3eventrcvd;		/* PTL_EVENT_RCVD Ack or Reply */
    unsigned long P3parse;		/* Calls to lib_parse (Ack or Reply) */


    /* IP messages (packets) */
    unsigned long IPrecv;		/* IP pkt receives */
    unsigned long IPrecvlen;		/* Total bytes received in IP pkts */
    unsigned long IPrecvminlen;		/* Smallest IP pkt received */
    unsigned long IPrecvmaxlen;		/* Largest IP pkt received */
    unsigned long IPsend;		/* Attempted IP pkt sends */
    unsigned long IPsendlen;		/* Total bytes sent in IP pkts */
    unsigned long IPsendminlen;		/* Smallest IP pkt sent */
    unsigned long IPsendmaxlen;		/* Largest IP pkt sent */


    /* Packet level */
    long outstanding_pkts;		/* How many tokens are left */
    unsigned long dbg_drop_pkt;		/* Num pkts drop for debugging */
    unsigned long PKTrecv;		/* Num packets received */
    unsigned long PKTrecvlen;		/* Total len of packets received */
    unsigned long PKTrecvminlen;	/* Smallest packet received */
    unsigned long PKTrecvmaxlen;	/* Biggest packet received */
    unsigned long PKTsend;		/* successful myrPkt_xmit() */
    unsigned long PKTsendbad;		/* unsuccessful myrPkt_xmit() */
    unsigned long PKTsendlen;		/* Total len of pkts sent */
    unsigned long PKTsendminlen;	/* Smallest pkt sent */
    unsigned long PKTsendmaxlen;	/* Biggest pkt sent */


    /* Error Correction */
    #ifdef DO_TIMEOUT_PROTOCOL
	unsigned long protoP2tout;		/* P2 RTS timeout */
	unsigned long protoP3tout;		/* P3 RTS timeout */
	unsigned long protoCTSrcv_tout;		/* CTS but didn't send data */
	unsigned long protoDATAsnd_tout;	/* Data sent; No STOP or LAST */
	unsigned long protoDATAend_tout;	/* tout waiting for CTS/MSGEND*/
	unsigned long protoMSGDROPresend;	/* Num of resends of MSGDROP */
	unsigned long protoMSGENDresend;	/* Num of resends of MSGEND */
	unsigned long protoCTSresend;		/* Num of resends of CTS */
	unsigned long protoP2resend;		/* Successful P2 resend */
	unsigned long protoP3resend;		/* Successful P3 resend */
	unsigned long protoP2giveUp;		/* Give up on P2 resend */
	unsigned long protoP3giveUp;		/* Give up on P3 resend */
    #ifdef EXTENDED_P3_RTSCTS
    unsigned long protoP3sync;          /* Successful P3_SYNC send */
    unsigned long protoP3resynced;      /* Successful receiver P3 sequence resync */
    #else
    unsigned long pad12;
    unsigned long pad13;
    #endif /* EXTENDED_P3_RTSCTS */
    #else
	unsigned long pad0, pad1, pad2, pad3, pad4, pad5, pad6, pad7, pad8;
	unsigned long pad9, pad10, pad11, pad12, pad13;
    #endif /* DO_TIMEOUT_PROTOCOL */


    /* Protocol stats */
    unsigned long protoSent[MAX_NUM_PROTO]; /* protocol msg [type] sent */
    unsigned long protoRcvd[MAX_NUM_PROTO]; /* protocol msg [type] rcvd */
    unsigned long protoGC;		/* Num pkts grabage collected (no GCH)*/
    unsigned long protoGCHdone;		/* Successful GCH */
    unsigned long protoGCHdoneErr;	/* Unsuccessful GCH */
    unsigned long protoWrongMsgNum;	/* Wrong msg num: dropped by rcvr */
    unsigned long badseq;		/* Num msgs with pkts out of sequence */

    unsigned long extradata;		/* Num msgs with extra data (rcv side)*/
    unsigned long extradatalen;		/* Bytes not passed to user */
    unsigned long truncate;		/* Num msgs truncated (send side) */
    unsigned long truncatelen;		/* Bytes not sent */


    /* Module Internal & Environment */
    unsigned long ptlPktExit;		/* Process Exits */
    unsigned long ptlPktInit;		/* Process inits */
    unsigned long badcpy;		/* Num bad memcpy to/from user */
    unsigned long sendpagealloc;	/* pages allocated for send */
    unsigned long old_pending;		/* Cleared old pending sends */


    /* Send errors */
    unsigned long sendErr5;		/* build_page() Can't get send page */
    unsigned long sendErr6;		/* IP pkt len < 0 */
    unsigned long sendErr7;		/* IP pkt len > payload */
    unsigned long sendErr8;		/* P3 pkt len < 0 */
    unsigned long sendErr9;		/* build_page() pkt len > MTU size */
    unsigned long sendErr10;    /* p3_send() bad dst_nid */


    /* Receive errors */
    unsigned long P3parsebad;   /* failed calls to lib_parse() */


    /* Protocol Errors */
    unsigned long protoErr3;		/* MSGEND for unknown msg ID */
    unsigned long protoErr4;		/* MSGDROP for unknown msg ID */
    unsigned long protoErr5;		/* CTS for unknown msg ID */
    unsigned long protoErr6;		/* Data for unknown msg ID */
    unsigned long protoErr9;		/* Last data pkt != LAST_DATA */
    #ifdef DO_TIMEOUT_PROTOCOL
	unsigned long pad14;
	unsigned long protoErr15;	/* P3RESEND for unknown msg ID */
	unsigned long protoErr16;	/* DATARESEND for unknown msg ID */
	unsigned long protoErr17;	/* unknown type in Qsnd_pending */
	unsigned long protoErr18;	/* unknown type in Qsending */
	unsigned long protoErr19;	/* Illegal last_event in Qreceiving */
	unsigned long pad20;
	unsigned long protoErr21;	/* P3 resend failed */
    #else
	unsigned long pad14, pad15, pad16, pad17, pad18, pad19, pad20, pad21;
    #endif /* DO_TIMEOUT_PROTOCOL */


    /* Queue management */
    unsigned long queue_alloc;		/* queue entries allocated */
    unsigned long queue_freed;		/* queue entries freed */
    unsigned long queue_add[MAX_NUM_Q];	/* queue enqueued */
    unsigned long queue_rm[MAX_NUM_Q];	/* queue dequeued */
    unsigned long queue_size[MAX_NUM_Q];/* queue current size */
    unsigned long queue_max[MAX_NUM_Q];	/* queue largest size */
    unsigned long badenqueue;		/* Out of mem in enqueue */

} rtscts_stat_t;

#ifdef __KERNEL__
extern rtscts_stat_t *rtscts_stat;

#ifdef LINUX24
extern int rtsctsProc(char *buf, char **start, off_t off, int len, int *eof,
                      void *data);
extern int remoteProc(char *buf, char **start, off_t off, int len, int *eof,
			void *data);
extern int versions_proc(char *buf, char **start, off_t off, int len, int *eof,
                      void *data);
#else
extern int rtsctsProc(char *buf, char **start, off_t off, int len, int unused);
extern int remoteProc(char *buf, char **start, off_t off, int len, int unused);
extern int versions_proc(char *buf, char **start, off_t off, int len,
		int unused);
#endif
extern void rtsctsProcInit(void);
#endif /* __KERNEL__ */

#endif /* RTSCTS_PROC_H */
