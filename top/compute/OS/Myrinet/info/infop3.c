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
** $Id: infop3.c,v 1.5.4.1 2002/03/25 22:45:11 jbogden Exp $
**
** This program collects information about P3 messages (number, length, etc.)
** from nodes listed on the command line, and summarizes it.
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

int verbose;
int clear;
int fd, rc, i;
rtscts_stat_t *rtscts_stat;
unsigned long P3recv;
unsigned long P3recvlen;
unsigned long P3recvmaxlen;
unsigned long P3recvminlen;
unsigned long P3send;
unsigned long P3sendlen;
unsigned long P3sendmaxlen;
unsigned long P3sendminlen;
int good, nnodes;
int *node_list;

int ch;
extern char *optarg;
extern int optind;


    /* Defaults */
    verbose= 0;
    clear= FALSE;

    while ((ch= getopt(argc, argv, "vc")) != EOF)   {
	switch (ch)   {
	    case 'v':
		verbose++;
		break;
	    case 'c':
		clear= TRUE;
		break;
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

    if ((fd= info_init(argv[0], verbose)) < 0)   {
	return -1;
    }

    nnodes= parse_node_list(argv[optind], NULL, 0, 0, MAX_NODES - 1);
    printf("%d nodes on the list\n", nnodes);
    node_list= (int *)malloc(nnodes * sizeof(int));
    if (node_list == NULL)   {
	fprintf(stderr, "%s: Out of memory\n", argv[0]);
	return -1;
    }
    nnodes= parse_node_list(argv[optind], node_list, nnodes, 0, MAX_NODES - 1);

    P3send= 0;
    P3sendlen= 0;
    P3sendmaxlen= 0;
    P3sendminlen= 999999999L;
    P3recv= 0;
    P3recvlen= 0;
    P3recvmaxlen= 0;
    P3recvminlen= 999999999L;
    good= 0;

    for (i= 0; i < nnodes; i++)   {
	/* request info for each node and add it up */
	rc= info_get_data(argv[0], fd, verbose, clear, node_list[i], SEG_RTSCTS,
		(void *)rtscts_stat);
	if (rc < 0)   {
	    continue;
	}
	good++;
	P3send += rtscts_stat->P3send;
	P3sendlen += rtscts_stat->P3sendlen;
	if (rtscts_stat->P3sendmaxlen > P3sendmaxlen)   {
	    P3sendmaxlen= rtscts_stat->P3sendmaxlen;
	}
	if (rtscts_stat->P3sendminlen < P3sendminlen)   {
	    P3sendminlen= rtscts_stat->P3sendminlen;
	}

	P3recv += rtscts_stat->P3recv;
	P3recvlen += rtscts_stat->P3recvlen;
	if (rtscts_stat->P3recvmaxlen > P3recvmaxlen)   {
	    P3recvmaxlen= rtscts_stat->P3recvmaxlen;
	}
	if (rtscts_stat->P3recvminlen < P3recvminlen)   {
	    P3recvminlen= rtscts_stat->P3recvminlen;
	}

	if (verbose)   {
	    printf("%4d: Sends/min/max/tot %12ld", node_list[i],
		rtscts_stat->P3send);
	    if (rtscts_stat->P3sendminlen > rtscts_stat->P3sendmaxlen)   {
		printf("          n/a %15ld %15ld\n", rtscts_stat->P3sendmaxlen,
		    rtscts_stat->P3sendlen);
	    } else   {
		printf(" %11ld %15ld %15ld\n", rtscts_stat->P3sendminlen,
		    rtscts_stat->P3sendmaxlen, rtscts_stat->P3sendlen);
	    }
	    printf("%4d: Recvs/min/max/tot %12ld", node_list[i],
		rtscts_stat->P3recv);
	    if (rtscts_stat->P3recvminlen > rtscts_stat->P3recvmaxlen)   {
		printf("          n/a %15ld %15ld\n", rtscts_stat->P3recvmaxlen,
		    rtscts_stat->P3recvlen);
	    } else   {
		printf(" %11ld %15ld %15ld\n", rtscts_stat->P3recvminlen,
		    rtscts_stat->P3recvmaxlen, rtscts_stat->P3recvlen);
	    }
	}
    }

    printf("P3 stat info for node(s) %s (%d/%d responded)\n", argv[optind],
	good, nnodes);
    printf("   Sends/min/max/tot %15ld", P3send);
    if (P3sendminlen > P3sendmaxlen)   {
	printf("         n/a %15ld %15ld\n", P3sendmaxlen, P3sendlen);
    } else   {
	printf(" %11ld %15ld %15ld\n", P3sendminlen, P3sendmaxlen, P3sendlen);
    }

    printf("   Recvs/min/max/tot %15ld", P3recv);
    if (P3recvminlen > P3recvmaxlen)   {
	printf("         n/a %15ld %15ld\n", P3recvmaxlen, P3recvlen);
    } else   {
	printf(" %11ld %15ld %15ld\n", P3recvminlen, P3recvmaxlen, P3recvlen);
    }

    /* add one final CR to help utils that parse our output */
    printf("\n");
    return 0;
}

/* ~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~ */

void
usage(char *pname)
{
    fprintf(stderr, "Usage: %s [-v] [-v] [-c] node_list\n", pname); 
    fprintf(stderr, "       e.g. %s -v 19,21..23\n", pname); 
    fprintf(stderr, "       -c  clears the counters after reading them\n");
}

/* ~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~ */
