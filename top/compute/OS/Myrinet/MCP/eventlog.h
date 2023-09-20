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
** $Id: eventlog.h,v 1.3 2001/08/29 16:22:27 pumatst Exp $
*/

#ifndef EVENTLOG_H
#define EVENTLOG_H

/******************************************************************************/

#define EVENT_MAX	(64)

typedef enum {EVENT_NONE= 0, EVENT_TOOLONG, EVENT_HSTORUN, EVENT_NRES,
    EVENT_MPAR, EVENT_MINT, EVENT_TRUNC, EVENT_LONG, EVENT_CRC, EVENT_LINK2,
    EVENT_WRNGPROT
} mcpevent_t;

typedef struct __eventlog_entry_t   {
    unsigned int	t0;
    unsigned int	t1;
    unsigned int	len;
    mcpevent_t		mcp_event;
    unsigned int	RMPvalue;
    unsigned int	RMLvalue;
    unsigned int	word0;		/* MCP Type and length */
    unsigned int	word1;		/* 0xdeadbeef */
    unsigned int	word2;		/* protocol version */
    unsigned int	word3;		/* src pnid / pkt type */
    unsigned int	word4;		/* msgID */
    unsigned int	word5;		/* len1 / len2 */
    unsigned int	word6;		/* sequence */
    unsigned int	word7;		/* msgNum */
    unsigned int	word8;		/* 1st data word */
    unsigned int	word9;		/* Garbage */
    unsigned int	word10;		/* Garbage */
    unsigned int	word11;		/* Garbage */
    unsigned int	rcvs;		/* Num of receives at this event */
    unsigned int	snds;		/* Num of sends at this event */
    unsigned int	isr;		/* ISR at this event */
} eventlog_entry_t;

void record_event(mcpevent_t event_type, unsigned int *start,
	unsigned int RMPvalue, unsigned int RMLvalue);

/******************************************************************************/

#endif /* EVENTLOG_H */
