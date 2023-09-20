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
** $Id: route_util.c,v 1.9 2000/10/23 22:25:25 pumatst Exp $
** Utilities to handle routing data.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defines.h"
#include "arch_asm.h"
#include "../MCP/MCPshmem.h"
#include "route_util.h"


/******************************************************************************/
/*
** read_route()
** Allocate storage and read a routing map from the file pointer provided.
*/
char *
read_route(FILE *route_fp, int verbose)
{

#define MAX_LINE	(1024)
unsigned char *route;
char line[MAX_LINE];
unsigned int byte[MAX_ROUTE_LEN];
unsigned int dummy;
char *start;
int i, rc;
int max_route;
int avg_route;
int num_route;
int valid_route;
int route_len;


    route= (char *)malloc(MAX_ROUTE_LEN * MAX_NUM_ROUTES);
    start= route;
    if (route == NULL)   {
	if (verbose)   {
	    fprintf(stderr, "read_route() Out of memory\n");
	}
	return NULL;
    }
    memset(route, 0, MAX_ROUTE_LEN * MAX_NUM_ROUTES);

    if (verbose)   {
	printf("Reading route file\n");
	fflush(stdout);
    }

    if (MAX_ROUTE_LEN != 32)   {
	fprintf(stderr, "The sscanf() on line %d in file %s\n", __LINE__ + 10,
	    __FILE__);
	fprintf(stderr, "    needs %d conversion specifiers (currently 16)\n",
	    MAX_ROUTE_LEN);
	return NULL;
    }

    num_route= 0;
    avg_route= 0;
    max_route= 0;
    valid_route= 0;
    while (1)   {
        if (fgets(line, MAX_LINE, route_fp) == NULL)   {
            break;
        }
        num_route++;

        rc= sscanf(line, 
	    " 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x "
            "0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x "
            "0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x "
            "0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x "
            "0x%02x \n", 
	    &byte[0], &byte[1], &byte[2], &byte[3], &byte[4], &byte[5], 
	    &byte[6], &byte[7], &byte[8], &byte[9], &byte[10], &byte[11], 
	    &byte[12], &byte[13], &byte[14], &byte[15], &byte[16], &byte[17], 
	    &byte[18], &byte[19], &byte[20], &byte[21], &byte[22], &byte[23], 
	    &byte[24], &byte[25], &byte[26], &byte[27], &byte[28], &byte[29],
            &byte[30], &byte[31], &dummy);
        if (rc == MAX_ROUTE_LEN)   {
	    /* The last byte better be 0 */
	    if (byte[MAX_ROUTE_LEN - 1] != 0)   {
		fprintf(stderr, "Last byte of route %d is not 0x00. Aborting "
		    "load\n", num_route);
		return NULL;
	    }
        } else if (rc > MAX_ROUTE_LEN)   {
	    /*
	    ** dummy will be read, and rc > MAX_ROUTE_LEN, if there
	    ** are too many entries on that line
	    */
	    fprintf(stderr, "Route longer than %d bytes. Aborting load\n",
		MAX_ROUTE_LEN);
	    return NULL;
        }

        route_len= 0;
        for (i= 0; i < rc; i++)   {
	    if (byte[i] == 0)   {
		/* Terminator */
		break;
	    }

	    /*
	    ** byte[i] has to be in the range of 0x80..0x8f or 0xb1..0xbf
	    ** to be valid for a 16 port switch.
	    */
	    if ( ! ((byte[i] >= 0x80) && (byte[i] <= 0x8f)) &&
		 ! ((byte[i] >= 0xb1) && (byte[i] <= 0xbf)))   {
		fprintf(stderr, "Invalid route byte 0x%02x. Aborting load\n",
		    byte[i]);
		return NULL;
	    }

	    route_len++;
	    *route++ = (char)byte[i];
        }

        if (route_len > 0)   {
	    valid_route++;
        }
        if (route_len > max_route)   {
	    max_route= route_len;
        }
        avg_route += route_len;

        route += MAX_ROUTE_LEN - route_len;
    }

    if (num_route > MAX_NUM_ROUTES)   {
	fprintf(stderr, "More than %d routes in route file (read %d routes). "
	    "Aborting load\n", MAX_NUM_ROUTES, num_route);
	return NULL;
    }
    if (valid_route == 0)   {
	fprintf(stderr, "No valid routes found! Aborting load\n");
	return NULL;
    }

    avg_route= avg_route / num_route;
    printf("%d routes read (%d valid). Max %d, avg %f\n", num_route,
	valid_route, max_route, (float)avg_route / num_route);

    return start;

}  /* end of read_route() */

/******************************************************************************/
/*
** dnld_route()
** Put a route map into the MCPs memory
** We treat the map as if it was a set of 4 byte integers.
*/
void
dnld_route(mcpshmem_t *mcpshmem, char *route, int verbose)
{

int i, j;
int value;
int *src_ptr;
int *dst_ptr;


    src_ptr= (int *)route;
    dst_ptr= (int *)mcpshmem->route;

    if (verbose)   {
	printf("Writing route into MCP memory\n");
	fflush(stdout);
    }

    for (i= 0; i < MAX_NUM_ROUTES; i++)   {
	for (j= 0; j < (MAX_ROUTE_LEN / sizeof(int)); j++)   {
	    value= *src_ptr;
	    *dst_ptr= value;
	    wmb();
	    dst_ptr++;
	    src_ptr++;
	}
    }

}  /* end of dnld_route() */

/******************************************************************************/
