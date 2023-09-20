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
 *  $Id: p3_ping.c,v 1.1.2.2 2002/03/25 22:41:51 jbogden Exp $
 *   
 *  p3_ping.c
 *
 *  Utility to generate to do P3 level rtscts pings.  P3 level pings can
 *  test the connection between nodes with respect to the P3 message
 *  sequence numbers.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <cTask/cTask.h>
#include <load/sys_limits.h>    /* For MAX_NODES */
#include <load/config.h>        /* For parse_node_list() */
#include "RTSCTS_ioctl.h"
#include "timing.h"             /* for start() and stop() */


void
usage()
{
    fprintf(stderr,"Usage: p3_ping [-v] [-c cnt] [-t] node_list\n");
    fprintf(stderr,"       -v        verbose mode\n"); 
    fprintf(stderr,"       -c <cnt>  number of iterations\n");
    fprintf(stderr,"       -t        do latency timing\n");
    fprintf(stderr,"       EX: p3_ping -v 45,65..100,120\n");                
}

/* some timeout values */
#define TIMEOUT		     2   /* in seconds */
#define MAX_TIMEOUTS     5   /* number of timed out packets */


int main(int argc, char *argv[])
{
    int verbose = 0;
    int cnt = 1;
    int timing = 0;
    int nnodes;
    int *node_list;
    int fdr;
    int rc;
    unsigned int curr_nid;
    int i;
    double delta = 0.0;
    double total = 0.0;
    int acked = 0;
    int nodes_acked = 0;
    
    /* getopt stuff*/
    int ch;
    extern char *optarg;
    extern int optind;


    while ((ch= getopt(argc, argv, "vc:t")) != EOF)   {
        switch (ch) {
            case 'v':
                verbose = 1;
                break;
            case 'c':
                cnt = atoi(optarg);
                break;
            case 't':
                timing = 1;
                cnt = 1000;
                break;
            default:
                usage();
                exit(-1);
        }
    }

    if (argv[optind] == NULL)   {
	    usage();
	    exit(-1);
    }
    
    /* Find out how many nodes were on the command line and allocate an array */
    nnodes= parse_node_list(argv[optind], NULL, 0, 0, MAX_NODES - 1);
    if (nnodes <= 0)   {
	    fprintf(stderr, "%s: No nodes specified\n", argv[0]);
	    return -1;
    }
    node_list= (int *)malloc(nnodes * sizeof(int));
    if (node_list == NULL)   {
	    fprintf(stderr, "%s: Out of memory\n", argv[0]);
	    return -1;
    }

    /* Extract the nodes and put them into an array */
    nnodes = parse_node_list(argv[optind], node_list, nnodes, 0, MAX_NODES - 1);

    /* open rtscts device */
    if ( (fdr = open("/dev/rtscts", O_RDWR)) < 0) {
        fprintf(stderr,"p3_ping: failed to open /dev/rtscts...\n");
        exit(-1);
    }

    /* send ping packet(s) to each node specified */
    for (curr_nid = 0; curr_nid < nnodes; curr_nid++) {
        total = 0.0;
        acked = 0;
        for (i = 0; i < cnt; i++) {
            /* try to send the packet, start the timer */
            start();
            rc = ioctl(fdr,RTS_REQ_P3_PING,node_list[curr_nid]);
            if (rc != 0) {
                fprintf(stderr,"%s: Couldn't send ping # %d to nid %d...\n",
                        argv[0],i+1,node_list[curr_nid]);
                /* exit the iteration loop and go to next node */
                i = cnt;
                break;
            }
            
            /* poll for a response to the ping */
            do {
                rc = ioctl(fdr,RTS_GET_P3_PING,node_list[curr_nid]);
                if ( (delta = stop()) > 1.0) {
                    if (verbose) {
                        fprintf(stderr,"%s: Ping # %d to nid %d timed out!!\n",
                                argv[0],i+1,node_list[curr_nid]);
                    }
                    i = cnt;
                    break;
                }
            } while (rc == -1);
            
            if (rc == 0) {
                /* the ping was successful */
                total += delta;
                acked++;
            }
            else if (rc != -1) {
                fprintf(stderr,"%s: Weird error %i from ioctl()...\n",
                        argv[0],rc);
                return -1;
            }
        }        
        
        if (timing  &&  acked == cnt) {
            /* We are done pinging for this node. Calculate the stats. */
            printf("%4d: %d P3 ping(s) took %8.6f ms (%8.6f us latency)\n",
                   node_list[curr_nid],cnt,total*1000,total*1000000/cnt);
        }
        else if (timing) {
            printf("%4d: timed out!\n",node_list[curr_nid]);
        }
        
        /* if the node acked all the pings we sent, then call this node
         * responsive. otherwise, we'll call him timed out. so basically,
         * if a node fails 1% of the pings it is still failed!
        */
        if (acked == cnt)
            nodes_acked++;
    }

    if (!timing)
        printf("%s: %d/%d nodes responded to the P3 ping\n",\
               argv[0],nodes_acked,nnodes);
    
    return 0;
}

