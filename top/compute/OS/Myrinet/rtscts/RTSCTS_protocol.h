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
** $Id: RTSCTS_protocol.h,v 1.43.2.3 2002/07/10 23:34:36 jbogden Exp $
*/

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "MCPshmem.h"		/* Import MAX_NUM_ROUTES */

/*
** No packet should be idle for longer than approximately a PKT_TIMEOUT
** which is nice round multiple of 1/16 of a second.
** !!! Make sure the Myrinet switches are set to a timeout of 1/16 seconds!!!
** (The MCP sets the interface to 1/16 of a second, but the switches have
** to be done using snmp.)
*/
#define PKT_TIMEOUT                 (5 * HZ / 4)	/* in jiffies */
#define PKT_TIMEOUT_DENOMINATOR     4

/* We define a message timeout as well. The message timeout applies to Portals
 * 3 messages within the context of rtscts, i.e. P3_RTS, P3_LAST_RTS, etc.
 * All our messages are packetized and PKT_TIMEOUT handles these timeouts.
 * The message timeout is used in logic that deals with message timeouts
 * like P3 message sequencing problems for instance.
*/
#define P3_MSG_TIMEOUT  (HZ * 5)    /* in jiffies */
#define P3_MSG_RETRIES  (P3_MSG_TIMEOUT / PKT_TIMEOUT) /* num retries based on time */
 
/* Packets should be garbage collected after 2 minutes */
#define GCH_TIMEOUT     (2 * 60 * HZ)


typedef enum {CTS= 2, DATA, STOP_DATA, LAST_DATA, MSGEND,
        MSGDROP, PING, PINGA, PINGR, IP, ROUTE_STAT, 
        ROUTE_ACK, P3_RTS,
        P3_LAST_RTS, ROUTE_REQ, ROUTE_REQ_REPLY,
        #ifdef DO_TIMEOUT_PROTOCOL
        P3_RESEND,
        #else
        PAD2,
        #endif /* DO_TIMEOUT_PROTOCOL */
        GCH, INFO_REQ, INFO_DATA, CACHE_REQ, CACHE_DATA,
        P3_PING_REQ, P3_PING_ACK,
        #ifdef EXTENDED_P3_RTSCTS
        P3_NULL, P3_SYNC,
        #endif /* EXTENDED_P3_RTSCTS */
        LAST_ENTRY_DO_NOT_MOVE} pkt_type_t;

#define MAX_NUM_PROTO	(LAST_ENTRY_DO_NOT_MOVE)

typedef enum {E_CREATE= 0, E_SENT_LAST_DATA,
		E_SENT_STOP_DATA, E_SENT_DATA, E_SENT_CTS,
		E_RCVD_CTS, E_RCVD_DATA, E_RCVD_STOP_DATA,
		E_RCVD_LAST_DATA, E_SENT_P3_RTS, E_SENT_P3_LAST_RTS,
		E_RCVD_P3_RTS, E_RCVD_P3_LAST_RTS,
        E_SENT_P3_PING_REQ, E_RCVD_P3_PING_REQ,
        E_SENT_P3_PING_ACK, E_RCVD_P3_PING_ACK,
        #ifdef EXTENDED_P3_RTSCTS
        E_SENT_P3_NULL, E_SENT_P3_SYNC, E_RCVD_P3_NULL, E_RCVD_P3_SYNC,
        #endif /* EXTENDED_P3_RTSCTS */
		E_SENT_MSGDROP, E_SENT_MSGEND, E_INVALID} entry_event_t;

#define RTSCTS_PROTOCOL_version	(0x000a0000)
#define msgID_MASK		(0x001fffff)
#define msgID_SHIFT		(21)
extern int msgID2pnid(unsigned int msgID);
extern int msgID2msgNUM(unsigned int msgID);
extern char *show_msgID(unsigned int msgID);
extern void init_protocol(void);
#ifdef DO_TIMEOUT_PROTOCOL
    extern void schedule_rtscts_timeout(void);
    extern void remove_rtscts_timeout(void);
#endif /* DO_TIMEOUT_PROTOCOL */

extern const char *protocol_name_str[];


/*
** Keep track of the status of other nodes
*/
#define RMT_BOOT	0x01
#define RMT_OK		0x02
#define RMT_PENDING	0x04
#define RMT_LOST	0x08
#define RMT_TALKING	0x40

typedef struct   {
    unsigned long last_update;
    unsigned int status;
    unsigned int first_ack;
} remote_status_t;

extern remote_status_t remote_status[MAX_NUM_ROUTES];

#define BOOTMSGNUM	(0)
#define ILLMSGNUM	(1)

extern unsigned int lastMsgNum[MAX_NUM_ROUTES];
extern unsigned int send_msgNum[MAX_NUM_ROUTES];


#endif /* PROTOCOL_H */
