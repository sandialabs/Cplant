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

/* 
From: hook@nas.nasa.gov (Edward C. Hook)
 */

#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include <errno.h>
#ifdef __STDC__
#define WHY strerror(errno)
#else
extern char *sys_errlist[];
#define WHY sys_errlist[errno]
#endif
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#endif

int main( argc, argv )
int argc;
char *argv[];
{
	int rank, size;
	int chunk = 4096;
	int i;
	char *sb;
	char *rb;
	int status, gstatus;

	MPI_Init(&argc,&argv);
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	MPI_Comm_size(MPI_COMM_WORLD,&size);

	for ( i=1 ; i < argc ; ++i ) {
		if ( argv[i][0] != '-' )
			continue;
		switch(argv[i][1]) {
			case 'm':
				chunk = atoi(argv[++i]);
				break;
			default:
				fprintf(stderr,"Unrecognized argument %s\n",
					argv[i]);
				MPI_Abort(MPI_COMM_WORLD,EXIT_FAILURE);
		}
	}

	sb = (char *)malloc(size*chunk*sizeof(int));
	if ( !sb ) {
		fprintf(stderr,"can't allocate send buffer: %s\n",WHY);
		MPI_Abort(MPI_COMM_WORLD,EXIT_FAILURE);
	}
	rb = (char *)malloc(size*chunk*sizeof(int));
	if ( !rb ) {
		free(sb);
		fprintf(stderr,"can't allocate recv buffer: %s\n",WHY);
		MPI_Abort(MPI_COMM_WORLD,EXIT_FAILURE);
	}
	for ( i=0 ; i < size*chunk ; ++i ) {
		sb[i] = rank + 1;
		rb[i] = 0;
	}

	/* fputs("Before MPI_Alltoall\n",stdout); */

	status = MPI_Alltoall(sb,chunk,MPI_INT,rb,chunk,MPI_INT,
			      MPI_COMM_WORLD);

	/* fputs("Before MPI_Allreduce\n",stdout); */
	MPI_Allreduce( &status, &gstatus, 1, MPI_INT, MPI_SUM, 
		       MPI_COMM_WORLD );

	/* fputs("After MPI_Allreduce\n",stdout); */
	if (rank == 0)
	    printf("all_to_all returned %d\n",gstatus);

	free(sb);
	free(rb);

	MPI_Finalize();

	return(EXIT_SUCCESS);
}

