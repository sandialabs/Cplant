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
** $Id: route.h,v 1.16 2001/08/29 16:22:27 pumatst Exp $
** This file contains the functions that deal with
** routing through the Myrinet.
*/

#ifndef ROUTE_H
#define ROUTE_H

#include "lanai_def.h"


/*
    Routes are stored in the route table with the route byte that has to go
    out first in the left-most position of the array. To send it out, using
    SML the routes need to be word aligned to the right in the array.

    So, during initialization, we copy all routes into a new array that
    is aligned properly and has room for the length and type field. That
    way, when we want to send, we write the length and packet type and
    send!

*/

typedef struct   {
    char *start;
    int *end;
    int offset;
} start_info_t;

extern start_info_t start_info[MAX_NUM_ROUTES];

extern void route_init(void);
extern __inline__ void snd_route(int dst);

int local_route[(MAX_TEST_ROUTE_LEN + 2 * sizeof(int)) / sizeof(int)]
            __attribute__ ((aligned (8)));

/******************************************************************************/
/*
** Start a message. We assume the network is send ready.
** The route is already setup and aligned at the end on an 8 byte boundary.
** All we have to do is add a word at the end of the route with the length
** and packet type information. Then we can send the whole string of bytes.
**
**                 8-byte align    start    end
**                 |               |        |
**                 |<---offset---->|        |
**                 V               V        V
** +--------   -----------------------------+--------+--------+
** |              Route bytes right aligned | hdr    | unused |
** +--------   -----------------------------+--------+--------+
** ^                                        ^                 ^
** |                                        |                 |
** 8-byte aligned                           8-byte aligned    8-byte aligned
**
*/
extern __inline__ void
start_msg(int dest, int cur_len)
{

unsigned int hdr;
char *start, *start0;
int *end;
int offset;
int troute_len;


    hdr= (MYRI_PORTAL_PACKET_TYPE << 16) | cur_len;

    if (dest != TEST_ROUTE_ID)   {
	/* Now send it */
	*(start_info[dest].end)= hdr;
	SMP= start_info[dest].start;
	#if defined (USE_32BIT_CRC)
	    SMH= (unsigned int *)(((unsigned int)(start_info[dest].start) & ~ALIGN_64b) + 8);
	#endif
	SA= start_info[dest].offset;
	GM_STBAR();
	#if defined (L4)
	    /* need to send 8 byte header to be compatible with L7 */
	    SML= start_info[dest].end + 1;
	#endif /* L4 */
	#if defined (L7) || defined (L9)
	    SML= start_info[dest].end;
	#endif /* L7 */
	ISR_BARRIER(SML);
    } else   {
	/* Use the test route instead */
	troute_len = mcpshmem->test_route_len;
	memset(local_route, 0, (MAX_TEST_ROUTE_LEN + 2 * sizeof(int)));

	start0= (char *)local_route + MAX_TEST_ROUTE_LEN - troute_len;
	memcpy(start0, mcpshmem->test_route, troute_len);
	end= local_route + (MAX_TEST_ROUTE_LEN  / sizeof(int));
       *end = hdr;
       *(end+1) = 0xdeadbeef;

	#if defined (L4)
	    start= (char *)((unsigned int)start0 & ~0x03);
	    offset= (unsigned int)start0 & 0x03;
	#endif /* L4 */
	#if defined (L7) || defined (L9)
	    start= (char *)((unsigned int)start0 & ~0x07);
	    offset= (unsigned int)start0 & 0x07;
	#endif /* L7 */

	SMP= start;
	SA= offset;
	GM_STBAR();
	#if defined (L4)
	    /* need to send 8 byte header to be compatible with L7 */
	    SML= end + 1;
	#endif /* L4 */
	#if defined (L7) || defined (L9)
	    SML= end;
	#endif /* L7 */
	ISR_BARRIER(SML);
    }

}  /* end of start_msg() */

/******************************************************************************/

#endif /* ROUTE_H */
