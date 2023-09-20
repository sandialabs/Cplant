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
** $Id: RTSCTS_debug.c,v 1.3.2.3 2002/08/05 18:50:12 jbogden Exp $
** Debugging trace of protocol messages
*/

#include <linux/sched.h>	/* For jiffies, HZ */
#include "RTSCTS_debug.h"

#define MAX_DEBUG_EVENTS	(256)

typedef struct   {
    unsigned int msgID;
    pkt_type_t event;
    unsigned int nid;
    unsigned int info;
    unsigned int info2;
    unsigned long int time;
    int sent;	/* 1 = sent, 0 = recvd */
} protocol_debug_event_t;

static protocol_debug_event_t debug_event[MAX_DEBUG_EVENTS];
static int last_debug_event;


/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

void
protocol_debug_init(void)
{

int i;


    for (i= 0; i < MAX_DEBUG_EVENTS; i++)   {
	debug_event[i].msgID= 0;
    }
    last_debug_event= 0;

}  /* end of protocol_debug_init() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

void
protocol_debug_add_func(unsigned int msgID, pkt_type_t event, unsigned int nid,
    unsigned int info, unsigned int info2, int sent)
{
    if (event != DATA) {
        debug_event[last_debug_event].msgID= msgID;
        debug_event[last_debug_event].event= event;
        debug_event[last_debug_event].nid= nid;
        debug_event[last_debug_event].info= info;
        debug_event[last_debug_event].info2= info2;
        debug_event[last_debug_event].sent= sent;
        debug_event[last_debug_event].time= jiffies;
        last_debug_event= (last_debug_event + 1) % MAX_DEBUG_EVENTS;
    }
}  /* end of protocol_debug_add_func() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */

void
protocol_debug_proc(char **pb_ptr, char *pb_end)
{

char *pb;
protocol_debug_event_t *entry;
int my_current;
int ago;
unsigned long flags;


    pb= *pb_ptr;

    pb += sprintf(pb, "Packet Protocol Log - max events=%d newest event=%d\n",
                  MAX_DEBUG_EVENTS,last_debug_event-1);
    pb += sprintf(pb, "Idx  msg ID    milis ago  type          nid  "
	    "info        info2       direction\n");

    save_flags(flags);
    cli();
    /*my_current= (last_debug_event + 1) % MAX_DEBUG_EVENTS;*/
    my_current = last_debug_event;
    do   {
	entry= &debug_event[my_current];
	if (entry->msgID == 0)   {
	    pb += sprintf(pb, "%3d  empty\n", my_current);
	    my_current= (my_current + 1) % MAX_DEBUG_EVENTS;
	    continue;
	}

	/* How long ago in miliseconds */
	ago= 1000 * (jiffies - entry->time) / HZ;

	if (entry->sent)   {
	    pb += sprintf(pb, "%3d  %08x %10d  %-11s  %4d  0x%08x  0x%08x  "
		    "sent\n", my_current, entry->msgID, ago,
		    protocol_name_str[entry->event], entry->nid, entry->info,
		    entry->info2);
	} else   {
	    pb += sprintf(pb, "%3d  %08x %10d  %-11s  %4d  0x%08x  0x%08x  "
		    "rcvd\n", my_current, entry->msgID, ago,
		    protocol_name_str[entry->event], entry->nid, entry->info,
		    entry->info2);
	}

	my_current= (my_current + 1) % MAX_DEBUG_EVENTS;
	/*
	** Make sure we don't overrun the printbuffer. None of our
	** lines should be longer than 128 bytes.
	*/
    } while ((my_current != last_debug_event) && ((pb + 128) < pb_end));
    restore_flags(flags);

    *pb_ptr= pb;

}  /* end of protocol_debug_proc() */

/* |+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+||+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+|+| */
