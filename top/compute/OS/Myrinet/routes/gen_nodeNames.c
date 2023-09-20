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
** $Id: gen_nodeNames.c,v 1.4 2000/12/01 23:41:56 rolf Exp $
**
** This program generates a list of host names in order of their physical
** node ID.
** This file must me linked with a machine specific file that provides the
** function gen_host_name().
*/
#include <stdio.h>
#include "gen_host.h"


int
main(int argc, char *argv[])
{

int port;
int sw;
int next_sw;
int pnid;
char label[256];

    if (argc != 2) {
      fprintf(stderr," Usage: %s system\n", argv[0]);
      fprintf(stderr," for example: %s siberia\n", argv[0]);
      return -1;
    }

    for (pnid= 0; pnid < 640; pnid++)   {

        if ( strcmp(argv[1],"siberia") == 0 ) {
	  port= pnid % 8;
	  sw= pnid / 8;
	  gen_host_name_siberia(label, sw, port);
        }
        if ( strcmp(argv[1],"iceberg") == 0 ) {
	  gen_host_name_iceberg(label, pnid);
        }
        if ( strcmp(argv[1],"iceberg2") == 0 ) {
	  gen_host_name_iceberg2(label, pnid);
        }
        if ( strcmp(argv[1],"alaska") == 0 ) {
	  gen_host_name_alaska(label, pnid);
        }
	printf("    %3d is %s\n", pnid, label);
    }

    return 0;

}  /* end of main() */
