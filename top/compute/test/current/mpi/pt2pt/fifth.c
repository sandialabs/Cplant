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
 *  $Id: fifth.c,v 1.1 1997/10/29 22:44:46 bright Exp $
 *
 *  (C) 1993 by Argonne National Laboratory and Mississippi State University.
 *
 */
 


#include "mpi.h"

int main( argc, argv )
int argc;
char **argv;
{
    int rank, np, data = 777;
    int st_source, st_tag, st_count;
    MPI_Request handle[4];
    MPI_Status status[4];

    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    MPI_Comm_size( MPI_COMM_WORLD, &np );

    if (np < 4) {
      MPI_Finalize();
      printf( "4 processors or more required, %d done\n", rank );
      return(1);
    }

    if (rank == 0) {
      MPI_Isend( &data, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &handle[0] );
      MPI_Irecv( &data, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &handle[1] );
      MPI_Isend( &data, 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &handle[2] );
      MPI_Irecv( &data, 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &handle[3] );
      MPI_Waitall ( 4, handle, status );
    }
    else if (rank == 1) {
      MPI_Irecv( &data, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &handle[0] );
      MPI_Isend( &data, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &handle[1] );
      MPI_Isend( &data, 1, MPI_INT, 3, 0, MPI_COMM_WORLD, &handle[2] );
      MPI_Irecv( &data, 1, MPI_INT, 3, 0, MPI_COMM_WORLD, &handle[3] );
      MPI_Waitall ( 4, handle, status );
    }
    else if (rank == 2) {
      MPI_Isend( &data, 1, MPI_INT, 3, 0, MPI_COMM_WORLD, &handle[0] );
      MPI_Irecv( &data, 1, MPI_INT, 3, 0, MPI_COMM_WORLD, &handle[1] );
      MPI_Irecv( &data, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &handle[2] );
      MPI_Isend( &data, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &handle[3] );
      MPI_Waitall ( 4, handle, status );
    }
    else if (rank == 3) {
      MPI_Irecv( &data, 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &handle[0] );
      MPI_Isend( &data, 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &handle[1] );
      MPI_Irecv( &data, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &handle[2] );
      MPI_Isend( &data, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &handle[3] );
      MPI_Waitall ( 4, handle, status );
    }
    Test_Waitforall( );
    MPI_Finalize();
    return(0);
}


