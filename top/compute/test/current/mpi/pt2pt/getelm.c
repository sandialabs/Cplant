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
 *  $Id: getelm.c,v 1.1 1997/10/29 22:44:52 bright Exp $
 *
 *  (C) 1993 by Argonne National Laboratory and Mississippi State University.
 *
 */
 

/*
 * This is a test of getting the number of basic elements
 */

#include "mpi.h"
#include <stdio.h>

typedef struct { 
    int len;
    double data[1000];
    } buf_t;

int main( argc, argv )
int  argc;
char **argv;
{
int err = 0, toterr;
MPI_Datatype contig1, varstruct1, oldtypes[2], varstruct2;
MPI_Aint     displs[2];
int          blens[2];
MPI_Comm     comm;
MPI_Status   status;
int          world_rank;
int          rank, size, partner, count, i;
int          send_ibuf[4], recv_ibuf[4];
buf_t        send_buf, recv_buf;

MPI_Init( &argc, &argv );
MPI_Comm_rank( MPI_COMM_WORLD, &world_rank );

/* Form the datatypes */
MPI_Type_contiguous( 4, MPI_INT, &contig1 );
MPI_Type_commit( &contig1 );
blens[0] = 1;
blens[1] = 1000;
oldtypes[0] = MPI_INT;
oldtypes[1] = MPI_DOUBLE;
/* Note that the displacement for the data is probably double aligned */
MPI_Address( &send_buf.len, &displs[0] );
MPI_Address( &send_buf.data[0], &displs[1] );
/* Make relative */
displs[1] = displs[1] - displs[0];
displs[0] = 0;
MPI_Type_struct( 2, blens, displs, oldtypes, &varstruct1 );
MPI_Type_commit( &varstruct1 );

comm = MPI_COMM_WORLD;

MPI_Comm_size( comm, &size );
MPI_Comm_rank( comm, &rank );

if (size < 2) {
    fprintf( stderr, "This test requires at least 2 processes\n" );
    MPI_Abort( MPI_COMM_WORLD, 1 );
    }

if (rank == size - 1) {
    partner = 0;
    /* Send contiguous data */
    for (i=0; i<4; i++) 
	send_ibuf[i] = i;
    MPI_Send( send_ibuf, 1, contig1, partner, 0, comm );

    /* Send partial structure */
    blens[1] = 23;
    MPI_Type_struct( 2, blens, displs, oldtypes, &varstruct2 );
    MPI_Type_commit( &varstruct2 );

    MPI_Send( &send_buf, 1, varstruct2, partner, 1, comm );
    MPI_Type_free( &varstruct2 );
    }
else if (rank == 0) {
    partner = size - 1;
    MPI_Recv( recv_ibuf, 1, contig1, partner, 0, comm, &status );
    MPI_Get_count( &status, MPI_INT, &count );
    if (count != 4) {
	err++;
	fprintf( stderr, 
		 "Wrong count for contig recv MPI_INT; got %d expected %d\n",
		 count, 4 );
	}
    MPI_Get_count( &status, contig1, &count );
    if (count != 1) {
	err++;
	fprintf( stderr, 
		"Wrong count for contig recv (contig); got %d expected %d\n",
		 count, 1 );
	}
    MPI_Get_elements( &status, contig1, &count );
    if (count != 4) {
	err++;
	fprintf( stderr, 
		 "Wrong elements for contig recv contig; got %d expected %d\n",
		 count, 4 );
	}

    /* Now, try the partial structure */
    MPI_Recv( &recv_buf, 1, varstruct1, partner, 1, comm, &status );
    MPI_Get_elements( &status, varstruct1, &count );
    if (count != 24) {
	err++;
	fprintf( stderr, 
  	    "Wrong number of elements for struct recv; got %d expected %d\n", 
		count, 24 );
	}
    }

MPI_Type_free( &contig1 );
MPI_Type_free( &varstruct1 );

MPI_Allreduce( &err, &toterr, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD );
if (world_rank == 0) {
    if (toterr == 0) 
	printf( "No errors in MPI_Get_elements\n" );
    else
	printf( "Found %d errors in MPI_Get_elements\n", toterr );
    }
MPI_Finalize( );
return toterr;
}
