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
** $Id: getp3.c,v 1.1 2001/12/14 18:21:16 rolf Exp $
**
** This program collects P3 statistics information from nodes listed on
** the command line. Each node's statistics is listed on a single line suitbale
** for processing by a script or program.
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
void disp_p3(rtscts_stat_t *rtscts_stat, int node);

/* ~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~ */

int
main(int argc, char* argv[])
{

int fd, rc, i;
int good, nnodes;
int *node_list;
rtscts_stat_t *rtscts_stat;
unsigned long P3recv;
unsigned long P3recvlen;
unsigned long P3send;
unsigned long P3sendlen;

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

    P3send= 0;
    P3sendlen= 0;
    P3recv= 0;
    P3recvlen= 0;
    good= 0;
    for (i= 0; i < nnodes; i++)   {
	/* request info for each node and add it up */
	rc= info_get_data(argv[0], fd, 0, FALSE, node_list[i], SEG_RTSCTS,
		(void *)rtscts_stat);
	if (rc < 0)   {
	    continue;
	}
	good++;
	P3send += rtscts_stat->P3send;
	P3sendlen += rtscts_stat->P3sendlen;
	P3recv += rtscts_stat->P3recv;
	P3recvlen += rtscts_stat->P3recvlen;

	disp_p3(rtscts_stat, node_list[i]);
    }

    printf("P3 stat info for node(s) %s (%d/%d responded)\n", argv[optind],
	good, nnodes);
    printf("   Sends %15ld  (%15ld Bytes)\n", P3send, P3sendlen);
    printf("   Recvs %15ld  (%15ld Bytes)\n", P3recv, P3recvlen);

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
disp_p3(rtscts_stat_t *rtscts_stat, int node)
{
    printf("%d: Sends %ld, len %ld.   Recvs %ld, len %ld\n", node,
	rtscts_stat->P3send, rtscts_stat->P3sendlen, rtscts_stat->P3recv,
	rtscts_stat->P3recvlen);
}  /* end of disp_p3() */

/* ~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~ */
