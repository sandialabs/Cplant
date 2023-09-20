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
** $Id: debug.c,v 1.9 2002/02/20 22:30:45 jbogden Exp $
** Debugging information about P3 messages based on progress through the
** rts/cts protocol.
*/

#include <linux/sched.h>	/* For jiffies, HZ */
#include "debug.h"

#define MAX_DEBUG_EVENTS	(256)

typedef struct   {
    unsigned int msgID;
    p3_event_type_t event;
    p3_status_t status;
    ptl_hdr_t hdr;
    unsigned long int time;
} p3_debug_event_t;

static p3_debug_event_t debug_event[MAX_DEBUG_EVENTS];
static int last_debug_event;

const char *p3_debug_errstr[] =   {
    [P3STAT_OK] =	"No Errors",
    [P3STAT_NOMEM] =	"Out of memory",
    [P3STAT_NOPAGE] =	"No free send pages",
    [P3STAT_BADCPY] =	"Bad memcpy to/from user space",
    [P3STAT_ABORTED] =	"Aborted: handshake failure",
    [P3STAT_DROPPED] =	"Receiver dropped message",
    [P3STAT_CHKSUM] =	"Check sum error",
    [P3STAT_NOMCP] =	"MCP not loaded",
    [P3STAT_LEN] =	"Invalid length",
    [P3STAT_HDR] =	"Ptl header too big",
    [P3STAT_BUILDPG] =	"Can't build send page",
    [P3STAT_NAL] =	"Inconsistent NAL",
    [P3STAT_MEMCPYF] =	"Bad memcpy from user space",
    [P3STAT_BADXMIT] =	"Bad xmit",
    [P3STAT_PROC] =	"No destination process",
    [P3STAT_DOOM] =	"Doomed: can't enqueue",
    [P3STAT_SEQ] =	"Packet out of sequence",
    [P3STAT_MEMCPYT] =	"Bad memcpy to user space",
    [P3STAT_PROTO] =	"Protocol error",
    [P3STAT_FINALIZE] =	"lib_finalize failed",
    [P3STAT_DSTNID] =	"Invalid dest nid",
};

const char *p3_debug_statstr[] =   {
    [SND_INITIATED] =	"snd initiated",
    [SND_QUEUED] =	"snd queued",
    [SND_DEQUEUED] =	"snd dequeued",
    [SND_STARTED] =	"snd started",
    [SND_FINISHED] =	"snd finished",
    [RCV_STARTED] =	"rcv started",
    [RCV_FINISHED] =	"rcv finished",
};

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

