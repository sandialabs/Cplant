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
** $Id: barrier.c,v 1.1 2000/03/03 22:29:22 rolf Exp $
*/
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <p30.h>

#define DEFAULT_CNT	(1000)


/* ------------------------------------------------------------------------- */
/* Local functions */
static void usage(char *pname);


/* ------------------------------------------------------------------------- */
/* Command line options */
static struct option opts[]=
{
    {"count",		required_argument,	0, 'C'},
    {"cnt",		required_argument,	0, 'C'},
    {"help",		no_argument,		0, 'H'},
};


/* ------------------------------------------------------------------------- */

int
main(int argc, char *argv[])
{

int rc, i;
int cnt;
ptl_handle_ni_t ni;
ptl_id_t nnodes;
ptl_process_id_t my_id;

int index;
int ch;
int error;


    /* Set defaults */
    cnt= DEFAULT_CNT;
    error= 0;

    while (1)   {
	ch= getopt_long_only(argc, argv, "", opts, &index);
	if (ch == EOF)   {
	    break;
	}

	switch (ch)   {
	    case 'C':
		cnt= strtol(optarg, NULL, 10);
		break;
	    case 'H':
		usage(argv[0]);
		exit(0);
		break;
	    default:
		error= 1;
	}
    }

    if (error)   {
	usage(argv[0]);
	exit(-1);
    }


    if ((rc= PtlInit()) != PTL_OK)   {
	fprintf(stderr, "PtlInit() failed (%s)\n", ptl_err_str[rc]);
	return -1;
    }

    if ((rc= PtlNIInit(PTL_IFACE_DEFAULT, 32, 4, &ni)) != PTL_OK)   {
	fprintf(stderr, "PtlNIInit(if %p, 32, 4) failed (%s)\n",
	    PTL_IFACE_DEFAULT, ptl_err_str[rc]);
	return -1;
    }

    if ((rc= PtlGetId(ni, &my_id, &nnodes)) != PTL_OK)   {
	fprintf(stderr, "PtlGetId() failed (%s)\n", ptl_err_str[rc]);
	return -1;
    }


    for (i= 0; i < cnt; i++)   {
	rc= PtlNIBarrier(ni);
	if (rc != PTL_OK)   {
	    fprintf(stderr, "PtlNIBarrier() failed (%s)\n", ptl_err_str[rc]);
	}
    }


    if ((rc= PtlNIFini(ni)) != PTL_OK)   {
	fprintf(stderr, "PtlNIFini failed (%s)\n", ptl_err_str[rc]);
	return -1;
    }

    PtlFini();

    if (my_id.rid == 0)   {
	fprintf(stderr, "Called PtlNIBarrier() %d times\n", cnt);
    }

    return 0;

}  /* end of main() */

/* ------------------------------------------------------------------------- */

static void
usage(char *pname)
{

    fprintf(stderr, "Usage: %s [-cnt cnt] [-count cnt]\n", pname);
    fprintf(stderr, "    cnt    How many PtlNIBarriers (default %d)\n",
	DEFAULT_CNT);

}  /* end of usage() */

/* ------------------------------------------------------------------------- */
