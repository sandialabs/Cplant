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

#define MAX_PROCESSES 10

int main( argc, argv )
int argc;
char **argv;
{
    int              rank, size, i,j;
    MPI_Request      handle;
    MPI_Status       status;
    int              table[MAX_PROCESSES][MAX_PROCESSES];
    int              errors=0;
    int              participants;
    int              displs[MAX_PROCESSES];
    int              recv_counts[MAX_PROCESSES];

    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    MPI_Comm_size( MPI_COMM_WORLD, &size );

    /* A maximum of MAX_PROCESSES processes can participate */
    if ( size > MAX_PROCESSES ) participants = MAX_PROCESSES;
    else              participants = size;
    /* while (MAX_PROCESSES % participants) participants--; */
    if (MAX_PROCESSES % participants) {
	fprintf( stderr, "Number of processors must divide %d\n",
		MAX_PROCESSES );
	MPI_Abort( MPI_COMM_WORLD, 1 );
	}
    if ( (rank < participants) ) {

      /* Determine what rows are my responsibility */
      int block_size = MAX_PROCESSES / participants;
      int begin_row  = rank * block_size;
      int end_row    = (rank+1) * block_size;
      int send_count = block_size * MAX_PROCESSES;
      
      /* Fill in the displacements and recv_counts */
      for (i=0; i<participants; i++) {
	displs[i]      = i * block_size * MAX_PROCESSES;
	recv_counts[i] = send_count;
      }

      /* Paint my rows my color */
      for (i=begin_row; i<end_row ;i++)
	for (j=0; j<MAX_PROCESSES; j++)
	  table[i][j] = rank + 10;
      
      /* Gather everybody's result together - sort of like an */
      /* inefficient allgather */
      for (i=0; i<participants; i++) {
	MPI_Gatherv(&table[begin_row][0], send_count, MPI_INT, 
		    &table[0][0], recv_counts, displs, MPI_INT, 
		    i, MPI_COMM_WORLD);
      }


      /* Everybody should have the same table now.

	 The entries are:
	 Table[i][j] = (i/block_size) + 10;
       */
      for (i=0; i<MAX_PROCESSES;i++) 
	if ( (table[i][0] - table[i][MAX_PROCESSES-1] !=0) ) 
	  errors++;
      for (i=0; i<MAX_PROCESSES;i++) {
	  for (j=0; j<MAX_PROCESSES;j++) {
	      if (table[i][j] != (i/block_size) + 10) errors++;
	      }
	  }
      if (errors) {
	  /* Print out table if there are any errors */
	  for (i=0; i<MAX_PROCESSES;i++) {
	      printf("\n");
	      for (j=0; j<MAX_PROCESSES; j++)
		  printf("  %d",table[i][j]);
	      }
	  printf("\n");
	  }
    } 

    Test_Waitforall( );
    MPI_Finalize();
    if (errors)
      printf( "[%d] done with ERRORS(%d)!\n", rank, errors );
    return errors;
}
