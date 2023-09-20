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
** $Id: getevent.c,v 1.1 2001/12/05 18:29:24 rolf Exp $
**
** This program collects MCP event information from nodes listed on
** the command line. Each event is listed on a single line suitbale
** for processing by a script or program.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <load/sys_limits.h>	/* For MAX_NODES */
#include <load/config.h>	/* For parse_node_list() */
#include "RTSCTS_info.h"	/* For eventlog_t */
#include "library.h"		/* For info_init(), etc. */


void usage(char *pname);
void disp_events(eventlog_t *eventlog, int node);

/* ~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~ */

int
main(int argc, char* argv[])
{

int fd, rc, i;
int good, nnodes;
int *node_list;
eventlog_t *eventlog;
unsigned long event_tot;

int ch;
extern char *optarg;
extern int optind;


    while ((ch= getopt(argc, argv, "")) != EOF)   {
	switch (ch)   {
	    default:
		usage(argv[0]);
		exit(-1);
	}
    }

    if (argv[optind] == NULL)   {
	usage(argv[0]);
	exit(-1);
    }

    event_tot= 0;


    /* We'll get the event log of another MCP */
    eventlog= (eventlog_t *)malloc(sizeof(eventlog_t));
    if (eventlog == NULL)   {
	fprintf(stderr, "%s out of memory\n", argv[0]);
	exit(-1);
    }

    if ((fd= info_init(argv[0], 0)) < 0)   {
	return -1;
    }

    nnodes= parse_node_list(argv[optind], NULL, 0, 0, MAX_NODES - 1);
    node_list= (int *)malloc(nnodes * sizeof(int));
    if (node_list == NULL)   {
	fprintf(stderr, "%s: Out of memory\n", argv[0]);
	return -1;
    }
    nnodes= parse_node_list(argv[optind], node_list, nnodes, 0, MAX_NODES - 1);

    good= 0;
    for (i= 0; i < nnodes; i++)   {
	/* request info for each node and add it up */
	rc= info_get_data(argv[0], fd, 0, FALSE, node_list[i], SEG_EVENTS,
		(void *)eventlog);
	if (rc < 0)   {
	    continue;
	}
	good++;

	event_tot += eventlog->event_num;

	disp_events(eventlog, node_list[i]);
    }

    printf("MCP event log for node(s) %s (%d/%d responded)\n", argv[optind],
	good, nnodes);
    printf("Total events counted:  %15ld\n", event_tot);

    return 0;
}

/* ~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~ */

void
usage(char *pname)
{
    fprintf(stderr, "Usage: %s node_list\n", pname); 
    fprintf(stderr, "       e.g. %s 19,21..23\n", pname); 
}

/* ~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~ */

void
disp_events(eventlog_t *eventlog, int node)
{

int i;
unsigned long s1, s2;
long double delta;


    for (i= 0; i < EVENT_MAX; i++)   {
	switch (eventlog->eventlog[i].mcp_event)   {
	    case EVENT_CRC:	printf("Node %4d:%02d/%d CRC:      ",
				    node, i, eventlog->event_num);
				break;
	    case EVENT_HSTORUN:	printf("Node %4d:%02d/%d HSTORUN:  ",
				    node, i, eventlog->event_num);
				break;
	    case EVENT_TRUNC:	printf("Node %4d:%02d/%d TRUNC:    ",
				    node, i, eventlog->event_num);
				break;
	    case EVENT_LONG:	printf("Node %4d:%02d/%d LONG:     ",
				    node, i, eventlog->event_num);
				break;
	    case EVENT_MINT:	printf("Node %4d:%02d/%d MEMINT:   ",
				    node, i, eventlog->event_num);
				break;
	    case EVENT_MPAR:	printf("Node %4d:%02d/%d MEMPAR:   ",
				    node, i, eventlog->event_num);
				break;
	    case EVENT_NRES:	printf("Node %4d:%02d/%d NRESET:   ",
				    node, i, eventlog->event_num);
				break;
	    case EVENT_LINK2:	printf("Node %4d:%02d/%d LINK2:    ",
				    node, i, eventlog->event_num);
				break;
	    case EVENT_WRNGPROT:printf("Node %4d:%02d/%d WRNGPROT: ",
				    node, i, eventlog->event_num);
				break;
	    case EVENT_NONE:	continue;
				break;
	    default:		printf("Node %4d:%02d/%d ???:      ",
				    node, i, eventlog->event_num);
				break;
	}
	s1= eventlog->eventlog[i].t0;
	s2= eventlog->eventlog[i].t1;
	delta= s1 * 0.0000005 + s2 * 0.0000005 * 4294967295.0;

	printf("after %8.6Lfs, %d rcvs, %d snds. ", delta,
	    eventlog->eventlog[i].rcvs, eventlog->eventlog[i].snds);
	printf("RMP 0x%08x,  RML 0x%08x,  len %d ",
	    eventlog->eventlog[i].RMPvalue, eventlog->eventlog[i].RMLvalue,
	    eventlog->eventlog[i].len);
	printf("type/len 0x%08x  xxx 0x%08x  ver 0x%08x  src/ptype 0x%08x ",
	    eventlog->eventlog[i].word0, eventlog->eventlog[i].word1,
	    eventlog->eventlog[i].word2, eventlog->eventlog[i].word3);
	printf("msgID    0x%08x  len 0x%08x  seq 0x%08x  msgNum    0x%08x ",
	    eventlog->eventlog[i].word4, eventlog->eventlog[i].word5,
	    eventlog->eventlog[i].word6, eventlog->eventlog[i].word7);
	printf("1st data 0x%08x  xxx 0x%08x  crc 0x%08x  xxx       0x%08x ",
	    eventlog->eventlog[i].word8, eventlog->eventlog[i].word9,
	    eventlog->eventlog[i].word10, eventlog->eventlog[i].word11);
	printf("ISR 0x%08x\n", eventlog->eventlog[i].isr);
    }
    
}  /* end of disp_events() */

/* ~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~ */
