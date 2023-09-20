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
** $Id: genroutesiberia.c,v 1.2 2001/03/07 01:40:57 rolf Exp $
** Generate a Myricom style route file with routes for Siberia.
** That route file can be piped through myr2sandia and converted 
** to individual routes files that our MCP can understand.
**
** Siberia (at this time) is a 2-D 8x10 mesh with wraparound.
** There are always two links in each direction. Odd numbered nodes
** will use left and upper links, while even numbered nodes will
** use right and lower links.
**
** This should produce more efficient routes than the general purpose
** algorithm in simple_routes.
*/

#include <stdio.h>


/*--> <-- --> <-- --> <-- --> <-- --> <--> <-- --> <-- --> <-- --> <-- --> <--*/
/*
** Constants
*/
#define MAX_COL		(8)	/* 8 columns */
#define MAX_ROW		(10)	/* 10 rows */
#define NODES_PER_SW	(8)	/* 8 nodes per switch */
#define MAX_NODES	(MAX_COL * MAX_ROW * NODES_PER_SW)
#define MAX_ROUTE_LEN	(32)	/* Max length of any route */
#define MAX_LABEL_LEN	(256)	/* Max length of a host name */
#define END_MARKER	(1024)	/* Invalid route bytes to mark the end */

/*--> <-- --> <-- --> <-- --> <-- --> <--> <-- --> <-- --> <-- --> <-- --> <--*/
/*
** Globals
*/
unsigned long max_route_len;
unsigned long tot_route_len;
unsigned long num_route;

/*--> <-- --> <-- --> <-- --> <-- --> <--> <-- --> <-- --> <-- --> <-- --> <--*/
/*
** Local functions
*/
FILE * create_file(char *fname, int num_nodes, int verbose);
void gen_section(FILE *fp, int node, int wrap, int verbose);
void gen_route(int src, int dst, int *route, int wrap, int verbose);
int sw_x(int sw);
int sw_y(int sw);
int sw_neighbor(int sw, int x_dir, int y_dir);
int sw_num(int x, int y);
void usage(char *pname);


/*--> <-- --> <-- --> <-- --> <-- --> <--> <-- --> <-- --> <-- --> <-- --> <--*/
int
main(int argc, char *argv[])
{

FILE *fp;
int i;
int wrap;
int verbose;

int ch;
extern char *optarg;
extern int optind;



    /* Defaults */
    wrap= 1;
    verbose= 0;

    while ((ch= getopt(argc, argv, "vw")) != EOF)   {
	switch (ch)   {
	    case 'v':
		verbose++;
		break;
	    case 'w':
		wrap= 0;
		break;
	    default:
		usage(argv[0]);
		exit(-1);
	}
    }


    fp= create_file(argv[optind], MAX_NODES, verbose);

    for (i= 0; i < MAX_NODES; i++)   {
	gen_section(fp, i, wrap, verbose);
    }

    if (wrap)   {
	fprintf(stderr, "    Wrap arounds are being used\n");
    } else   {
	fprintf(stderr, "    Wrap arounds are not used\n");
    }
    fprintf(stderr, "    Number of routes generated: %ld\n", num_route);
    fprintf(stderr, "    Average route length:       %.2f\n",
	(float)tot_route_len / num_route);
    fprintf(stderr, "    Maximum route length:       %ld\n", max_route_len);

}  /* end of main() */

/*--> <-- --> <-- --> <-- --> <-- --> <--> <-- --> <-- --> <-- --> <-- --> <--*/
/*
** Create the output file and write the header (or use stdout if
** fname is empty.
*/
FILE *
create_file(char *fname, int num_nodes, int verbose)
{

FILE *fp;


    if ((fname == NULL) || (fname[0] == '\0'))   {
	fp= stdout;
	if (verbose) fprintf(stderr, "    Output goes to stdout\n");
    } else   {
	fp= fopen(fname, "w");
	if (fp == NULL)   {
	    fprintf(stderr, "Could not open output file \"%s\"\n", fname);
	    perror("");
	    exit(-1);
	}
	if (verbose) fprintf(stderr, "    Output file is \"%s\"\n", fname);
    }
    fprintf(fp, "%d 1\n", num_nodes);
    return fp;

}  /* emd of create_file() */

