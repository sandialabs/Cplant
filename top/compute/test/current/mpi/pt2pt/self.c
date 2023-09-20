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
 *  $Id: self.c,v 1.1 1997/10/29 22:45:14 bright Exp $
 *
 *  (C) 1993 by Argonne National Laboratory and Mississippi State University.
 *
 */
 

#include "mpi.h"
#include <stdio.h>
int main( argc, argv )
int argc;
char **argv;
{
int           sendbuf[10];
int           sendcount = 10;
int           recvbuf[10];
int           recvcount = 10;
int           source = 0, recvtag = 2;
int           dest = 0, sendtag = 2;

    int               mpi_errno = MPI_SUCCESS;
    MPI_Status        status_array[2];
    MPI_Request       req[2];

    MPI_Init( &argc, &argv );
    if (mpi_errno = MPI_Irecv ( recvbuf, recvcount, MPI_INT,
			    source, recvtag, MPI_COMM_WORLD, &req[1] )) 
	return mpi_errno;
    if (mpi_errno = MPI_Isend ( sendbuf, sendcount, MPI_INT, dest,   
			    sendtag, MPI_COMM_WORLD, &req[0] )) 
	return mpi_errno;

    fprintf( stdout, "[%d] Starting waitall\n", 0 );
    mpi_errno = MPI_Waitall ( 2, req, status_array );
    fprintf( stdout, "[%d] Ending waitall\n", 0 );

    MPI_Finalize();
    return (mpi_errno);
}
