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
** $Id: showroutesiberia.c,v 1.4 2001/11/30 00:05:39 pumatst Exp $
**
** Given a starting node (pnid) and a route, show the path; i.e.
** switches and ports plus destination pnid. Compare the calculated
** destination pnid with the one supplied on the command line.
**
** This program is specific to the naming and numbering conventions
** as well as the topology of Siberia.
**
** Usage:  showroutesiberia snid dnid route_bytes....
*/

#include <stdio.h>
#include "gen_host.h"


int check_route_byte(char *rb);
int connection(int out_port, int src_sw, int route_byte, int *next_port,
	int *next_sw);


int
main(int argc, char *argv[])
{

int i;
int start_pnid;
int dest_pnid;
int route_byte;
int in_port;
int out_port;
int next_port;
int sw;
int next_sw;
int pnid;
char label[256];
char label2[256];


    if (argc < 4)   {
	fprintf(stderr, "Usage: %s src dst route\n", argv[0]);
	fprintf(stderr, "    Example: %s 61 88 0x84 0xbc 0xbe 0xb1\n", argv[0]);
	#ifdef NOTYET
	fprintf(stderr, "    Example: %s 61 c-0.SU-5 0x84 0xbc 0xbe 0xb1\n",
	    argv[0]);
	fprintf(stderr, "    Example: %s c-5.SU-3 c-0.SU-5 0x84 0xbc 0xbe "
	    "0xb1\n", argv[0]);
	#endif /* NOTYET */
	return -1;
    }
    start_pnid= atoi(argv[1]);
    if ((start_pnid < 0) || (start_pnid > 639))   {
	fprintf(stderr, "%s: ERROR src pnid %d invalid\n", start_pnid);
	return -1;
    }
    dest_pnid= atoi(argv[2]);
    if ((dest_pnid < 0) || (dest_pnid > 639))   {
	fprintf(stderr, "%s: ERROR dest pnid %d invalid\n", dest_pnid);
	return -1;
    }
    in_port= start_pnid % 8;
    sw= start_pnid / 8;
    gen_host_name_siberia(label, sw, in_port);
    printf("    Source %s (pnid %d) switch %d, port %d\n", label, start_pnid,
	sw, in_port);

    for (i= 3; i < argc; i++)   {
	if ((route_byte= check_route_byte(argv[i])) == -999)   {
	    fprintf(stderr, "%s: Invalid route byte %d: 0x%02x (%d)\n",
		argv[0], route_byte, route_byte);
	    return -1;
	}

	out_port= in_port + route_byte;
	if ((out_port < 0) || (out_port > 15))   {
	    fprintf(stderr, "Invalid exit port %d: in %d, route %d\n",
		out_port, in_port, route_byte);
	    return -1;
	}

	if ((out_port >= 0) && (out_port <= 7))   {
	    /* We're at the end */
	    pnid= 8 * sw + out_port;
	    gen_host_name_siberia(label, sw, out_port);
	    if (pnid == dest_pnid)   {
		printf("                        Destination %s (pnid %d) switch %d, port %d\n",
		    label, pnid, sw, out_port);
		return 0;
	    } else   {
		gen_host_name_siberia(label2, dest_pnid / 8, dest_pnid % 8);
		printf("                        Destination %s (pnid %d) switch %d, port %d IS NOT"
		    "\n                        expected destination %s (pnid %d)\n",
		    label, pnid, sw, out_port, label2, dest_pnid);

		return -1;
	    }
	}

	if (connection(out_port, sw, route_byte, &next_port, &next_sw) != 0)   {
	    fprintf(stderr, "Invalid connection: sw %d, p %d, route %d\n",
		sw, out_port, route_byte);
	    return -1;
	}
	printf("        Route %3d:   Switch %2d port %2d ---> Switch %2d "
	    "port %2d\n",
	    route_byte, sw, out_port, next_sw, next_port);
	in_port= next_port;
	sw= next_sw;
    }

    return 0;

}  /* end of main() */


int
check_route_byte(char *rb)
{

int route_byte;


    route_byte= strtol(rb, NULL, 0);

    /*
    ** If it is in the range of 0x80..0x8f or 0xb1..0xbf, then it
    ** is a wire route byte.
    ** If it is in the range of -15..15, then it is in Myrinet
    ** route file format.
    ** Anything else is invalid on 16-port switches.
    */
    if ((route_byte >= 0x80) && (route_byte <= 0x8f))   {
	route_byte -= 0x80;
    } else if ((route_byte >= 0xb1) && (route_byte <= 0xbf))   {
	route_byte -= 0xc0;
    } else if ((route_byte >= -15) && (route_byte <= 15))   {
	route_byte= route_byte;
    } else   {
	route_byte= -999;
    }

    return route_byte;

} /* end of check_route_byte() */


int
connection(int out_port, int src_sw, int route_byte, int *next_port,
	int *next_sw)
{

int west_sw, east_sw;
int south_sw, north_sw;
int min_x_sw, max_x_sw;
int row, col;


    if ((out_port < 8) || (out_port > 15))   {
	return -1;
    }
    if ((src_sw < 0) || (src_sw > 79))   {
	return -1;
    }

    min_x_sw= (src_sw >> 3) * 8;
    max_x_sw= (src_sw >> 3) * 8 + 7;
    if ((src_sw % 2) == 0)   {
	west_sw= src_sw + 2;
	east_sw= src_sw - 2;
	if (west_sw > max_x_sw) west_sw--;
	if (east_sw < min_x_sw) east_sw= min_x_sw + 1;
    } else   {
	west_sw= src_sw - 2;
	east_sw= src_sw + 2;
	if (west_sw < min_x_sw) west_sw++;
	if (east_sw > max_x_sw) east_sw= max_x_sw - 1;
    }
    row= src_sw / 8;
    col= 8 - (src_sw % 8);
    switch (row)   {
	case 6:
	    north_sw= src_sw + 8;
	    break;
	case 1: case 9:
	    north_sw= src_sw - 8;
	    break;
	case 0: case 2: case 4: case 7:
	    north_sw= src_sw + 16;
	    break;
	case 3: case 5:
	    north_sw= src_sw - 16;
	    break;
	case 8:
	    north_sw= src_sw - 24;
	    break;
    }
    switch (row)   {
	case 0: case 8:
	    south_sw= src_sw + 8;
	    break;
	case 7:
	    south_sw= src_sw - 8;
	    break;
	case 1: case 3:
	    south_sw= src_sw + 16;
	    break;
	case 9: case 6: case 4: case 2:
	    south_sw= src_sw - 16;
	    break;
	case 5:
	    south_sw= src_sw + 24;
	    break;
    }

    switch (out_port)   {
	case 8: *next_sw= west_sw; *next_port= 12; break;
	case 9: *next_sw= west_sw; *next_port= 13; break;
	case 10: *next_sw= south_sw; *next_port= 14; break;
	case 11: *next_sw= south_sw; *next_port= 15; break;
	case 12: *next_sw= east_sw; *next_port= 8; break;
	case 13: *next_sw= east_sw; *next_port= 9; break;
	case 14: *next_sw= north_sw; *next_port= 10; break;
	case 15: *next_sw= north_sw; *next_port= 11; break;
    }

    return 0;

}  /* end of connection() */
