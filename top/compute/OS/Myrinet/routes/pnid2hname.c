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
** $Id: pnid2hname.c,v 1.4 2000/12/01 23:41:56 rolf Exp $
**
** Given a physical node ID, calculate the host name. This file
** must me linked with a machine specific file that provides the
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


    if (argc < 3)   {
	fprintf(stderr, "Usage: %s pnid system\n", argv[0]);
        fprintf(stderr, "where \"system\" is an installation type\n");
        fprintf(stderr, "such as \"siberia\"\n");
	return -1;
    }
    pnid= atoi(argv[1]);
    if ((pnid < 0) || (pnid > 639))   {
	fprintf(stderr, "%s: ERROR pnid %d invalid\n", pnid);
	return -1;
    }

    if ( strcmp(argv[2], "siberia") == 0 ) { 
      port= pnid % 8;
      sw= pnid / 8;
      gen_host_name_siberia(label, sw, port);
      printf("%s\n", label);
      return 0;
    }

    if ( strcmp(argv[2], "iceberg") == 0 ) {
      gen_host_name_iceberg(label, pnid);
      printf("%s\n", label);
      return 0;
    }

    if ( strcmp(argv[2], "iceberg2") == 0 ) {
      gen_host_name_iceberg2(label, pnid);
      printf("%s\n", label);
      return 0;
    }

    if ( strcmp(argv[2], "alaska") == 0 ) {
      gen_host_name_alaska(label, pnid);
      printf("%s\n", label);
      return 0;
    }

    fprintf(stderr,"ERROR: system type %s doesnt match known types\n", argv[2]);
    return -1;

}  /* end of main() */