void
p3_debug_init(void)
{

int i;


    for (i= 0; i < MAX_DEBUG_EVENTS; i++)   {
	debug_event[i].msgID= 0;
    }
    last_debug_event= 0;

}  /* end of p3_debug_init() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

void
p3_debug_add(unsigned int msgID, ptl_hdr_t *hdr, p3_event_type_t event,
	p3_status_t status)
{

    debug_event[last_debug_event].msgID= msgID;
    debug_event[last_debug_event].event= event;
    debug_event[last_debug_event].status= status;
    debug_event[last_debug_event].time= jiffies;
    memcpy(&debug_event[last_debug_event].hdr, hdr, sizeof(ptl_hdr_t));
    last_debug_event= (last_debug_event + 1) % MAX_DEBUG_EVENTS;

}  /* end of p3_debug_add() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

void
p3_debug_proc(char **pb_ptr, char *pb_end)
{

char *pb;
p3_debug_event_t *entry;
int my_current;
ptl_hdr_t *hdr;
int ago;
unsigned long flags;


    pb= *pb_ptr;

    pb += sprintf(pb, "Portals 3 Message Protocol Log\n");
    pb += sprintf(pb, "msg ID    milis ago action        type  src gid/rid  "
	    "( nid/pid ) dst nid/pid  error\n");

    save_flags(flags);
    cli();
    my_current= (last_debug_event + 1) % MAX_DEBUG_EVENTS;
    do   {
	entry= &debug_event[my_current];
	hdr= &(entry->hdr);
	if (entry->msgID == 0)   {
	    my_current= (my_current + 1) % MAX_DEBUG_EVENTS;
	    continue;
	}

	/* How long ago in miliseconds */
	ago= 1000 * (jiffies - entry->time) / HZ;

	if (hdr->type == PTL_MSG_ACK)   {
	    pb += sprintf(pb, "%08x %10d %-13s %s from %4d/%-4d (%4d/%-4d) to "
		    "%4d/%-4d %-s\n", entry->msgID, ago,
		    p3_debug_statstr[entry->event], "Ack", hdr->src.gid,
		    hdr->src.rid, hdr->src.nid, hdr->src.pid, hdr->nid,
		    hdr->pid, p3_debug_errstr[entry->status]);
	    pb += sprintf(pb, "                    manipulated len %d\n",
		    hdr->msg.ack.mlength);
	} else if (hdr->type == PTL_MSG_PUT)   {
	    pb += sprintf(pb, "%08x %10d %-13s %s from %4d/%-4d (%4d/%-4d) to "
		    "%4d/%-4d %-s\n", entry->msgID, ago,
		    p3_debug_statstr[entry->event], "Put", hdr->src.gid,
		    hdr->src.rid, hdr->src.nid, hdr->src.pid, hdr->nid,
		    hdr->pid, p3_debug_errstr[entry->status]);
	    pb += sprintf(pb, "                    ptl %d, match bits 0x%0lx, "
		    "len %d, off %d, hdr data 0x%0lx\n",
		    hdr->msg.put.ptl_index, hdr->msg.put.match_bits,
		    hdr->msg.put.length, hdr->msg.put.offset,
		    hdr->msg.put.hdr_data);
	} else if (hdr->type == PTL_MSG_GET)   {
	    pb += sprintf(pb, "%08x %10d %-13s %s from %4d/%-4d (%4d/%-4d) to "
		    "%4d/%-4d %-s\n", entry->msgID, ago,
		    p3_debug_statstr[entry->event], "Get", hdr->src.gid,
		    hdr->src.rid, hdr->src.nid, hdr->src.pid, hdr->nid,
		    hdr->pid, p3_debug_errstr[entry->status]);
	    pb += sprintf(pb, "                    ptl %d, match bits 0x%lx, "
		    "len %d, src off %d, ret off %d\n",
		    hdr->msg.get.ptl_index, hdr->msg.get.match_bits,
		    hdr->msg.get.length, hdr->msg.get.src_offset,
		    hdr->msg.get.return_offset);
	} else if (hdr->type == PTL_MSG_REPLY)   {
	    pb += sprintf(pb, "%08x %10d %-13s %s from %4d/%-4d (%4d/%-4d) to "
		    "%4d/%-4d %-s\n", entry->msgID, ago,
		    p3_debug_statstr[entry->event], "Rpy", hdr->src.gid,
		    hdr->src.rid, hdr->src.nid, hdr->src.pid, hdr->nid,
		    hdr->pid, p3_debug_errstr[entry->status]);
	    pb += sprintf(pb, "                    dst off %d, len %d\n",
		    hdr->msg.reply.dst_offset, hdr->msg.reply.length);
	} else if (hdr->type == PTL_MSG_BARRIER)   {
	    pb += sprintf(pb, "%08x %10d %-13s %s from %4d/%-4d (%4d/%-4d) to "
		    "%4d/%-4d %-s\n", entry->msgID, ago,
		    p3_debug_statstr[entry->event], "Brr", hdr->src.gid,
		    hdr->src.rid, hdr->src.nid, hdr->src.pid, hdr->nid,
		    hdr->pid, p3_debug_errstr[entry->status]);
	    pb += sprintf(pb, "                    seq %d\n",
		    hdr->msg.barrier.sequence);
	} else   {
	    pb += sprintf(pb, "%08x %10d %-13s %s from %4d/%-4d (%4d/%-4d) to "
		    "%4d/%-4d %-s\n", entry->msgID, ago,
		    p3_debug_statstr[entry->event], "???", hdr->src.gid,
		    hdr->src.rid, hdr->src.nid, hdr->src.pid, hdr->nid,
		    hdr->pid, p3_debug_errstr[entry->status]);
	}

	my_current= (my_current + 1) % MAX_DEBUG_EVENTS;
	/*
	** Make sure we don't overrun the printbuffer. None of our
	** lines should be longer than 512 bytes.
	*/
    } while ((my_current != last_debug_event) && ((pb + 512) < pb_end));
    restore_flags(flags);

    *pb_ptr= pb;

}  /* end of p3_debug_proc() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */
