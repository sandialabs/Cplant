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
 *  $Id: testall.c,v 1.1 1997/10/29 22:45:33 bright Exp $
 *
 *  (C) 1993 by Argonne National Laboratory and Mississippi State University.
 *
 */
 

#include "mpi.h"
#include <stdio.h>
#include <memory.h>

/* 
   Multiple completions
   
   This is similar to a test in allpair.f, but with an expanded range of
   datatypes and communicators.
 */

int main( argc, argv )
int  argc;
char **argv;
{
MPI_Datatype *types;
void         **inbufs, **outbufs;
char         **names;
int          *counts, *bytesize, ntype;
MPI_Comm     comms[20];
int          ncomm = 20, rank, np, partner, tag, count, source, size;
int          i, j, k, err, world_rank, errloc;
MPI_Status   status, statuses[2];
int          flag, index, outcount, indices[2];
char         *obuf;
MPI_Request  requests[2];


MPI_Init( &argc, &argv );

AllocateForData( &types, &inbufs, &outbufs, &counts, &bytesize, 
		 &names, &ntype );
GenerateData( types, inbufs, outbufs, counts, bytesize, names, &ntype );

MPI_Comm_rank( MPI_COMM_WORLD, &world_rank );
MakeComms( comms, 20, &ncomm, 0 );

/* Test over a wide range of datatypes and communicators */
err = 0;
for (i=0; i<ncomm; i++) {
    MPI_Comm_rank( comms[i], &rank );
    MPI_Comm_size( comms[i], &np );
    if (np < 2) continue;
    tag = i;
    /* This is the test.  
       master:                               worker:
       irecv                                 send
       isend
       testall  (fail)
       sendrecv                              sendrecv
                                             irecv
       sendrecv                              sendrecv
       testall  (should succeed)             wait
     */
    for (j=0; j<ntype; j++) {
	if (world_rank == 0) 
	    fprintf( stdout, "Testing type %s\n", names[j] );
	/* This test does an irsend between both partners, with 
	   a sendrecv after the irecv used to guarentee that the
	   irsend has a matching receive
	 */
        if (rank == 0) {
	    /* Master */
	    partner = np - 1;
#if 0
	    MPIR_PrintDatatypePack( stdout, counts[j], types[j], 0, 0 );
#endif
	    obuf = outbufs[j];
	    for (k=0; k<bytesize[j]; k++) 
		obuf[k] = 0;
	    
	    MPI_Irecv(outbufs[j], counts[j], types[j], partner, tag, 
		      comms[i], &requests[0] );
	    
	    /* Use issend to ensure that the test cannot complete */
	    MPI_Issend( inbufs[j], counts[j], types[j], partner, tag, 
		        comms[i], &requests[1] );

	    /* Note that the send may have completed */
	    MPI_Testall( 2, &requests[0], &flag, statuses );
	    if (flag) {
		err++;
		fprintf( stderr, "MPI_Testall returned flag == true!\n" );
		}
	    if (!requests[1]) {
		err++;
		fprintf( stderr, "MPI_Testall freed a request\n" );
		}
	    MPI_Sendrecv( MPI_BOTTOM, 0, MPI_INT, partner, ncomm+i, 
			  MPI_BOTTOM, 0, MPI_INT, partner, ncomm+i, 
			  comms[i], &status );
	    MPI_Sendrecv( MPI_BOTTOM, 0, MPI_INT, partner, ncomm+i, 
			  MPI_BOTTOM, 0, MPI_INT, partner, ncomm+i, 
			  comms[i], &status );
	    MPI_Testall( 2, requests, &flag, statuses );
	    if (!flag) {
		err++;
		fprintf( stderr, "MPI_Testall returned flag == false!\n" );
		}
	    if (requests[0] || requests[1]) {
		err++;
		fprintf( stderr, "MPI_Testall failed to free requests\n" );
		}
	    /* Check the received data */
            if (CheckDataAndPrint( inbufs[j], outbufs[j], bytesize[j],
				   names[j], j )) {
		err++;
		}
	    }
	else if (rank == np - 1) {
	    /* receiver */
	    partner = 0;
	    obuf = outbufs[j];
	    for (k=0; k<bytesize[j]; k++) 
		obuf[k] = 0;
	    
	    MPI_Send( inbufs[j], counts[j], types[j], partner, tag, 
		        comms[i] );
	    
	    MPI_Sendrecv( MPI_BOTTOM, 0, MPI_INT, partner, ncomm+i, 
			  MPI_BOTTOM, 0, MPI_INT, partner, ncomm+i, 
			  comms[i], &status );

	    MPI_Irecv(outbufs[j], counts[j], types[j], partner, tag, 
		      comms[i], &requests[0] );

	    MPI_Sendrecv( MPI_BOTTOM, 0, MPI_INT, partner, ncomm+i, 
			  MPI_BOTTOM, 0, MPI_INT, partner, ncomm+i, 
			  comms[i], &status );

	    MPI_Wait( requests, statuses );
            if (CheckDataAndPrint( inbufs[j], outbufs[j], bytesize[j],
				   names[j], j )) {
                err++;
		}
	    }
	}
    }

if (err > 0) {
    fprintf( stderr, "%d errors on %d\n", err, rank );
    }
FreeDatatypes( types, inbufs, outbufs, counts, bytesize, names, ntype );
FreeComms( comms, ncomm );
MPI_Finalize();

return err;
}
