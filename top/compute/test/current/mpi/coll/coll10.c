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
#include "mpi.h"
#include <stdio.h>
#define BAD_ANSWER 100000

/*
    The operation is inoutvec[i] = invec[i] op inoutvec[i] 
    (see 4.9.4).  The order is important.

    Note that the computation is in process rank (in the communicator)
    order, independant of the root.
 */
int assoc(invec, inoutvec, len, dtype)
int *invec, *inoutvec, *len;
MPI_Datatype *dtype;
{
  int i;
  for ( i=0; i<*len; i++ )  {
    if (inoutvec[i] <= invec[i] ) {
      int rank;
      MPI_Comm_rank( MPI_COMM_WORLD, &rank );
      fprintf( stderr, "[%d] inout[0] = %d, in[0] = %d\n", 
	       rank, inoutvec[0], invec[0] );
      inoutvec[i] = BAD_ANSWER;
      }
    else 
      inoutvec[i] = invec[i];
  }
  return (1);
}

int main( argc, argv )
int argc;
char **argv;
{
    int              rank, size, i;
    MPI_Request      handle;
    MPI_Status       status;
    int              data;
    int              errors=0;
    int              result = -100;
    int              correct_result;
    MPI_Op           op;

    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    MPI_Comm_size( MPI_COMM_WORLD, &size );

    data = rank;

    MPI_Op_create( (MPI_User_function*)assoc, 0, &op );
    MPI_Reduce ( &data, &result, 1, MPI_INT, op, size-1, MPI_COMM_WORLD );
    MPI_Bcast  ( &result, 1, MPI_INT, size-1, MPI_COMM_WORLD );
    MPI_Op_free( &op );
    if (result == BAD_ANSWER) errors++;

    if (errors)
      printf( "[%d] done with ERRORS(%d)!\n", rank, errors );
    Test_Waitforall( );
    MPI_Finalize();

    return errors;
}
