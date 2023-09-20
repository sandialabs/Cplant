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
** $Id: infoerr.c,v 1.6.4.1 2002/05/22 20:01:22 jbogden Exp $
**
** This program collects error information
** from nodes listed on the command line, and summarizes it.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>		/* For memset() */
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <load/sys_limits.h>	/* For MAX_NODES */
#include <load/config.h>	/* For parse_node_list() */
#include "library.h"		/* For info_init(), etc. */


void usage(char *pname);

/* ~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~ */

int
main(int argc, char* argv[])
{

int verbose;
int clear;
int fd, rc, i;
seg_error_t *seg_error;
seg_error_t tot;
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


    /* We'll get the mcp_stat_t structure of another node */
    seg_error= (seg_error_t *)malloc(sizeof(seg_error_t));
    if (seg_error == NULL)   {
	fprintf(stderr, "%s out of memory\n", argv[0]);
	exit(-1);
    }

    if ((fd= info_init(argv[0], verbose)) < 0)   {
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
    memset(&tot, 0, sizeof(seg_error_t));
    for (i= 0; i < nnodes; i++)   {
	/* request info for each node and add it up */
	rc= info_get_data(argv[0], fd, verbose, clear, node_list[i], SEG_ERROR,
		(void *)seg_error);
	if (rc < 0)   {
	    continue;
	}
	good++;
	tot.module_send_errs += seg_error->module_send_errs;
	tot.module_recv_errs += seg_error->module_recv_errs;
	tot.module_badcpy += seg_error->module_badcpy;
	tot.module_sendErr += seg_error->module_sendErr;
    tot.module_recvErr += seg_error->module_recvErr;
	tot.module_protErr += seg_error->module_protErr;
	tot.module_outstanding_pkts += seg_error->module_outstanding_pkts;
	tot.module_neterrs += seg_error->module_neterrs;

	if (verbose)   {
	    printf("%4d Packet send errors  %12ld     Pkt receive errors   "
		"%12ld\n", node_list[i],
		seg_error->module_send_errs, seg_error->module_recv_errs);
	    printf("%4d Module send errors  %12ld     Failed user memcpy   "
		"%12ld\n", node_list[i],
		seg_error->module_sendErr, seg_error->module_badcpy);
	    printf("%4d Protocol errors     %12ld     Outstanding packets  "
		"%12ld\n", node_list[i],
		seg_error->module_protErr, seg_error->module_outstanding_pkts);
	    printf("%4d Network errors      %12ld     Module recv errors   "
        "%12ld\n", node_list[i],
		seg_error->module_neterrs,seg_error->module_recvErr);
	}
    }

    printf("Message errors for node(s) %s (%d/%d responded)\n", argv[optind],
	good, nnodes);
    printf("    Packet send errors  %12ld     Pkt receive errors   %12ld\n",
	tot.module_send_errs, tot.module_recv_errs);
    printf("    Module send errors  %12ld     Failed user memcpy   %12ld\n",
	tot.module_sendErr, tot.module_badcpy);
    printf("    Protocol errors     %12ld     Outstanding packets  %12ld\n",
	tot.module_protErr, tot.module_outstanding_pkts);
    printf("    Network errors      %12ld     Module recv errors   %12ld\n",
    tot.module_neterrs,tot.module_recvErr);

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
