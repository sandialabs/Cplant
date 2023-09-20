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
** $Id: queue.h,v 1.34.2.2 2002/05/22 21:40:03 jbogden Exp $
** manage the queues for the rts/cts module
*/

#ifndef QUEUE_H
#define QUEUE_H

#include <p30/lib-nal.h>	/* For nal_cb_t */
#ifdef KERNEL_ADDR_CACHE
#include <compute/OS/addrCache/cache.h>	/* For addr_key_t */
#endif
#include "RTSCTS_protocol.h"	/* For pkt_type_t */


/*
** The queues are used for the following:
** Qsnd_pending Every send goes onto this queue. Eventually the RTS will be
**              acknowledged, and the send entry is released (in the case of
**              a single packet message), or moved to the Qsending queue.
** Qsending     Multi-packet messages go here after the first packet (RTS)
**              has been acknowledged. The message stays here until the last
**              data packet has been acknowledged.
** Qreceiving   If a message is longer than one packet, it gets enqueued.
**              Every incoming data packet gets handled based on information
**              from this queue.
*/
typedef enum {Qsnd_pending= 0, Qsending, Qreceiving} Q_t;
#define MAX_NUM_Q	(3)

typedef struct __Qentry_t   {
    ptl_hdr_t phdr;		/* P3 header for this message */
    int pktlen;			/* Length of first packet */
    char *buf;			/* user buffer (send or receive) */
    unsigned int msgID;
    unsigned int msgNum;	/* Point to point msg number for ordering */
    unsigned int len;   	/* How long is this message? */
    unsigned int datasent;      /* How much data has been transferred? */
    unsigned int seq;		/* Last packet sequence number */
    unsigned short src_nid;	/* Source of message */
    unsigned short dst_nid;	/* Destination of message */
    unsigned int crc;   	/* check sum */
    entry_event_t event;	/* Last event type for this entry */
    unsigned long idle;		/* Time (in jiffies) of last event */
    nal_cb_t *nal;		/* Pointer to relevant NAL (for P3 only) */
    lib_msg_t *cookie;		/* Cookie needed by P3 library */
    struct task_struct *task;	/* Pointer to relevant task struct */
    struct __Qentry_t *next;
#ifdef KERNEL_ADDR_CACHE
    addr_key_t *addr_key;       /* An address cache key structure */ 
#endif                       
    long int granted_pkts; 	/* How many grants this msg currently has */
    int lib_finalize_called; 	/* Have we done this already? */
    int retries; 		/* How many times has this msg been retried? */
    int p3msg_retries;    /* How many times has a p3 msg been retried? */
} Qentry_t;


Qentry_t *enqueue(Q_t Q, unsigned int msgID);
void free_entry(Qentry_t *entry);
void enqueue_entry(Q_t Q, Qentry_t *entry);
Qentry_t *dequeue(Q_t Q, unsigned int msgID);
Qentry_t *dequeue_first(Q_t Q);
Qentry_t *find_queue(Q_t Q, unsigned int msgID);
void clean_queue(Q_t Q, void *task);
#ifdef DO_TIMEOUT_PROTOCOL
    Qentry_t *check_idle(Q_t Q, int *pos, unsigned long idle);
    Qentry_t *find_oldest_msgNum(Q_t Q, int dest, unsigned long idle);
#endif /* DO_TIMEOUT_PROTOCOL */
void disp_queues(void);
void proc_queues(char **pb_ptr, char *pb_end);
void reset_queue(Q_t Q);
void reset_all_queues(void);
void disp_Qentry(Qentry_t *entry);
char *event_str(entry_event_t event);

#endif /* QUEUE_H */
