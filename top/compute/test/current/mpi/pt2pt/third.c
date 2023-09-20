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
 *  $Id: third.c,v 1.1 1997/10/29 22:45:39 bright Exp $
 *
 *  (C) 1993 by Argonne National Laboratory and Mississippi State University.
 *
 */
 

/* @
    third - test program that tests queueing by sending messages with various
            tags, receiving them in particular order.
@ */

#include <string.h>
#include "mpi.h"

/* Define VERBOSE to get printed output */
int main( argc, argv )
int argc;
char **argv;
{
    int rank, size, to, from, tag, count, np, i;
    int src, dest, waiter;
    int st_count;
#ifdef VERBOSE
    int st_source, st_tag;
#endif
    MPI_Request handle;
    MPI_Status status;
    char data[100];
    MPI_Request rq[2];
    MPI_Status statuses[2];

    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    MPI_Comm_size( MPI_COMM_WORLD, &size );
/*
    src  = size - 1;
    dest = 0;
 */
    src = 0;
    dest = size - 1;
    /* waiter = dest; */  	/* Receiver delays, so msgs unexpected */
    /* waiter = src;  */  	/* Sender delays, so recvs posted      */
    waiter = 10000;		/* nobody waits */

    if (rank == src)
    {
	if (waiter == src)
	    sleep(10);
	to     = dest;
	tag    = 2001;
	sprintf(data,"First message, type 2001");
	count = strlen(data) + 1;
	MPI_Isend( data, count, MPI_CHAR, to, tag, MPI_COMM_WORLD, &rq[0] );
#ifdef VERBOSE	
	printf("%d sent :%s:\n", rank, data );
#endif
	tag    = 2002;
	sprintf(data,"Second message, type 2002");
	count = strlen(data) + 1;
	MPI_Isend( data, count, MPI_CHAR, to, tag, MPI_COMM_WORLD, &rq[1] );
	MPI_Waitall( 2, rq, statuses );
#ifdef VERBOSE	
	printf("%d sent :%s:\n", rank, data );
#endif
    }
    else
    if (rank == dest)
    {
	if (waiter == dest)
	    sleep(10);
	from  = MPI_ANY_SOURCE;
	count = 100;		

	tag   = 2002;
	MPI_Recv(data, count, MPI_CHAR, from, tag, MPI_COMM_WORLD, &status ); 

	MPI_Get_count( &status, MPI_CHAR, &st_count );
	if (st_count != strlen("Second message, type 2002") + 1) {
	    printf( "Received wrong length!\n" );
	    }
#ifdef VERBOSE	
	st_source = status.MPI_SOURCE;
	st_tag    = status.MPI_TAG;
	printf( "Status info: source = %d, tag = %d, count = %d\n",
	        st_source, st_tag, st_count );
	printf( "%d received :%s:\n", rank, data);
#endif
	tag   = 2001;
	MPI_Recv(data, count, MPI_CHAR, from, tag, MPI_COMM_WORLD, &status ); 

	MPI_Get_count( &status, MPI_CHAR, &st_count );
	if (st_count != strlen("First message, type 2001") + 1) {
	    printf( "Received wrong length!\n" );
	    }
#ifdef VERBOSE	
	st_source = status.MPI_SOURCE;
	st_tag    = status.MPI_TAG;\
	printf( "Status info: source = %d, tag = %d, count = %d\n",
	        st_source, st_tag, st_count );
	printf( "%d received :%s:\n", rank, data);
#endif
    }
#ifdef VERBOSE	
    printf( "Process %d exiting\n", rank );
#endif
    Test_Waitforall( );
    MPI_Finalize();
    return 0;
}