/*--> <-- --> <-- --> <-- --> <-- --> <--> <-- --> <-- --> <-- --> <-- --> <--*/
/*
** Create the routes for this node and write them to file fp
*/
void
gen_section(FILE *fp, int node, int wrap, int verbose)
{

int i, j;
int port, sw;
int my_port, my_sw;
char label[MAX_LABEL_LEN];
int route[MAX_ROUTE_LEN];


    /*
    ** Header for this node
    ** The second number in parentheses is supposed to be the sum of
    ** all route lengths in this section. We don't know yet how long
    ** the routes will be, so we set an upper limit. myr2sandia wont
    ** look at that number anyway.
    */
    my_port= node % MAX_COL;
    my_sw= node / MAX_COL;
    gen_host_name_siberia(label, my_sw, my_port);
    fprintf(fp, "\"%s\" (%d %d)\n", label, MAX_NODES,
	MAX_ROUTE_LEN * MAX_NODES);
    if (verbose > 1)   {
	fprintf(stderr, "    Generating routes for node %d (\"%s\") on port %d "
	    "on switch %d\n", node, label, my_port, my_sw);
    }


    /* Routes to each other node */
    for (i= 0; i < MAX_NODES; i++)   {
	port= i % MAX_COL;
	sw= i / MAX_COL;
	gen_host_name_siberia(label, sw, port);
	gen_route(node, i, route, wrap, verbose);
	fprintf(fp, "\"%s\" ", label);
	j= 0;
	while (route[j + 1] != END_MARKER)   {
	    fprintf(fp, "%d,", route[j]);
	    j++;
	}
	fprintf(fp, "%d\n", route[j]);
    }

}  /* end of gen_section() */

/*--> <-- --> <-- --> <-- --> <-- --> <--> <-- --> <-- --> <-- --> <-- --> <--*/
void
gen_route(int src, int dst, int *route, int wrap, int verbose)
{

int route_len;
int x_offset, y_offset;
int hops;
int x_dir, y_dir;
int src_port, src_sw;	/* Source connection */
int dst_port, dst_sw;	/* Destination connection */
int cur_port, cur_sw;	/* Current connection */
int out_port;		/* Which port we're going out on */
int new_port, new_sw;	/* Next connection */


    route_len= 0;
    src_port= src % MAX_COL;
    src_sw= src / MAX_COL;
    cur_port= src_port;
    cur_sw= src_sw;
    dst_port= dst % MAX_COL;
    dst_sw= dst / MAX_COL;

    if (verbose > 2)   {
	fprintf(stderr, "        From %d to %d\n", src, dst);
    }

    if (wrap)   {
	if (sw_x(dst_sw) > sw_x(cur_sw))   {
	    /* destination switch is to the right of the source switch */
	    x_offset= sw_x(dst_sw) - sw_x(cur_sw);
	    x_dir= 1;
	    hops= MAX_COL - sw_x(dst_sw) + sw_x(cur_sw);
	    if (hops < x_offset)   {
		x_dir= -1;
		x_offset= hops;
	    }
	} else   {
	    /* destination switch is to the left of the source switch */
	    x_offset= sw_x(cur_sw) - sw_x(dst_sw);
	    x_dir= -1;
	    hops= MAX_COL - sw_x(cur_sw) + sw_x(dst_sw);
	    if (hops < x_offset)   {
		x_dir= 1;
		x_offset= hops;
	    }
	}
	if (sw_y(dst_sw) > sw_y(cur_sw))   {
	    /* destination switch is below the source switch */
	    y_offset= sw_y(dst_sw) - sw_y(cur_sw);
	    y_dir= 1;
	    hops= MAX_ROW - sw_y(dst_sw) + sw_y(cur_sw);
	    if (hops < y_offset)   {
		y_dir= -1;
		y_offset= hops;
	    }
	} else   {
	    /* destination switch is above the source switch */
	    y_offset= sw_y(cur_sw) - sw_y(dst_sw);
	    y_dir= -1;
	    hops= MAX_ROW - sw_y(cur_sw) + sw_y(dst_sw);
	    if (hops < y_offset)   {
		y_dir= 1;
		y_offset= hops;
	    }
	}
    } else   {
	x_offset= abs(sw_x(dst_sw) - sw_x(cur_sw));
	if (sw_x(dst_sw) > sw_x(cur_sw))   {
	    x_dir= 1;
	} else   {
	    x_dir= -1;
	}
	y_offset= abs(sw_y(dst_sw) - sw_y(cur_sw));
	if (sw_y(dst_sw) > sw_y(cur_sw))   {
	    y_dir= 1;
	} else   {
	    y_dir= -1;
	}
    }
    if (verbose > 2)   {
	fprintf(stderr, "        X offset %d, dir %+d   Y offset %d, dir %+d\n",
	    x_offset, x_dir, y_offset, y_dir);
    }


    /* Move from switch to switch X first, then Y. */
    while (cur_sw != dst_sw)   {
	if (x_offset)   {
	    if (src % 2 == 0)   {
		/* Use lower link */
		if (x_dir > 0)   {
		    /* Go right, out of port 9 */
		    out_port= 9;
		    new_port= 13;
		} else   {
		    /* Go left, out of port 13 */
		    out_port= 13;
		    new_port= 9;
		}
	    } else   {
		/* Use upper link */
		if (x_dir > 0)   {
		    /* Go right, out of port 8 */
		    out_port= 8;
		    new_port= 12;
		} else   {
		    /* Go left, out of port 12 */
		    out_port= 12;
		    new_port= 8;
		}
	    }
	    if (verbose > 3)   {
		fprintf(stderr, "        In X direction from sw%d#%d via #%d "
		    "to sw%d#%d, route %+d\n", cur_sw, cur_port, out_port,
		    sw_neighbor(cur_sw, x_dir, 0), new_port,
		    out_port - cur_port);
	    }
	    route[route_len++]= out_port - cur_port;
	    cur_port= new_port;
	    cur_sw= sw_neighbor(cur_sw, x_dir, 0);
	    x_offset--;
	} else if (y_offset)   {
	    if (src % 2 == 0)   {
		/* Use left link */
		if (y_dir > 0)   {
		    /* Go down, out of port 14 */
		    out_port= 14;
		    new_port= 10;
		} else   {
		    /* Go up, out of port 10 */
		    out_port= 10;
		    new_port= 14;
		}
	    } else   {
		/* Use right link */
		if (y_dir > 0)   {
		    /* Go down, out of port 15 */
		    out_port= 15;
		    new_port= 11;
		} else   {
		    /* Go up, out of port 11 */
		    out_port= 11;
		    new_port= 15;
		}
	    }
	    if (verbose > 3)   {
		fprintf(stderr, "        In Y direction from sw%d#%d via #%d "
		    "to sw%d#%d, route %+d\n", cur_sw, cur_port, out_port,
		    sw_neighbor(cur_sw, x_dir, 0), new_port,
		    out_port - cur_port);
	    }
	    route[route_len++]= out_port - cur_port;
	    cur_port= new_port;
	    cur_sw= sw_neighbor(cur_sw, 0, y_dir);
	    y_offset--;
	}
    }

    /* Last hop and marker */
    if (verbose > 3)   {
	fprintf(stderr, "        To #%d, route %+d\n", dst_port,
	    dst_port - cur_port);
    }
    route[route_len++]= dst_port - cur_port;
    route[route_len]= END_MARKER;
    if (route_len >= MAX_ROUTE_LEN)   {
	fprintf(stderr, "Internal error: Route longer than %d\n",
	    MAX_ROUTE_LEN);
	exit(-1);
    }
    if (route_len > max_route_len)   {
	max_route_len= route_len;
    }
    tot_route_len += route_len;
    num_route++;

}  /* end of gen_route() */

