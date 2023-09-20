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
 *  $Id: pack.c,v 1.1 1997/10/29 22:45:06 bright Exp $
 *
 *  (C) 1993 by Argonne National Laboratory and Mississippi State University.
 *
 */
 

#include "mpi.h"
#include <stdio.h>

/*
   Check pack/unpack of mixed datatypes.
 */
#define BUF_SIZE 100
main(argc, argv)
int argc;
char **argv;
{
    int myrank;
    char buffer[BUF_SIZE];
    int n, size, src, dest, errcnt, errs;
    double a,b;
    int pos;

    MPI_Status status;
    MPI_Init(&argc, &argv);
    
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

    src	   = 0;
    dest   = 1;
    errcnt = 0;
    if (myrank == src)
	{
	    pos	= 0;
	    n	= 10;
	    a	= 1.1;
	    b	= 2.2;
	    MPI_Pack(&n, 1, MPI_INT, buffer, BUF_SIZE, &pos, MPI_COMM_WORLD);
	    MPI_Pack(&a, 1, MPI_DOUBLE, buffer, BUF_SIZE, &pos, 
		     MPI_COMM_WORLD);
	    MPI_Pack(&b, 1, MPI_DOUBLE, buffer, BUF_SIZE, &pos, 
		     MPI_COMM_WORLD);
	    /* printf( "%d\n", pos ); */
	    MPI_Send(&pos, 1, MPI_INT, dest, 999, MPI_COMM_WORLD);
	    MPI_Send(buffer, pos, MPI_PACKED, dest, 99, MPI_COMM_WORLD);
	}
    else
	{
	    MPI_Recv(&size, 1, MPI_INT, src, 999, MPI_COMM_WORLD, &status);
	    MPI_Recv(buffer, size, MPI_PACKED, src, 99, 
		     MPI_COMM_WORLD, &status);
	    pos = 0;
	    MPI_Unpack(buffer, size, &pos, &n, 1, MPI_INT, MPI_COMM_WORLD);
	    MPI_Unpack(buffer, size, &pos, &a, 1, MPI_DOUBLE, MPI_COMM_WORLD);
	    MPI_Unpack(buffer, size, &pos, &b, 1, MPI_DOUBLE, MPI_COMM_WORLD);
	    /* Check results */
	    if (n != 10) { 
		errcnt++;
		printf( "Wrong value for n; got %d expected %d\n", n, 10 );
		}
	    if (a != 1.1) { 
		errcnt++;
		printf( "Wrong value for a; got %f expected %f\n", a, 1.1 );
		}
	    if (b != 2.2) { 
		errcnt++;
		printf( "Wrong value for b; got %f expected %f\n", b, 2.2 );
		}
	}
    MPI_Allreduce( &errcnt, &errs, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD );
    if (myrank == 0) {
	if (errs == 0) printf( "No errors\n" );
	else           printf( "%d errors\n", errs );
	}
    MPI_Finalize();
return 0;
}
