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
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <mpi.h>

#undef DEBUG
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#endif

int iteration = 0;
int last_iteration = 0;
int interval = 5;

/* getopt related variables */
int ch;
extern char *optarg;
extern int optind;


void timeout(int sig)
{
  if (iteration != last_iteration)
  {
    fprintf(stdout, "mpi_alltoall: %d successfull iterations\n", iteration);
    fflush(stdout);
  }
  last_iteration = iteration;
  alarm(interval);
}


/* Hacked up version of coll13.c from MPICH test suite */

int main( int argc, char *argv[] )
{
    int rank, size;
    int chunk = 1;
    int status, i, j;
    int *sb, *rb, sum, *sums;
    int forever = 0;
    int rc;
    int *temp;
    int flag;
    
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Comm_size(MPI_COMM_WORLD,&size);

    while ((ch= getopt(argc, argv, "fm:")) != EOF)   {
        switch (ch) {
            case 'f':
                forever = 1;
                break;
            case 'm':
                chunk = atoi(optarg);
                break;
            default:
                exit(-1);
        }
    }
    
    if (rank == 0)
      printf("exchanging %d integer(s)\n", chunk);

    if (chunk != 0) {
        sb = malloc(size*chunk*sizeof(int));
        if ( !sb ) {
	    perror( "can't allocate send buffer" );
	    MPI_Abort(MPI_COMM_WORLD,EXIT_FAILURE);
        }
        rb = malloc(size*chunk*sizeof(int));
        if ( !rb ) {
	    perror( "can't allocate recv buffer");
	    free(sb);
	    MPI_Abort(MPI_COMM_WORLD,EXIT_FAILURE);
        }
	printf("allocated 2 %d byte buffers\n", size*chunk*sizeof(int));
    }
    sums = malloc(size*sizeof(int));
    if ( !sums ) {
	perror( "can't allocate sum buffer");
	if (chunk != 0) {
	    free(sb);
	    free(rb);
	}
	MPI_Abort(MPI_COMM_WORLD,EXIT_FAILURE);
    }

    for ( i=0 ; i < size*chunk ; ++i ) {
	sb[i] = rank + 1;
	rb[i] = 0;
    }

    if (rank == 0)
    {
      signal(SIGALRM, timeout);
      alarm(interval);
    }

    do {
  
      status = MPI_Alltoall(sb,chunk,MPI_INT,rb,chunk,MPI_INT,
			    MPI_COMM_WORLD);
      
      for ( i=0, sum=0 ; i < size*chunk ; ++i ) {
	  sum += rb[i];
      }
      
      status = MPI_Gather(&sum, 1, MPI_INT, sums, 1, MPI_INT, 0, 
			  MPI_COMM_WORLD);

#ifdef DEBUG
      for (j=0; j<size; ++j) {
        if (j == rank) {
            for ( i=0 ; i < size*chunk ; ++i ) {
	      printf("[%d] sb[%d]=%d, rb[%d]=%d\n", rank, i, sb[i], i, rb[i]);
            }
            printf("[%d] sum = %d\n", rank, sum);
        }
        MPI_Barrier(MPI_COMM_WORLD);
      }
  
      if (rank == 0) {
        for (i=0; i<size; ++i)
	  printf("sums[%d] = %d\n", i, sums[i]);
      }
#endif

      if (rank == 0) {
        for (i=1, status = 0; i<size; ++i) {
          if (sums[0] != sums[i]) {
	    printf("Checksum ERROR, sums[0] != sums[%d]\n", i);
	    status = 1;
	    forever = 0;
          }
        }
        if (status)
          MPI_Abort(MPI_COMM_WORLD,EXIT_FAILURE);
        else {
	  if (!forever)
            printf("%s: test was successful\n", argv[0]);
	  else {
            ++iteration;
	  }
	}
      }

    } while (forever);

    if (chunk != 0) {
        free(sb);
        free(rb);
    }
    free(sums);

    MPI_Finalize();

    return(EXIT_SUCCESS);
}

