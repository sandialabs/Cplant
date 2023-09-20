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
** $Id: geniceberg2.c,v 1.3 2000/12/01 23:41:56 rolf Exp $
**
** This program has hard-coded into it the topology and naming and
** numbering scheme of Iceberg2. It produces a Myricom mapper file on
** stdout that can be processed by Myrinet tools such as simple_route
** to generate routes.
** Link it with gen_host.c for the machine specific function
** gen_host_name_iceberg2()
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


    num_sw= 2;

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
	** Six more ports are used to connect to other switches
	*/
	printf("12\n");

	/* Hosts */
	for (port= 0; port < 8; port++)   {
	    pnid= port + sw * 8;
	    gen_host_name_iceberg2(label, pnid);
	    printf("%d h - \"%s\" 0\n", port, label);
	}
	/* Other Switches */
	switch (sw)   {
	    case 0:
		other_sw= 1;
		break;
	    case 1:
		other_sw= 0;
		break;
	    default:
		fprintf(stderr, "Not that many switches in iceberg2!\n");
		exit(-1);
	}

	printf("8 s 16 \"s%d\" 12\n", other_sw);
	printf("9 s 16 \"s%d\" 13\n", other_sw);
	printf("12 s 16 \"s%d\" 8\n", other_sw);
	printf("13 s 16 \"s%d\" 9\n", other_sw);

	gen_switch_name_iceberg2(label, sw);
	printf("label \"%s\"\n\n", label);
    }
}  /* end of create_switches() */


void
create_hosts(int num_sw)
{

int sw;


    /* We have 2 switches with 8 hosts on each */
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
	gen_host_name_iceberg2(label, pnid);

	printf("h - \"%s\"\n", label);
	printf("1\n");
	printf("0 s 16 \"s%d\" %d\n", sw, port);
	printf("label \"%s\"\n\n", label);
    }
}  /* end of hosts_on_sw() */
