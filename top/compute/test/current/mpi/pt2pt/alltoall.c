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
** $Id: alltoall.c,v 1.4 2000/12/14 22:58:57 rolf Exp $
** Simple all-all exchange test created by Mark Sears
** modified by Matt Burke to use MPI instead of ml
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mpi.h"

void *mpi_malloc (size_t sz);

int
main(int argc, char **argv)
{
    int rank,nnodes;
    int i, j, q, qp;
    int ntest, ltest;
    int *bufs;
    int *bufr;
    MPI_Request *req;
    MPI_Request rreq;
    MPI_Status status;

    double t0, td;

    MPI_Init (&argc, &argv);

    MPI_Comm_size (MPI_COMM_WORLD, &nnodes);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);

    ntest = 10;
    ltest = 10000;
    bufs = mpi_malloc (ltest * sizeof (int));
    bufr = mpi_malloc (ltest * sizeof (int));
    req = mpi_malloc (nnodes * sizeof (MPI_Request));

    MPI_Barrier (MPI_COMM_WORLD);
    t0 = MPI_Wtime ();

    for (j = 0; j < ltest; j++) {
	bufs[j] = 123 + (123 * rank) + (123 * nnodes * j);
    }

    td = 0.;
    for (i = 0; i < ntest; i++) {
	for (q = 0; q < nnodes; q++) {
	    int chk;

	    qp = (q - rank + nnodes) % nnodes;
	    if (qp == rank) continue;

	    MPI_Irecv(bufr, ltest, MPI_INT, qp, 1001, MPI_COMM_WORLD, &rreq);
	    MPI_Isend (bufs, ltest, MPI_INT, qp, 1001, MPI_COMM_WORLD, &req[q]);
	    MPI_Wait(&rreq, &status);

	    chk = 0;
	    for (j = 0; j < ltest; j++) {
		if (bufr[j] != (123 + (123 * qp) + (123 * nnodes * j))) {
		    fprintf (stderr, "Node %3d got bad data from node %3d. Got "
			"%d 0x%08x expected 0x%08x\n", rank, qp, bufr[j],
			bufr[j], 123 + (123 * qp) + (123 * nnodes * j));
		    chk++;
		}
	    }

            if(chk) {
		fprintf (stderr,
		    "failed exch between %d and %d (%d errs) test %d\n",
		    rank, qp, chk, i);
	    }

            j = ltest * sizeof (int) * 2;
            td += (double) j;
	}
	MPI_Barrier (MPI_COMM_WORLD);
	if (rank == 0) {
	    printf ("test %d done\n", i);
	}

    }

    MPI_Barrier (MPI_COMM_WORLD);
    t0 = MPI_Wtime () - t0;
    if (rank == 0) {
	/* printf ("time %le rate %le\n", t0, td / t0); */
	printf ("time %e rate %e\n", t0, td / t0);
    }

    MPI_Finalize ();
    exit (0);

    return 0;
}

void*
mpi_malloc (size_t sz)
{
    void *rv;
    int ok;
    int allok;

    /* Initialize these two values */
    ok = 1;
    rv = NULL;

    /* Attempt to allocate memory if a positive size was given */
    if (sz > 0) {
	rv = malloc (sz);
	if (rv == NULL) {
	    /* Record that the malloc failed */
	    ok = -1;
	}
    }

    /* Check how all the malloc's did */
    MPI_Allreduce (&ok, &allok, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);

    if (allok < 0) {
	/* Somebody's malloc failed... exit */
	if (rv != NULL)
	    free (rv);
	exit (100);
    }

    return rv;
}
