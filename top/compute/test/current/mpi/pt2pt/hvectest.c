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
 *  $Id: hvectest.c,v 1.1 1997/10/29 22:44:54 bright Exp $
 *
 *  (C) 1993 by Argonne National Laboratory and Mississippi State University.
 *
 */
 

/*
    hvectest - test program that sends an array of floats from the first 
             process of a group to the last, using send and recv and the
	     vector datatype.
*/

#include <stdio.h>
#include "mpi.h"
/* #define SHOWMSG */

int main( argc, argv )
int argc;
char **argv;
{
    int rank, size, to, from, tag, count, np, i;
    int src, dest;
    int st_source, st_tag, st_count;
    int errcnt = 0;
    MPI_Request handle;
    MPI_Status status;
    double data[100];
    MPI_Datatype rowtype;

    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    MPI_Comm_size( MPI_COMM_WORLD, &size );

    src  = size - 1;
    dest = 0;
/*
    dest  = size - 1;
    src = 0;
 */
    MPI_Type_vector( 10, 1, 10, MPI_DOUBLE, &rowtype );
    MPI_Type_commit( &rowtype );
    /* First test: send a row */
    if (rank == src)
    {
	to     = dest;
	count  = 1;
	tag    = 2001;
	for (i = 0; i < 100; i++)
	    data[i] = i;
	/* Send a row */
	MPI_Send( data, count, rowtype, to, tag, MPI_COMM_WORLD );
#ifdef SHOWMSG
	printf("%d sent", rank );
	for (i = 0; i < 10; i++) printf(" %f",data[i*10]);printf("\n");
#endif
    }

    if (rank == dest)
    {
	tag   = MPI_ANY_TAG;
	count = 10;		
	from  = MPI_ANY_SOURCE;
	MPI_Recv(data, count, MPI_DOUBLE, from, tag, MPI_COMM_WORLD,
		 &status ); 

	st_source = status.MPI_SOURCE;
	st_tag    = status.MPI_TAG;
	MPI_Get_count( &status, MPI_DOUBLE, &st_count );

	printf( "Status info: source = %d, tag = %d, count = %d\n",
	        st_source, st_tag, st_count );
#ifdef SHOWMSG
	printf( "%d received", rank);
	for (i = 0; i < 10; i++) printf(" %f",data[i]);	printf("\n");
#endif
	for (i = 0; i < 10; i++) if (data[i] != 10*i) { 
	    errcnt++;
	    fprintf( stderr, 
		    "[%d](rcv double) %d'th element = %f, should be %f\n",
		     rank, i, data[i], 10.0*i );
	    }
    }

    /* Second test: receive a column into row */
    if (rank == src)
    {
	to     = dest;
	count  = 10;
	tag    = 2001;
	for (i = 0; i < 100; i++)
	    data[i] = i;
	/* Send a row */
	MPI_Send( data, count, MPI_DOUBLE, to, tag, MPI_COMM_WORLD );
#ifdef SHOWMSG
	printf("%d sent", rank );
	for (i = 0; i < 10; i++) printf(" %f",data[i]);printf("\n");
#endif
    }
    if (rank == dest)
    {
	tag   = MPI_ANY_TAG;
	count = 1;		
	from  = MPI_ANY_SOURCE;
	MPI_Recv(data, count, rowtype, from, tag, MPI_COMM_WORLD,
		 &status ); 

	st_source = status.MPI_SOURCE;
	st_tag    = status.MPI_TAG;
	MPI_Get_count( &status, MPI_DOUBLE, &st_count );

	printf( "Status info: source = %d, tag = %d, count = %d\n",
	        st_source, st_tag, st_count );
#ifdef SHOWMSG
	printf( "%d received", rank);
	for (i = 0; i < 10; i++) printf(" %f",data[i*10]);printf("\n");
#endif
	for (i = 0; i < 10; i++) if (data[i*10] != i) {
	    errcnt++;
	    fprintf( stderr, 
		    "[%d](rcv row) %d'th element = %f, should be %f\n",
		     rank, i, data[i*10], 1.0*i );
	    }
    }

    /* Third test: send AND receive a row */
    if (rank == src)
    {
	to     = dest;
	count  = 1;
	tag    = 2001;
	for (i = 0; i < 100; i++)
	    data[i] = i;
	/* Send a row */
	MPI_Send( data, count, rowtype, to, tag, MPI_COMM_WORLD );
#ifdef SHOWMSG
	printf("%d sent", rank );
	for (i = 0; i < 10; i++) printf(" %f",data[i*10]);printf("\n");
#endif
    }
    if (rank == dest)
    {
	tag   = MPI_ANY_TAG;
	count = 1;		
	from  = MPI_ANY_SOURCE;
	MPI_Recv(data, count, rowtype, from, tag, MPI_COMM_WORLD,
		 &status ); 

	st_source = status.MPI_SOURCE;
	st_tag    = status.MPI_TAG;
	MPI_Get_count( &status, MPI_DOUBLE, &st_count );

	printf( "Status info: source = %d, tag = %d, count = %d\n",
	        st_source, st_tag, st_count );
#ifdef SHOWMSG
	printf( "%d received", rank);
	for (i = 0; i < 10; i++) printf(" %f",data[i*10]);printf("\n");
#endif
	for (i = 0; i < 10; i++) if (data[i*10] != i*10) {
	    errcnt++;
	    fprintf( stderr, 
		    "[%d](rcv row-row) %d'th element = %f, should be %f\n",
		     rank, i, data[i*10], 10.0*i );
	    }
    }

    /* Second Set of Tests: Use Isend and Irecv instead of Send and Recv */
    /* First test: send a row */
    if (rank == src)
    {
	to     = dest;
	count  = 1;
	tag    = 2001;
	for (i = 0; i < 100; i++)
	    data[i] = i;
	/* Send a row */
	MPI_Isend( data, count, rowtype, to, tag, MPI_COMM_WORLD, &handle );
	MPI_Wait( &handle, &status );
#ifdef SHOWMSG
	printf("%d sent", rank );
	for (i = 0; i < 10; i++) printf(" %f",data[i*10]);printf("\n");
#endif
    }

    if (rank == dest)
    {
	tag   = MPI_ANY_TAG;
	count = 10;		
	from  = MPI_ANY_SOURCE;
	MPI_Irecv(data, count, MPI_DOUBLE, from, tag, MPI_COMM_WORLD,
		 &handle ); 
	MPI_Wait( &handle, &status );

	st_source = status.MPI_SOURCE;
	st_tag    = status.MPI_TAG;
	MPI_Get_count( &status, MPI_DOUBLE, &st_count );

	printf( "Status info: source = %d, tag = %d, count = %d\n",
	        st_source, st_tag, st_count );
#ifdef SHOWMSG
	printf( "%d received", rank);
	for (i = 0; i < 10; i++) printf(" %f",data[i]);	printf("\n");
#endif
	for (i = 0; i < 10; i++) if (data[i] != 10*i) {
	    errcnt++;
	    fprintf( stderr, 
		    "[%d](ircv double) %d'th element = %f, should be %f\n",
		     rank, i, data[i], 10.0*i );
	    }
    }

    /* Second test: receive a column into row */
    if (rank == src)
    {
	to     = dest;
	count  = 10;
	tag    = 2001;
	for (i = 0; i < 100; i++)
	    data[i] = i;
	/* Send a row */
	MPI_Isend( data, count, MPI_DOUBLE, to, tag, MPI_COMM_WORLD, 
		   &handle );
	MPI_Wait( &handle, &status );
#ifdef SHOWMSG
	printf("%d sent", rank );
	for (i = 0; i < 10; i++) printf(" %f",data[i]);printf("\n");
#endif
    }
    if (rank == dest)
    {
	tag   = MPI_ANY_TAG;
	count = 1;		
	from  = MPI_ANY_SOURCE;
	MPI_Irecv(data, count, rowtype, from, tag, MPI_COMM_WORLD,
		 &handle ); 
	MPI_Wait( &handle, &status );

	st_source = status.MPI_SOURCE;
	st_tag    = status.MPI_TAG;
	MPI_Get_count( &status, MPI_DOUBLE, &st_count );

	printf( "Status info: source = %d, tag = %d, count = %d\n",
	        st_source, st_tag, st_count );
#ifdef SHOWMSG
	printf( "%d received", rank);
	for (i = 0; i < 10; i++) printf(" %f",data[i*10]);printf("\n");
#endif
	for (i = 0; i < 10; i++) if (data[i*10] != i) {
	    errcnt++;
	    fprintf( stderr, 
		    "[%d](ircv row) %d'th element = %f, should be %f\n",
		     rank, i, data[i*10], 1.0*i );
	    }
    }

    /* Third test: send AND receive a row */
    if (rank == src)
    {
	to     = dest;
	count  = 1;
	tag    = 2001;
	for (i = 0; i < 100; i++)
	    data[i] = i;
	/* Send a row */
	MPI_Isend( data, count, rowtype, to, tag, MPI_COMM_WORLD, &handle );
	MPI_Wait( &handle, &status );
#ifdef SHOWMSG
	printf("%d sent", rank );
	for (i = 0; i < 10; i++) printf(" %f",data[i*10]);printf("\n");
#endif
    }
    if (rank == dest)
    {
	tag   = MPI_ANY_TAG;
	count = 1;		
	from  = MPI_ANY_SOURCE;
	MPI_Irecv(data, count, rowtype, from, tag, MPI_COMM_WORLD,
		 &handle ); 
	MPI_Wait( &handle, &status );

	st_source = status.MPI_SOURCE;
	st_tag    = status.MPI_TAG;
	MPI_Get_count( &status, MPI_DOUBLE, &st_count );

	printf( "Status info: source = %d, tag = %d, count = %d\n",
	        st_source, st_tag, st_count );
#ifdef SHOWMSG
	printf( "%d received", rank);
	for (i = 0; i < 10; i++) printf(" %f",data[i*10]);printf("\n");
#endif
	for (i = 0; i < 10; i++) if (data[i*10] != i*10) {
	    errcnt++;
	    fprintf( stderr, 
		    "[%d](ircv row-row) %d'th element = %f, should be %f\n",
		     rank, i, data[i*10], 10.0*i );
	    }
    }

    i = errcnt;
    MPI_Allreduce( &i, &errcnt, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD );
    if (errcnt > 0) {
	printf( "Found %d errors in the run \n", errcnt );
	}
    MPI_Type_free( &rowtype );
    Test_Waitforall( );
    MPI_Finalize();
    return 0;
}
