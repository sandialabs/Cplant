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
** $Id: eventlog.c,v 1.4 2001/10/31 18:14:42 rolf Exp $
*/

#include "MCPshmem.h"
#include "timer.h"
#include "eventlog.h"


/******************************************************************************/

void
record_event(mcpevent_t event_type, unsigned int *start, unsigned int RMPvalue,
    unsigned int RMLvalue)
{

int event_slot;
int len;
world_time_t world_time;


    mcpshmem->event_num++;
    event_slot= mcpshmem->eventlog_next;
    getWorldTime( &world_time);
    mcpshmem->eventlog[event_slot].t0= world_time.t0;
    mcpshmem->eventlog[event_slot].t1= world_time.t1;
    if (start != 0)   {
	#if defined(L4)
	    /* RMP is 4 over on LANai 4.x */
	    len= RMPvalue - (unsigned int)start - 4;
	#endif /* L4 */
	#if defined(L7) || defined (L9)
	    /* RMP is 8 over on LANai 7.x, 9.x */
	    len= RMPvalue - (unsigned int)start - 8;
	#endif /* L7 */

	mcpshmem->eventlog[event_slot].len= len;
	len= len / sizeof(int);
	mcpshmem->eventlog[event_slot].word0= *start;
	mcpshmem->eventlog[event_slot].word1= *(start + 1);
	mcpshmem->eventlog[event_slot].word2= *(start + 2);
	mcpshmem->eventlog[event_slot].word3= *(start + 3);
	mcpshmem->eventlog[event_slot].word4= *(start + 4);
	mcpshmem->eventlog[event_slot].word5= *(start + 5);
	mcpshmem->eventlog[event_slot].word6= *(start + 9);
	mcpshmem->eventlog[event_slot].word7= *(start + 12);
	mcpshmem->eventlog[event_slot].word8= *(start + 15);
	mcpshmem->eventlog[event_slot].word9=  *(start + len - 1);
	mcpshmem->eventlog[event_slot].word10= *(start + len);
	mcpshmem->eventlog[event_slot].word11= *(start + len + 1);
    } else   {
	mcpshmem->eventlog[event_slot].len= 0;
	mcpshmem->eventlog[event_slot].word0= 0;
	mcpshmem->eventlog[event_slot].word1= 0;
	mcpshmem->eventlog[event_slot].word2= 0;
	mcpshmem->eventlog[event_slot].word3= 0;
	mcpshmem->eventlog[event_slot].word4= 0;
	mcpshmem->eventlog[event_slot].word5= 0;
	mcpshmem->eventlog[event_slot].word6= 0;
	mcpshmem->eventlog[event_slot].word7= 0;
	mcpshmem->eventlog[event_slot].word8= 0;
	mcpshmem->eventlog[event_slot].word9= 0;
	mcpshmem->eventlog[event_slot].word10= 0;
	mcpshmem->eventlog[event_slot].word11= 0;
    }
    mcpshmem->eventlog[event_slot].mcp_event= event_type;
    mcpshmem->eventlog[event_slot].RMPvalue= RMPvalue;
    mcpshmem->eventlog[event_slot].RMLvalue= RMLvalue;
    mcpshmem->eventlog[event_slot].rcvs= mcpshmem->counters.rcvs;
    mcpshmem->eventlog[event_slot].snds= mcpshmem->counters.sends;
    mcpshmem->eventlog[event_slot].isr= get_ISR();

    mcpshmem->eventlog_next++;
    if (mcpshmem->eventlog_next >= EVENT_MAX)   {
	mcpshmem->eventlog_next= 0;
    }
}  /* end of record_event() */

/******************************************************************************/
