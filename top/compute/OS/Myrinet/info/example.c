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
** $Id: example.c,v 1.2 2001/04/26 20:39:02 rolf Exp $
**
** This is a simple program to show how to retrieve data from another
** node on the Myrinet.
**
** Currently, the following 5 data structures from within the rtscts
** module can be retrieved:
**     SEG_ROUTE_USAGE     route_status
**     SEG_RTSCTS          rtscts_stat
**     SEG_MYRPKT          proc_stat
**     SEG_MCP             MCPshmem info
**     SEG_ERROR           Various errors collected upon request
** The file RTSCTS_info.h has more details.
**
** The function info_get_data() takes one of these segement IDs, a node
** number, and a pointer to a buffer, and gets the desired data (or
** times out).
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include "Pkt_proc.h"		/* For proc_stat_t */
#include "library.h"		/* For info_init(), etc. */


void usage(char *pname);

/* ~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~ */

int
main(int argc, char* argv[])
{

int fd, rc;
proc_stat_t *proc_stat;


    if (argc != 2)   {
	usage(argv[0]);
	exit(-1);
    }


    /* We'll get the proc_stat structure of another node */
    proc_stat= (proc_stat_t *)malloc(sizeof(proc_stat_t));
    if (proc_stat == NULL)   {
	fprintf(stderr, "%s out of memory\n", argv[0]);
	exit(-1);
    }

    /* Specify verbose = 2, to get all error fprintf's */
    if ((fd= info_init(argv[0], 2)) < 0)   {
	return -1;
    }

    /* request the info */
    /* Specify verbose = 2, to get all error fprintf's */
    /* FALSE = don't clear counters */
    rc= info_get_data(argv[0], fd, 2, 0, atoi(argv[1]), SEG_MYRPKT,
	    (void *)proc_stat);
    if (rc < 0)   {
	return -1;
    }

    printf("Got %d bytes! %ld sends, %ld recvs\n", rc, proc_stat->snd_xmit,
	proc_stat->rcv_rtscts);

    return 0;
}

/* ~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~ */

void
usage(char *pname)
{
    fprintf(stderr, "Usage: %s node\n", pname); 
}

/* ~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~ */
