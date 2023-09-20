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
 * Test of reduce scatter.
 *
 * Each processor contributes its rank + the index to the reduction, 
 * then receives the ith sum
 *
 * Can be called with any number of processors.
 */

#include "mpi.h"
#include <stdio.h>
#include <malloc.h>
#include <memory.h>

int main( argc, argv )
int  argc;
char **argv;
{
int      err = 0, toterr;
int      *sendbuf, recvbuf, *recvcounts;
int      size, rank, i, sumval;
MPI_Comm comm;


MPI_Init( &argc, &argv );
comm = MPI_COMM_WORLD;

MPI_Comm_size( comm, &size );
MPI_Comm_rank( comm, &rank );
sendbuf = (int *) malloc( size * sizeof(int) );
for (i=0; i<size; i++) 
    sendbuf[i] = rank + i;
recvcounts = (int *)malloc( size * sizeof(int) );
for (i=0; i<size; i++) 
    recvcounts[i] = 1;

MPI_Reduce_scatter( sendbuf, &recvbuf, recvcounts, MPI_INT, MPI_SUM, comm );

sumval = size * rank + ((size - 1) * size)/2;
/* recvbuf should be size * (rank + i) */
if (recvbuf != sumval) {
    err++;
    fprintf( stdout, "Did not get expected value for reduce scatter\n" );
    fprintf( stdout, "[%d] Got %d expected %d\n", rank, recvbuf, sumval );
    }

MPI_Allreduce( &err, &toterr, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD );
if (rank == 0 && toterr == 0) {
    printf( "No errors found in MPI_Reduce_scatter\n" );
    }
MPI_Finalize( );

return toterr;
}
