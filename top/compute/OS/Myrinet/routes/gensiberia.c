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
** $Id: gensiberia.c,v 1.4 2000/12/01 23:41:56 rolf Exp $
**
** This program has hard-coded into it the topology and naming and
** numbering scheme of Siberia. It produces a Myricom mapper file on
** stdout that can be processed by Myrinet tools such as simple_route
** to generate routes.
** Link it with gen_host_siberia.c for the machine specific function
** gen_host_name_siberia()
*/
#include <stdio.h>
#include "gen_host.h"


void create_switches(int num_sw);
void create_hosts(int num_sw);
void hosts_on_sw(int sw);


int
main(int argc, char *argv[])
{

int num_sw;


    num_sw= 80;

    printf("%d %d\n", num_sw * 8, num_sw);
    create_switches(num_sw);
    create_hosts(num_sw);

    return 0;

}  /* end of main() */


void
create_switches(int num_sw)
{

int sw;
int west_sw, east_sw;
int south_sw, north_sw;
int min_x_sw, max_x_sw;
int port;
int row, col;
char label[256];


    /* For every switch generate an entry in the map file */
    for (sw= 0; sw < num_sw; sw++)   {
	printf("s 16 \"s%d\"\n", sw);
	/*
	** There are 8 nodes on each switch. They are attached to ports 0 - 7
	** However, on sw 6, 15, 22, 31, 62, and 71, there are only 2 (I/O)
	** nodes. We still need to generate routes for all exisiting ports.
	*/
	printf("16\n");

	/* Hosts */
	for (port= 0; port < 8; port++)   {
	    gen_host_name_siberia(label, sw, port);
	    printf("%d h - \"%s\" 0\n", port, label);
	}
	/* Other Switches */
	min_x_sw= (sw >> 3) * 8;
	max_x_sw= (sw >> 3) * 8 + 7;
	if ((sw % 2) == 0)   {
	    west_sw= sw + 2;
	    east_sw= sw - 2;
	    if (west_sw > max_x_sw) west_sw--;
	    if (east_sw < min_x_sw) east_sw= min_x_sw + 1;
	} else   {
	    west_sw= sw - 2;
	    east_sw= sw + 2;
	    if (west_sw < min_x_sw) west_sw++;
	    if (east_sw > max_x_sw) east_sw= max_x_sw - 1;
	}
	row= sw / 8;
	col= 8 - (sw % 8);
	switch (row)   {
	    case 6:
		north_sw= sw + 8;
		break;
	    case 1: case 9:
		north_sw= sw - 8;
		break;
	    case 0: case 2: case 4: case 7:
		north_sw= sw + 16;
		break;
	    case 3: case 5:
		north_sw= sw - 16;
		break;
	    case 8:
		north_sw= sw - 24;
		break;
	}
	switch (row)   {
	    case 0: case 8:
		south_sw= sw + 8;
		break;
	    case 7:
		south_sw= sw - 8;
		break;
	    case 1: case 3:
		south_sw= sw + 16;
		break;
	    case 9: case 6: case 4: case 2:
		south_sw= sw - 16;
		break;
	    case 5:
		south_sw= sw + 24;
		break;
	}
	printf("8 s 16 \"s%d\" 12\n", west_sw);
	printf("9 s 16 \"s%d\" 13\n", west_sw);
	printf("10 s 16 \"s%d\" 14\n", south_sw);
	printf("11 s 16 \"s%d\" 15\n", south_sw);
	printf("12 s 16 \"s%d\" 8\n", east_sw);
	printf("13 s 16 \"s%d\" 9\n", east_sw);
	printf("14 s 16 \"s%d\" 10\n", north_sw);
	printf("15 s 16 \"s%d\" 11\n", north_sw);

	gen_switch_name_siberia(label, sw);
	printf("label \"%s\"\n\n", label);
    }
}  /* end of create_switches() */


void
create_hosts(int num_sw)
{

int sw;


    /* We have 80 switches with 8 hosts on each */
    for (sw= 0; sw < num_sw; sw++)   {
	hosts_on_sw(sw);
    }
}  /* end of create_hosts() */


void
hosts_on_sw(int sw)
{

int node;
int port;
char label[256];


    /*
    ** There are 8 nodes on each switch. They are attached to ports 0 - 7
    ** However, on sw 6, 15, 22, 31, 62, and 71, there are only 2 (I/O)
    ** nodes. We'll generate ficticous nodes for those ports.
    */
    for (node= 0; node < 8; node++)   {
	port= node % 8;
	gen_host_name_siberia(label, sw, port);

	printf("h - \"%s\"\n", label);
	printf("1\n");
	printf("0 s 16 \"s%d\" %d\n", sw, port);
	printf("label \"%s\"\n\n", label);
    }
}  /* end of hosts_on_sw() */
