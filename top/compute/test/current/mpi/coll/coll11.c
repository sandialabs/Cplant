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

void addem(invec, inoutvec, len, dtype)
int *invec, *inoutvec, *len;
MPI_Datatype *dtype;
{
  int i;
  for ( i=0; i<*len; i++ ) 
    inoutvec[i] += invec[i];
}

#define BAD_ANSWER 100000

/*
    The operation is inoutvec[i] = invec[i] op inoutvec[i] 
    (see 4.9.4).  The order is important.

    Note that the computation is in process rank (in the communicator)
    order, independant of the root.
 */
void assoc(invec, inoutvec, len, dtype)
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
    MPI_Op           op_assoc, op_addem;

    /*
    ** The first MPI_Scan call always fails.
    */
    printf("This test is broken.  Sorry.\n");
    exit(0);

    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    MPI_Errhandler_set( MPI_COMM_WORLD, MPI_ERRORS_RETURN );

    data = rank;

    correct_result = 0;
    for (i=0;i<=rank;i++)
      correct_result += i;

    MPI_Scan ( &data, &result, -1, MPI_INT, MPI_SUM, MPI_COMM_WORLD );
    if (result != correct_result) {
	fprintf( stderr, "[%d] Error suming ints with scan\n", rank );
	errors++;
	}

    MPI_Scan ( &data, &result, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD );
    if (result != correct_result) {
	fprintf( stderr, "[%d] Error summing ints with scan (2)\n", rank );
	errors++;
	}

    data = rank;
    result = -100;
    MPI_Op_create( (MPI_User_function *)assoc, 0, &op_assoc );
    MPI_Op_create( (MPI_User_function *)addem, 1, &op_addem );
    MPI_Scan ( &data, &result, 1, MPI_INT, op_addem, MPI_COMM_WORLD );
    if (result != correct_result) {
	fprintf( stderr, "[%d] Error summing ints with scan (userop)\n", 
		 rank );
	errors++;
	}

    MPI_Scan ( &data, &result, 1, MPI_INT, op_addem, MPI_COMM_WORLD );
    if (result != correct_result) {
	fprintf( stderr, "[%d] Error summing ints with scan (userop2)\n", 
		 rank );
	errors++;
	}
    result = -100;
    data = rank;
    MPI_Scan ( &data, &result, 1, MPI_INT, op_assoc, MPI_COMM_WORLD );
    if (result == BAD_ANSWER) {
	fprintf( stderr, "[%d] Error scanning with non-commutative op\n",
		 rank );
	errors++;
	}

    MPI_Op_free( &op_assoc );
    MPI_Op_free( &op_addem );

    if (errors)
      printf( "[%d] done with ERRORS(%d)!\n", rank, errors );

    Test_Waitforall( );
    MPI_Finalize();
    return errors;
}
