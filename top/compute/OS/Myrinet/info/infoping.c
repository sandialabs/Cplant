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
** $Id: infoping.c,v 1.6.2.1 2002/03/25 22:45:11 jbogden Exp $
**
** Don't look at this program, if you want to learn how the rtscts info
** facility works. Use one of the other programs in this directory.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <load/sys_limits.h>	/* For MAX_NODES */
#include <load/config.h>	/* For parse_node_list() */
#include "../util/timing.h"	/* For start() stop() */
#include "RTSCTS_ioctl.h"	/* For RTS_REQ_INFO */
#include "RTSCTS_proc.h"	/* For rtscts_stat_t */
#include "RTSCTS_route.h"	/* For route_used */
#include "Pkt_proc.h"		/* For proc_stat_t */
#include "library.h"		/* For info_init(), etc. */


#define NUM	(1000)

void usage(char *pname);
int do_ping(char *pname, int fd, int verbose, int nnodes, int *node_list);
void do_timing(char *pname, int fd, int node, int cnt);
void do_flood(char *pname, int fd, int node, int cnt);

/* ~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~ */

int
main(int argc, char* argv[])
{

int verbose;
int fd;
int cnt, i;
int timing, flood;
int good, nnodes;
int *node_list;

int ch;
extern char *optarg;
extern int optind;


    /* Defaults */
    verbose= 0;
    cnt= -1;
    timing= FALSE;
    flood= FALSE;

    while ((ch= getopt(argc, argv, "c:ftv")) != EOF)   {
	switch (ch)   {
	    case 'c':
		cnt= strtol(optarg, NULL, 10);
		break;
	    case 'f':
		flood= TRUE;
		break;
	    case 't':
		timing= TRUE;
		break;
	    case 'v':
		verbose++;
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

    if ((cnt < 0) && (timing || flood))   {
	cnt= NUM;
    } else if (cnt < 0)   {
	cnt= 1;
    }

    if ((fd= info_init(argv[0], verbose)) < 0)   {
	return -1;
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
    nnodes= parse_node_list(argv[optind], node_list, nnodes, 0, MAX_NODES - 1);

    if ((timing == FALSE) && (flood == FALSE))   {
	for (i= 0; i < cnt; i++)   {
	    good= do_ping(argv[0], fd, verbose, nnodes, node_list);
	    if (good == nnodes)   {
		printf("%s[%d] %d/%d nodes responded\n", argv[0], i + 1, good,
		    nnodes);
	    } else   {
		printf("%s[%d] %d/%d nodes responded. %d timed out!\n", argv[0],
		    i + 1, good, nnodes, nnodes - good);
	    }
	}
    } else   {
	if (timing)   {
	    do_timing(argv[0], fd, node_list[0], cnt);
	}
	if (flood)   {
	    do_flood(argv[0], fd, node_list[0], cnt);
	}
    }
    return 0;
}

/* ~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~ */

void
usage(char *pname)
{
    fprintf(stderr, "Usage: %s [-v] [-v] [-c cnt] [-t] [-f] node_list\n",
	pname); 
    fprintf(stderr, "       e.g. %s -v 19,21..23\n", pname); 
    fprintf(stderr, "       cnt     reapeat ping \"cnt\" times\n"); 
    fprintf(stderr, "       -t      Do timing (against first node in list)\n"); 
    fprintf(stderr, "       -f      flood first node in list\n"); 
}

/* ~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~ */

int
do_ping(char *pname, int fd, int verbose, int nnodes, int *node_list)
{

int i, rc;
int good;
static seg_error_t *seg_error= NULL;


    if (seg_error == NULL)   {
	seg_error= (seg_error_t *)malloc(sizeof(seg_error_t));
	if (seg_error == NULL)   {
	    fprintf(stderr, "%s out of memory\n", pname);
	    exit(-1);
	}
    }

    /* request SEG_PING from each node, because it is the quickest */
    good= 0;
    for (i= 0; i < nnodes; i++)   {
	rc= info_get_data(pname, fd, verbose, 0, node_list[i], SEG_PING,
		(void *)seg_error);
	if (rc < 0)   {
	    continue;
	}
	good++;
    }

    return good;

} /* do_ping() */

/* ~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~ */

void
do_timing(char *pname, int fd, int node, int cnt)
{

int i, rc;
static int first_time= TRUE;
static seg_error_t *seg_error;
static rtscts_stat_t *rtscts_stat;
static proc_stat_t *proc_stat;
static unsigned long *route_used;
static mcp_stat_t *mcp_stat;
double dtime;


    if (first_time)   {
	printf("    sizeof(seg_error_t) is %ld bytes\n", sizeof(seg_error_t));
	printf("    sizeof(rtscts_stat_t) is %ld bytes\n", sizeof(rtscts_stat_t));
	printf("    sizeof(proc_stat_t) is %ld bytes\n", sizeof(proc_stat_t));
	printf("    sizeof(route_used) is %ld bytes\n", sizeof(unsigned long) * MAX_NUM_ROUTES);
	printf("    sizeof(mcp_stat_t) is %ld bytes\n", sizeof(mcp_stat_t));

	route_used= (unsigned long *)malloc(sizeof(unsigned long) * MAX_NUM_ROUTES);
	if (route_used == NULL)   {
	    fprintf(stderr, "%s out of memory\n", pname);
	    exit(-1);
	}
	seg_error= (seg_error_t *)route_used;
	proc_stat= (proc_stat_t *)route_used;
	mcp_stat= (mcp_stat_t *)route_used;
	rtscts_stat= (rtscts_stat_t *)route_used;
	first_time= FALSE;
    }

    /* request SEG_PING cnt times and measure how long it takes */
    start();
    for (i= 0; i < cnt; i++)   {
	rc= info_get_data(pname, fd, 0, 0, node, SEG_PING, NULL);
	if (rc < 0)   {
	    break;
	}
    }
    dtime= stop();

    if (i != cnt)   {
	fprintf(stderr, "%s: SEG_PING request timed out. Giving up\n", pname);
    } else   {
	printf("%d SEG_PING requests took %8.6f ms (%8.6f us/request)\n",
	    cnt, dtime*1000, dtime*1000000 / cnt);
    }

    /* request seg_error cnt times and measure how long it takes */
    start();
    for (i= 0; i < cnt; i++)   {
	rc= info_get_data(pname, fd, 0, 0, node, SEG_ERROR, (void *)seg_error);
	if (rc < 0)   {
	    break;
	}
    }
    dtime= stop();

    if (i != cnt)   {
	fprintf(stderr, "%s: SEG_ERROR request timed out. Giving up\n", pname);
    } else   {
	printf("%d SEG_ERROR requests took %8.6f ms (%8.6f us/request)\n",
	    cnt, dtime*1000, dtime*1000000 / cnt);
    }

    /* request rtscts_stat cnt times and measure how long it takes */
    start();
    for (i= 0; i < cnt; i++)   {
	rc= info_get_data(pname, fd, 0, 0, node, SEG_RTSCTS, (void *)rtscts_stat);
	if (rc < 0)   {
	    break;
	}
    }
    dtime= stop();

    if (i != cnt)   {
	fprintf(stderr, "%s: SEG_RTSCTS request timed out. Giving up\n", pname);
    } else   {
	printf("%d SEG_RTSCTS requests took %8.6f ms (%8.6f us/request)\n",
	    cnt, dtime*1000, dtime*1000000 / cnt);
    }

    /* request proc_stat cnt times and measure how long it takes */
    start();
    for (i= 0; i < cnt; i++)   {
	rc= info_get_data(pname, fd, 0, 0, node, SEG_MYRPKT, (void *)proc_stat);
	if (rc < 0)   {
	    break;
	}
    }
    dtime= stop();

    if (i != cnt)   {
	fprintf(stderr, "%s: SEG_MYRPKT request timed out. Giving up\n", pname);
    } else   {
	printf("%d SEG_MYRPKT requests took %8.6f ms (%8.6f us/request)\n",
	    cnt, dtime*1000, dtime*1000000 / cnt);
    }

    /* request mcp_stat cnt times and measure how long it takes */
    start();
    for (i= 0; i < cnt; i++)   {
	rc= info_get_data(pname, fd, 0, 0, node, SEG_MCP, (void *)mcp_stat);
	if (rc < 0)   {
	    break;
	}
    }
    dtime= stop();

    if (i != cnt)   {
	fprintf(stderr, "%s: SEG_MCP request timed out. Giving up\n", pname);
    } else   {
	printf("%d SEG_MCP requests took %8.6f ms (%8.6f us/request)\n",
	    cnt, dtime*1000, dtime*1000000 / cnt);
    }

    /* request route_used cnt times and measure how long it takes */
    start();
    for (i= 0; i < cnt; i++)   {
	rc= info_get_data(pname, fd, 0, 0, node, SEG_ROUTE_USAGE,
		(void *)route_used);
	if (rc < 0)   {
	    break;
	}
    }
    dtime= stop();

    if (i != cnt)   {
	fprintf(stderr, "%s: SEG_ROUTE_USAGE request timed out. Giving up\n", pname);
    } else   {
	printf("%d SEG_ROUTE_USAGE requests took %8.6f ms (%8.6f us/request)\n",
	    cnt, dtime*1000, dtime*1000000 / cnt);
    }

    return;

}  /* end of do_timing() */

/* ~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~ */

void
do_flood(char *pname, int fd, int node, int cnt)
{

int i, rc;
static int first_time= TRUE;
static seg_error_t *seg_error;
get_info_req_t get_info;
double dtime;


    if (first_time)   {
	seg_error= (seg_error_t *)malloc(sizeof(seg_error_t));
	if (seg_error == NULL)   {
	    fprintf(stderr, "%s out of memory\n", pname);
	    exit(-1);
	}
	first_time= FALSE;
    }

    /* send out a seg_error request "cnt" times and see how long it takes */
    get_info.node= node;
    get_info.seg= SEG_PING;
    get_info.buf= seg_error;

    start();
    for (i= 0; i < cnt; i++)   {
	rc= ioctl(fd, RTS_REQ_INFO, &get_info);
	if (rc < 0)   {
	    break;
	}
    }
    dtime= stop();

    if (i != cnt)   {
	fprintf(stderr, "%s: SEG_PING request failed. Giving up\n", pname);
    } else   {
	printf("Sending %d SEG_PING requests took %8.6f ms (%8.6f us/"
	    "request)\n", cnt, dtime*1000, dtime*1000000 / cnt);
    }

    return;

}  /* end of do_flood() */

/* ~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~\__/~~~ */
