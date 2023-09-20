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
** $Id: getpackets.c,v 1.1.4.1 2002/03/25 22:45:11 jbogden Exp $
** For each node on the command line, get the number of packets the
** MCP has sent and received, as well as the number of bytes sent and
** received. The output is suitable for parsing by a script or program.
**
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <load/sys_limits.h>	/* For MAX_NODES */
#include <load/config.h>	/* For parse_node_list() */
#include "RTSCTS_proc.h"	/* For rtscts_stat_t */
#include "library.h"		/* For info_init(), etc. */


void usage(char *pname);

/* ~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~ */

int
main(int argc, char* argv[])
{

int fd, rc, i;
rtscts_stat_t *rtscts_stat;
unsigned long Pktrecv;
unsigned long Pktrecvlen;
unsigned long Pktsend;
unsigned long Pktsendlen;
int good, nnodes;
int *node_list;

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


    /* We'll get the rtscts_stat_t structure of another node */
    rtscts_stat= (rtscts_stat_t *)malloc(sizeof(rtscts_stat_t));
    if (rtscts_stat == NULL)   {
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

    Pktsend= 0;
    Pktsendlen= 0;
    Pktrecv= 0;
    Pktrecvlen= 0;
    good= 0;

    for (i= 0; i < nnodes; i++)   {
	/* request info for each node and add it up */
	rc= info_get_data(argv[0], fd, 0, 0, node_list[i], SEG_RTSCTS,
		(void *)rtscts_stat);
	if (rc < 0)   {
	    continue;
	}
	good++;
	Pktsend += rtscts_stat->PKTsend;
	Pktsendlen += rtscts_stat->PKTsendlen;
	Pktrecv += rtscts_stat->PKTrecv;
	Pktrecvlen += rtscts_stat->PKTrecvlen;

	printf("%4d: Sends %ld, len %ld.   Recvs %ld, len %ld\n",
	    node_list[i], rtscts_stat->PKTsend, rtscts_stat->PKTsendlen,
	    rtscts_stat->PKTrecv, rtscts_stat->PKTrecvlen);
    }

    printf("Packet stat info for node(s) %s (%d/%d responded)\n", argv[optind],
	good, nnodes);
    printf("   Sends %15ld, total len %15ld\n", Pktsend, Pktsendlen);
    printf("   Recvs %15ld, total len %15ld\n", Pktrecv, Pktrecvlen);

    /* add one final CR to help utils that parse our output */
    printf("\n");
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
