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
** $Id: geniceberg.c,v 1.1 2000/12/01 23:41:56 rolf Exp $
**
** This program has hard-coded into it the topology and naming and
** numbering scheme of Iceberg. It produces a Myricom mapper file on
** stdout that can be processed by Myrinet tools such as simple_route
** to generate routes.
** Link it with gen_host.c for the machine specific function
** gen_host_name_iceberg()
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


    num_sw= 4;

    printf("%d %d\n", num_sw * 8, num_sw);
    create_switches(num_sw);
    create_hosts(num_sw);

    return 0;

}  /* end of main() */


void
create_switches(int num_sw)
{

int sw;
int other_sw;
int port;
int pnid;
char label[256];


    /* For every switch generate an entry in the map file */
    for (sw= 0; sw < num_sw; sw++)   {
	printf("s 16 \"s%d\"\n", sw);
	/*
	** There are 8 nodes on each switch. They are attached to ports 0 - 7
	** Switches 1 and 2 are connected to two other switches. Switches
	** 0 and 3 have only 1 connection
	*/
	switch (sw)   {
	    case 0:
	    case 3:
		printf("9\n");
		break;
	    case 1:
	    case 2:
		printf("10\n");
		break;
	}


	/* Hosts */
	for (port= 0; port < 8; port++)   {
	    pnid= port + sw * 8;
	    gen_host_name_iceberg(label, pnid);
	    if ((sw == 3) && ((port == 0) || (port == 1)))   {
		/* Special case for two LANai 9.x nodes */
		printf("%d h - \"%s\" 0\n", 15 - port, label);
	    } else   {
		printf("%d h - \"%s\" 0\n", port, label);
	    }
	}
	/* Other Switches */
	switch (sw)   {
	    case 0:
		printf("11 s 16 \"s1\" 11\n");
		break;
	    case 1:
		printf("10 s 16 \"s2\" 11\n");
		printf("11 s 16 \"s0\" 11\n");
		break;
	    case 2:
		printf("10 s 16 \"s3\" 11\n");
		printf("11 s 16 \"s1\" 10\n");
		break;
	    case 3:
		printf("11 s 16 \"s2\" 10\n");
		break;
	    default:
		fprintf(stderr, "Not that many switches in iceberg!\n");
		exit(-1);
	}

	gen_switch_name_iceberg(label, sw);
	printf("label \"%s\"\n\n", label);
    }
}  /* end of create_switches() */


void
create_hosts(int num_sw)
{

int sw;


    /* We have 4 switches with 8 hosts on each */
    for (sw= 0; sw < num_sw; sw++)   {
	hosts_on_sw(sw);
    }
}  /* end of create_hosts() */


void
hosts_on_sw(int sw)
{

int node;
int port;
int pnid;
char label[256];


    /*
    ** There are 8 nodes on each switch. They are attached to ports 0 - 7
    */
    for (node= 0; node < 8; node++)   {
	port= node % 8;
	pnid= port + sw * 8;
	gen_host_name_iceberg(label, pnid);

	printf("h - \"%s\"\n", label);
	printf("1\n");
	if ((sw == 3) && ((port == 0) || (port == 1)))   {
	    /* Special case for two LANai 9.x nodes */
	    printf("0 s 16 \"s%d\" %d\n", sw, 15 - port);
	} else   {
	    printf("0 s 16 \"s%d\" %d\n", sw, port);
	}
	printf("label \"%s\"\n\n", label);
    }
}  /* end of hosts_on_sw() */
