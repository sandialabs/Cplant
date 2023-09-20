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
** $Id: infomcp.c,v 1.4.4.1 2002/04/02 18:43:20 jbogden Exp $
**
** This program collects information about the MCP
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
mcp_stat_t *mcp_stat;
mcp_stat_t tot;
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
    mcp_stat= (mcp_stat_t *)malloc(sizeof(mcp_stat_t));
    if (mcp_stat == NULL)   {
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
    memset(&tot, 0, sizeof(mcp_stat_t));
    for (i= 0; i < nnodes; i++)   {
	/* request info for each node and add it up */
	rc= info_get_data(argv[0], fd, verbose, clear, node_list[i], SEG_MCP,
		(void *)mcp_stat);
	if (rc < 0)   {
	    continue;
	}
	good++;
	tot.mcp_fres += mcp_stat->mcp_fres;
	tot.mcp_reset += mcp_stat->mcp_reset;
	tot.mcp_hst_dly += mcp_stat->mcp_hst_dly;
	tot.mcp_crc += mcp_stat->mcp_crc;
	tot.mcp_truncated += mcp_stat->mcp_truncated;
	tot.mcp_toolong += mcp_stat->mcp_toolong;
	tot.mcp_send_timeout += mcp_stat->mcp_send_timeout;
	tot.mcp_link2+= mcp_stat->mcp_link2;
	tot.mcp_mem_parity += mcp_stat->mcp_mem_parity;

	if (verbose)   {
	    printf("%4d Network resets      %12ld     "
		"CRC errors           %12ld\n",
		node_list[i], mcp_stat->mcp_fres, mcp_stat->mcp_crc);
	    printf("%4d LANai resets        %12ld     "
		"Host delayed pkt     %12ld\n",
		node_list[i], mcp_stat->mcp_reset, mcp_stat->mcp_hst_dly);
	    printf("%4d Send pkt blocked    %12ld\n",
		node_list[i], mcp_stat->mcp_link2);
	    printf("%4d Send pkt timeouts   %12ld     "
		"Truncated rcv pkts   %12ld\n",
		node_list[i], mcp_stat->mcp_send_timeout, mcp_stat->mcp_truncated);
	    printf("%4d Mem parity errors   %12ld     "
		"Too long rcv pkts    %12ld\n",
		node_list[i], mcp_stat->mcp_mem_parity, mcp_stat->mcp_toolong);
	}
    }

    printf("MCP stat info for node(s) %s (%d/%d responded)\n", argv[optind],
	good, nnodes);
    printf("    Network resets      %12ld     CRC errors           %12ld\n",
	tot.mcp_fres, tot.mcp_crc);
    printf("    LANai resets        %12ld     Host delayed pkt     %12ld\n",
	tot.mcp_reset, tot.mcp_hst_dly);
    printf("    Send pkt blocked    %12ld\n", tot.mcp_link2);
    printf("    Send pkt timeouts   %12ld     Truncated rcv pkts   %12ld\n",
	tot.mcp_send_timeout, tot.mcp_truncated);
    printf("    Mem parity errors   %12ld     Too long rcv pkts    %12ld\n",
	tot.mcp_mem_parity, tot.mcp_toolong);

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
