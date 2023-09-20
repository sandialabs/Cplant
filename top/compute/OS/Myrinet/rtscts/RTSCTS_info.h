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
** $Id: RTSCTS_info.h,v 1.10.2.1 2002/05/22 21:40:02 jbogden Exp $
*/
#ifndef RTSCTS_INFO_H
#define RTSCTS_INFO_H

#include <linux/kernel.h>	/* For pid_t */
#include "../MCP/eventlog.h"	/* For eventlog_entry_t */


/*
** These are the segements (areas) we currently support for
** info requests; i.e. this is the data that can be gotten.
*/
typedef enum {
    SEG_ROUTE_USAGE,	/* route_status */
    SEG_RTSCTS,		/* rtscts_stat */
    SEG_MYRPKT,		/* proc_stat */
    SEG_MCP,		/* MCPshmem info */
    SEG_ERROR,		/* Variuos error counters gathered in one place */
    SEG_EVENTS,		/* MCP events */
    SEG_PING,       /* just a ping */
    SEG_SENTINEL	/* Must be last! Don't move or change. */
} seg_info_t;

#define MAX_TMP_DATA	(32 * 1024)

typedef struct {
    int node;		/* What node we are request information from */
    int clear;		/* Clear the counters after reading them */
    seg_info_t seg;	/* Which segement we want */
    void *buf;		/* User buffer to point the data */
    pid_t pid;		/* ppid of the requesting process */
    unsigned int msgID;	/* The msgID for this transaction (req + return data) */
    int done;		/* All data received? */
    int rcvd;		/* How much data have we received yet? */
    char data[MAX_TMP_DATA];	/* tmp storage for data */
} get_info_req_t;

typedef struct {
    unsigned long module_send_errs;
    unsigned long module_recv_errs;
    unsigned long module_badcpy;
    unsigned long module_sendErr;
    unsigned long module_recvErr;
    unsigned long module_protErr;
    unsigned long module_outstanding_pkts;
    unsigned long module_neterrs;	/* Network errs detected at pkt lvl */
} seg_error_t;

typedef struct {
    unsigned long mcp_fres;
    unsigned long mcp_reset;
    unsigned long mcp_hst_dly;
    unsigned long mcp_crc;
    unsigned long mcp_truncated;
    unsigned long mcp_toolong;
    unsigned long mcp_send_timeout;
    unsigned long mcp_link2;
    unsigned long mcp_mem_parity;
} mcp_stat_t;

typedef struct   {
    eventlog_entry_t eventlog[EVENT_MAX];
    unsigned int event_num;
    unsigned int eventlog_next;
} eventlog_t;

int send_info_req(void *uptr);
void handleInfoReq(int src_nid, unsigned int msgID, seg_info_t seg, int clear);
void handleInfoData(int src_nid, unsigned int msgID, char *data,
	unsigned int offset, int len);
int get_info_data(void *buf);

#endif /* RTSCTS_INFO_H */
