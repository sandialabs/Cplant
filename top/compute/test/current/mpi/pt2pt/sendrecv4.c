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
 *  $Id: sendrecv4.c,v 1.1 1997/10/29 22:45:23 bright Exp $
 *
 *  (C) 1993 by Argonne National Laboratory and Mississippi State University.
 *
 */
 

#include <mpi.h>
#include <stdio.h>

/*
   This program is from mpich/tsuite/pt2pt and should be changed there only.
   It needs gcomm and dtype from mpich/tsuite, and can be run with 
   any number of processes > 1.

   This version sends and receives EVERYTHING from MPI_BOTTOM, by putting
   the data into a structure.
 */
int main(argc, argv)
int argc;
char **argv;
{
MPI_Datatype *types;
void         **inbufs, **outbufs;
char         **names;
int          *counts, *bytesize, ntype;
MPI_Comm     comms[20];
int          ncomm = 20, rank, np, partner, tag, count, source, size;
int          i, j, k, err, world_rank, errloc;
MPI_Status   status;
char         *obuf;
MPI_Datatype offsettype;
int          blen;
MPI_Aint     displ, extent;

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
    for (j=0; j<ntype; j++) {
	if (world_rank == 0) 
	    fprintf( stdout, "Testing type %s\n", names[j] );
        if (rank == 0) {
	    MPI_Address( inbufs[j], &displ );
	    blen = 1;
	    MPI_Type_struct( 1, &blen, &displ, types + j, &offsettype );
	    MPI_Type_commit( &offsettype );
	    /* Warning: if the type has an explicit MPI_UB, then using a
	       simple shift of the offset won't work.  For now, we skip
	       types whose extents are negative; the correct solution is
	       to add, where required, an explicit MPI_UB */
	    MPI_Type_extent( offsettype, &extent );
	    if (extent < 0) {
		if (world_rank == 0) 
		    fprintf( stdout, 
			"... skipping (appears to have explicit MPI_UB\n" );
		MPI_Type_free( &offsettype );
		continue;
		}
	    partner = np - 1;
#if 0
		MPIR_PrintDatatypePack( stdout, counts[j], offsettype, 
					  0, 0 );
#endif
            MPI_Send( MPI_BOTTOM, counts[j], offsettype, partner, tag, 
		      comms[i] );
	    MPI_Type_free( &offsettype );
            }
        else if (rank == np-1) {
	    partner = 0;
	    obuf = outbufs[j];
	    for (k=0; k<bytesize[j]; k++) 
		obuf[k] = 0;
	    MPI_Address( outbufs[j], &displ );
	    blen = 1;
	    MPI_Type_struct( 1, &blen, &displ, types + j, &offsettype );
	    MPI_Type_commit( &offsettype );
	    /* Warning: if the type has an explicit MPI_UB, then using a
	       simple shift of the offset won't work.  For now, we skip
	       types whose extents are negative; the correct solution is
	       to add, where required, an explicit MPI_UB */
	    MPI_Type_extent( offsettype, &extent );
	    if (extent < 0) {
		MPI_Type_free( &offsettype );
		continue;
		}
            MPI_Recv( MPI_BOTTOM, counts[j], offsettype, 
		     partner, tag, comms[i], &status );
            /* Test correct */
            MPI_Get_count( &status, types[j], &count );
            if (count != counts[j]) {
		fprintf( stderr, 
			"Error in counts (got %d expected %d) with type %s\n",
			 count, counts[j], names[j] );
                err++;
                }
            if (status.MPI_SOURCE != partner) {
		fprintf( stderr, 
			"Error in source (got %d expected %d) with type %s\n",
			 status.MPI_SOURCE, partner, names[j] );
                err++;
                }
            if (errloc = CheckData( inbufs[j], outbufs[j], bytesize[j] )) {
		fprintf( stderr, 
                  "Error in data with type %s (type %d on %d) at byte %d\n", 
			 names[j], j, world_rank, errloc - 1 );
                err++;
#if 0
		MPIR_PrintDatatypeUnpack( stdout, counts[j], offsettype, 
					  0, 0 );
#endif
                }
	    MPI_Type_free( &offsettype );
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