/*--> <-- --> <-- --> <-- --> <-- --> <--> <-- --> <-- --> <-- --> <-- --> <--*/
/*
** What's the X position of switch "sw"
*/
int
sw_x(int sw)
{

int x[MAX_COL]= {4, 3, 5, 2, 6, 1, 7, 0};
int column;

    column= sw % MAX_COL;
    return x[column];

}  /* end of sw_x() */

/*--> <-- --> <-- --> <-- --> <-- --> <--> <-- --> <-- --> <-- --> <-- --> <--*/
/*
** What's the Y position of switch "sw"
*/
int
sw_y(int sw)
{

int y[MAX_ROW]= {0, 9, 1, 8, 2, 7, 3, 4, 6, 5};
int row;

    row= sw / MAX_COL;
    return y[row];

}  /* end of sw_y() */

/*--> <-- --> <-- --> <-- --> <-- --> <--> <-- --> <-- --> <-- --> <-- --> <--*/
/*
** Figure out the neighbor switch number of "sw" and consider wrap around
*/
int
sw_neighbor(int sw, int x_dir, int y_dir)
{

int x, y;


    x= (sw_x(sw) + MAX_COL + x_dir) % MAX_COL;
    y= (sw_y(sw) + MAX_ROW + y_dir) % MAX_ROW;
    return sw_num(x, y);

}  /* end of sw_neighbor() */

/*--> <-- --> <-- --> <-- --> <-- --> <--> <-- --> <-- --> <-- --> <-- --> <--*//*
** What is the switch number at postition X, y?
*/
int xy[MAX_ROW][MAX_COL]= {
    { 7,  5,  3,  1,  0,  2,  4,  6},
    {23, 21, 19, 17, 16, 18, 20, 22},
    {39, 37, 35, 33, 32, 34, 36, 38},
    {55, 53, 51, 49, 48, 50, 52, 54},
    {63, 61, 59, 57, 56, 58, 60, 62},
    {79, 77, 75, 73, 72, 74, 76, 78},
    {71, 69, 67, 65, 64, 66, 68, 70},
    {47, 45, 43, 41, 40, 42, 44, 46},
    {31, 29, 27, 25, 24, 26, 28, 30},
    {15, 13, 11,  9,  8, 10, 12, 14}
};
int
sw_num(int x, int y)
{
    return xy[y][x];
}  /* end of sw_num() */

/*--> <-- --> <-- --> <-- --> <-- --> <--> <-- --> <-- --> <-- --> <-- --> <--*/
void
usage(char *pname)
{
    fprintf(stderr, "Usage:   %s [-w] [-v] [-v] [-v] [out_file]\n", pname);
    fprintf(stderr, "         -w Don't use wrap arounds\n");
    fprintf(stderr, "         -v Increase verbosity\n");
}  /* end of usage() */

/*--> <-- --> <-- --> <-- --> <-- --> <--> <-- --> <-- --> <-- --> <-- --> <--*/
