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
** $Id: route.c,v 1.13 2001/09/21 16:42:45 pumatst Exp $
*/

#include "MCP.h"
#include "MCPshmem.h"
#include "route.h"


start_info_t start_info[MAX_NUM_ROUTES];


/******************************************************************************/
/*
** Initialize the route table by figuring out the length of each route.
*/

void
route_init(void)
{

int dest;
int route_len;
char *start;
int *end;


    memset(mcpshmem->route_copy, 0, sizeof(int) * MAX_NUM_ROUTES *
	    (MAX_ROUTE_LEN + 2 * sizeof(int)) / sizeof(int));

    for (dest= 0; dest < MAX_NUM_ROUTES; dest++)   {
	for (route_len= 0; route_len < MAX_ROUTE_LEN; route_len++)   {
	    if (mcpshmem->route[dest][route_len] == 0)   {
		break;
	    }
	}

	if (route_len == 0)   {
	    /* We'll make zero routes point back at us */
	    route_len= 1;
	    *((unsigned int *)( &(mcpshmem->route[dest][0]))) = 0x80000000;
	}

	/* Where the route bytes start */
	start= (char *)mcpshmem->route_copy[dest] + MAX_ROUTE_LEN - route_len;

	/* Where the header starts */
	end= mcpshmem->route_copy[dest] + (MAX_ROUTE_LEN  / sizeof(int));

	/* Copy the route bytes. Header bytes are generated for each messages */
	memcpy(start, mcpshmem->route[dest], route_len);
	#if defined (L4)
	    start_info[dest].start= (char *)((unsigned int)start & ~0x03);
	    start_info[dest].offset= (unsigned int)start & 0x03;
	#endif /* L4 */
	#if defined (L7) || defined (L9)
	    start_info[dest].start= (char *)((unsigned int)start & ~0x07);
	    start_info[dest].offset= (unsigned int)start & 0x07;
	#endif /* L7 */
	start_info[dest].end= end;
	*(++end)= 0xdeadbeef;
    }

}  /* end of route_init() */

/******************************************************************************/
