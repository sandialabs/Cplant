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
** $Id: infoprotocol.c,v 1.4.4.2 2002/05/22 20:01:22 jbogden Exp $
**
** This program collects information about protocol errors and stats
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
int good = 0;
int nnodes = 0;
int *node_list = NULL;
unsigned long protoErr3 = 0;
unsigned long protoErr4 = 0;
unsigned long protoErr5 = 0;
unsigned long protoErr6 = 0;
unsigned long protoErr9 = 0;
unsigned long outstanding_pkts = 0;
unsigned long GCHdoneErr = 0;
unsigned long protoWrongMsgNum = 0;
unsigned long badseq = 0;
unsigned long PKTsend = 0;
unsigned long PKTrecv = 0;
#ifdef DO_TIMEOUT_PROTOCOL
unsigned long protoErr15 = 0;
unsigned long protoErr16 = 0;
unsigned long protoErr17 = 0;
unsigned long protoErr18 = 0;
unsigned long protoErr19 = 0;
unsigned long protoErr21 = 0;
unsigned long protoP3resend = 0;
unsigned long protoP3tout = 0;
#endif /* DO_TIMEOUT_PROTOCOL */
#ifdef EXTENDED_P3_RTSCTS
unsigned long protoP3sync = 0;
unsigned long protoP3resynced = 0;
#endif /* EXTENDED_P3_RTSCTS */

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
    node_list= (int *)malloc(nnodes * sizeof(int));
    if (node_list == NULL)   {
	fprintf(stderr, "%s: Out of memory\n", argv[0]);
	return -1;
    }
    nnodes= parse_node_list(argv[optind], node_list, nnodes, 0, MAX_NODES - 1);

    for (i= 0; i < nnodes; i++)   {
	/* request info for each node and add it up */
	rc= info_get_data(argv[0], fd, verbose, clear, node_list[i], SEG_RTSCTS,
		(void *)rtscts_stat);
	if (rc < 0)   {
	    continue;
	}
    
	good++;
	protoErr3 += rtscts_stat->protoErr3;
	protoErr4 += rtscts_stat->protoErr4;
	protoErr5 += rtscts_stat->protoErr5;
	protoErr6 += rtscts_stat->protoErr6;
	protoErr9 += rtscts_stat->protoErr9;
	outstanding_pkts += rtscts_stat->outstanding_pkts;
	GCHdoneErr += rtscts_stat->protoGCHdoneErr;
	protoWrongMsgNum += rtscts_stat->protoWrongMsgNum;
	badseq += rtscts_stat->badseq;
    PKTsend += rtscts_stat->PKTsend;
    PKTrecv += rtscts_stat->PKTrecv;
    
    #ifdef DO_TIMEOUT_PROTOCOL
	protoErr15 += rtscts_stat->protoErr15;
	protoErr16 += rtscts_stat->protoErr16;
	protoErr17 += rtscts_stat->protoErr17;
	protoErr18 += rtscts_stat->protoErr18;
	protoErr19 += rtscts_stat->protoErr19;
	protoErr21 += rtscts_stat->protoErr21;
	protoP3resend += rtscts_stat->protoP3resend;
	protoP3tout += rtscts_stat->protoP3tout;
    #endif /* DO_TIMEOUT_PROTOCOL */
    
    #ifdef EXTENDED_P3_RTSCTS
    protoP3sync += rtscts_stat->protoP3sync;
    protoP3resynced += rtscts_stat->protoP3resynced;
    #endif /* EXTENDED_P3_RTSCTS */
    
	if (verbose)   {
	    printf("%4d: orphan MSGEND        %15ld    orphan MSGDROP       "
		"%15ld\n", node_list[i], rtscts_stat->protoErr3,
		rtscts_stat->protoErr4);
	    printf("%4d: orphan CTS           %15ld    orphan Data          "
		"%15ld\n", node_list[i], rtscts_stat->protoErr5,
		rtscts_stat->protoErr6);
	    printf("%4d: Last pkt != LAST_DATA%15ld    GCH done unsuccessful"
		"%15ld\n", node_list[i], rtscts_stat->protoErr9,
		rtscts_stat->protoGCHdoneErr);
        printf("%4d: Wrong msg num dropped%15ld    Pkts out of sequence "
        "%15ld\n", node_list[i],rtscts_stat->protoWrongMsgNum,
        rtscts_stat->badseq);
        
        #ifdef DO_TIMEOUT_PROTOCOL
	    printf("%4d: orphan P3_RESEND     %15ld    orphan DATA_RESEND   "
		"%15ld\n", node_list[i], rtscts_stat->protoErr15,
		rtscts_stat->protoErr16);
	    printf("%4d: Unxpctd evnt Qsnd_pnd%15ld    Unxpctd evnt Qsending"
		"%15ld\n", node_list[i], rtscts_stat->protoErr17,
		rtscts_stat->protoErr18);
	    printf("%4d: Ill. LstEvnt Qrcving %15ld    P3 resend failed     "
		"%15ld\n", node_list[i], rtscts_stat->protoErr19,
		rtscts_stat->protoErr21);
	    printf("%4d: P3 resend OK         %15ld    P3 RTS timeout       "
		"%15ld\n", node_list[i], rtscts_stat->protoP3resend,
		rtscts_stat->protoP3tout);
        #endif /* DO_TIMEOUT_PROTOCOL */
        
        #ifdef EXTENDED_P3_RTSCTS
	    printf("%4d: P3 seq sync attempted%15ld    P3 seq sync accepted "
		"%15ld\n", node_list[i], rtscts_stat->protoP3sync,
		rtscts_stat->protoP3resynced);        
        #endif /* EXTENDED_P3_RTSCTS */

	    printf("%4d: RTSCTS packets sent  %15ld    RTSCTS packets recvd "
		"%15ld\n", node_list[i], rtscts_stat->PKTsend,
		rtscts_stat->PKTrecv);
        
	    printf("%4d: Outstanding packets  %15ld\n", node_list[i],
		rtscts_stat->outstanding_pkts);
	}
    }

    printf("Protocol errors and stat for node(s) %s (%d/%d responded)\n",
	argv[optind], good, nnodes);
    printf("    orphan MSGEND        %15ld    orphan MSGDROP       %15ld\n",
	protoErr3, protoErr4);
    printf("    orphan CTS           %15ld    orphan Data          %15ld\n",
	protoErr5, protoErr6);
    printf("    Last pkt != LAST_DATA%15ld    GCH done unsuccessful%15ld\n",
	protoErr9, GCHdoneErr);
    printf("    Wrong msg num dropped%15ld    Pkts out of sequence %15ld\n",
    protoWrongMsgNum,badseq);
    
    #ifdef DO_TIMEOUT_PROTOCOL
    printf("    orphan P3_RESEND     %15ld    orphan DATA_RESEND   %15ld\n",
	protoErr15, protoErr16);
    printf("    Unxpctd evnt Qsnd_pnd%15ld    Unxpctd evnt Qsending%15ld\n",
	protoErr17, protoErr18);
    printf("    Ill. LstEvnt Qrcving %15ld    P3 resend failed     %15ld\n",
	protoErr19, protoErr21);
	printf("    P3 resend OK         %15ld    P3 RTS timeout       %15ld\n",
    protoP3resend,protoP3tout);
    #endif /* DO_TIMEOUT_PROTOCOL */
    
    #ifdef EXTENDED_P3_RTSCTS
	printf("    P3 seq sync attempted%15ld    P3 seq sync accepted %15ld\n",
    protoP3sync,protoP3resynced);        
    #endif /* EXTENDED_P3_RTSCTS */

	printf("    RTSCTS packets sent  %15ld    RTSCTS packets recvd %15ld\n",
    PKTsend, PKTrecv);
    
    printf("    Outstanding packets  %15ld\n", outstanding_pkts);

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
