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

#include <stdio.h>
#include "mpi.h"

#define TABLE_SIZE 2

main( argc, argv )
int argc;
char **argv;
{
  int    rank, size;
  double a[TABLE_SIZE];
  struct { double a; int b; } in[TABLE_SIZE], out[TABLE_SIZE];
  int    i, ranks[TABLE_SIZE];
  int    errors = 0;

  /* Initialize the environment and some variables */
  MPI_Init( &argc, &argv );
  MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  MPI_Comm_size( MPI_COMM_WORLD, &size );

  /* Initialize the maxloc data */
  for ( i=0; i<TABLE_SIZE; i++ ) a[i] = 0;
  for ( i=rank; i<TABLE_SIZE; i++ ) a[i] = (double)rank + 1.0;

  /* Copy data to the "in" buffer */
  for (i=0; i<TABLE_SIZE; i++) { 
	in[i].a = a[i];
	in[i].b = (double)rank;
  }

  /* Reduce it! */
  MPI_Reduce( in, out, TABLE_SIZE, MPI_DOUBLE_INT, MPI_MAXLOC, 0, MPI_COMM_WORLD );
  MPI_Bcast ( out, TABLE_SIZE, MPI_DOUBLE_INT, 0, MPI_COMM_WORLD );

  /* Check to see that we got the right answers */
  for (i=0; i<TABLE_SIZE; i++) 
	if (i % size == rank)
	  if (out[i].b != rank) {
        printf("MAX (ranks[%d] = %d != %d\n", i, out[i].b, rank );
		errors++;
      }

  /* Initialize the minloc data */
  for ( i=0; i<TABLE_SIZE; i++ ) a[i] = 0;
  for ( i=rank; i<TABLE_SIZE; i++ ) a[i] = -(double)rank - 1.0;

  /* Copy data to the "in" buffer */
  for (i=0; i<TABLE_SIZE; i++)  {
	in[i].a = a[i];
	in[i].b = rank;
  }

  /* Reduce it! */
  MPI_Allreduce( in, out, TABLE_SIZE, MPI_DOUBLE_INT, MPI_MINLOC, MPI_COMM_WORLD );

  /* Check to see that we got the right answers */
  for (i=0; i<TABLE_SIZE; i++) 
	if (i % size == rank)
	  if (out[i].b != rank) {
        printf("MIN (ranks[%d] = %d != %d\n", i, out[i].b, rank );
		errors++;
      }

  /* Finish up! */
  MPI_Finalize();
  if (errors)
	printf( "[%d] done with ERRORS(%d)!\n", rank, errors );
  return errors;
}
