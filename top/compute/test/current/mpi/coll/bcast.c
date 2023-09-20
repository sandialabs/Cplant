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
 * This program performs some simple tests of the MPI_Bcast broadcast
 * functionality.
 */

#include "test.h"
#include "mpi.h"
#include <stdlib.h>

int
main(argc, argv)
int argc;
char **argv;
{
    int rank, size, ret, passed, i, *test_array;
    MPI_Status Status;

    /* Set up MPI */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /* Setup the tests */
    Test_Init("bcast", rank);
    test_array = (int *)malloc(size*sizeof(int));

    /* Perform the test - this operation should really be done
       with an allgather, but it makes a good test... */
    passed = 1;
    for (i=0; i < size; i++) {
	if (i == rank)
	    test_array[i] = i;
	MPI_Bcast(test_array, size, MPI_INT, i, MPI_COMM_WORLD);
	if (test_array[i] != i)
	    passed = 0;
    }
    if (!passed)
	Test_Failed("Simple Broadcast test");
    else {
	if (rank == 0)
	    Test_Passed("Simple Broadcast test");
	}

    /* Close down the tests */
    free(test_array);
    if (rank == 0)
	ret = Summarize_Test_Results();
    else
	ret = 0;
    Test_Finalize();

    /* Close down MPI */
    Test_Waitforall( );
    MPI_Finalize();
    return ret;
}
