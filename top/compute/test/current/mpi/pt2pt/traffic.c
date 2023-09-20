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
** $Id: traffic.c,v 1.2 2001/03/16 17:40:37 rolf Exp $
** Create lots of traffic among nodes
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "mpi.h"


/* ===++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=== */
/*
** Functions
*/
void *mpi_malloc(size_t sz, int nnodes, int rank);
void disp_progress(int n, int nnodes, int msg_size, time_t start, int *bufs);


/* ===++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=== */
/*
** Constants
*/
#define ITERATIONS	(1000000)
#define MSG_SIZE	(100000 / sizeof(int))


/* ===++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=== */


int
main(int argc, char **argv)
{

int i, n;
int *bufs;
int *bufr;
int *bufptr;
MPI_Request *sreq;
MPI_Request *rreq;
MPI_Status *status;
int msg_size, ntest;
int to;
time_t start_time;
unsigned int seed;
int rank, nnodes;


    MPI_Init (&argc, &argv);
    MPI_Comm_size (MPI_COMM_WORLD, &nnodes);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);

    msg_size= 8000000 - nnodes * sizeof (MPI_Status);
    msg_size -= nnodes * sizeof (MPI_Request);
    msg_size -= nnodes * sizeof (MPI_Request);
    msg_size= msg_size / (sizeof (int) + nnodes * sizeof (int));
    ntest= ITERATIONS;

    if (rank == 0)   {
	printf("\n\nMessage traffic test\n");
	printf("===++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++"
	    "=++=++=++=++===\n");
	printf("    There will be %d iterations\n", ntest);
	printf("    In each iteration each node will send %d integers (%ld "
	    "bytes each)\n", msg_size, sizeof(int));
	printf("    = %ld bytes to each of the %d nodes\n",
	    msg_size * sizeof(int), nnodes);
	printf("    No data checking will be done. The idea is to generate ");
	printf("lots of traffic\n    on the network\n");
    }

    bufs= mpi_malloc(msg_size * sizeof (int), nnodes, rank);
    bufr= mpi_malloc(nnodes * msg_size * sizeof (int), nnodes, rank);
    rreq= mpi_malloc(nnodes * sizeof (MPI_Request), nnodes, rank);
    sreq= mpi_malloc(nnodes * sizeof (MPI_Request), nnodes, rank);
    status= mpi_malloc(nnodes * sizeof (MPI_Status), nnodes, rank);
    if (rank == 0)   {
	int total= 0;
	total += msg_size * sizeof (int);
	total += nnodes * msg_size * sizeof (int);
	total += nnodes * sizeof (MPI_Request);
	total += nnodes * sizeof (MPI_Request);
	total += nnodes * sizeof (MPI_Status);
	printf("    Allocated %d bytes on each node for buffers and status\n",
	    total);
    }


    /* Fill the send buffer.  */
    seed= (int)time(NULL) * (rank + 117);
    srandom(seed);
    for (i = 0; i < msg_size; i++) {
	bufs[i] = (int)random();
    }

    /* Pre-post all receives */
    for (i= 0; i < nnodes; i++)   {
	bufptr= bufr + (i * msg_size);
	MPI_Irecv(bufptr, msg_size, MPI_INT, i, 1001, MPI_COMM_WORLD, &rreq[i]);
    }

    if (rank == 0)   {
	printf("\nElapsed         Aggregate              Aggregate\n");
	printf("Time            data sent              bandwidth\n");
    }
    start_time= time(NULL);
    MPI_Barrier(MPI_COMM_WORLD);


    for (n = 0; n < ntest; n++) {
	if (rank == 0)   {
	    disp_progress(n, nnodes, msg_size, start_time, bufs);
	}

	/* send to everybody */
	for (i= 0; i < nnodes; i++)   {
	    to= (rank + i + n) % nnodes;
	    MPI_Isend(bufs, msg_size, MPI_INT, to, 1001, MPI_COMM_WORLD,
		&sreq[i]);
	}

	/* Wait for the receives to complete */
	MPI_Waitall(nnodes, rreq, status);


	/* Pre-post the receives */
	for (i= 0; i < nnodes; i++)   {
	    bufptr= bufr + (i * msg_size);
	    MPI_Irecv(bufptr, msg_size, MPI_INT, i, 1001, MPI_COMM_WORLD,
		&rreq[i]);
	}


	/* Make sure the sends have completed */
	MPI_Waitall(nnodes, sreq, MPI_STATUSES_IGNORE);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();

    return 0;
}

/* ===++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=== */

void*
mpi_malloc(size_t sz, int nnodes, int rank)
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
	if (rv != NULL)   {
	    free (rv);
	}
	if (rank == 0)   {
	    fprintf(stderr, "Not enough memory to run on %d nodes\n", nnodes);
	}
	exit(-1);
    }

    return rv;
}

/* ===++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=== */

void
disp_progress(int n, int nnodes, int msg_size, time_t start, int *bufs)
{

long long total;
double dtotal;
char *unit;
time_t elapsed;
time_t current_time;
time_t dot_time, line_time;
static time_t last_dot_time= 0;
static time_t last_line_time= 0;
time_t tmp;
int ss, mm, hh;


    current_time= time(NULL);
    dot_time= current_time - last_dot_time;
    line_time= current_time - last_line_time;
    if (line_time >= 60)   {                 
        last_line_time= current_time;
        last_dot_time= current_time;

	/*
	** In n iterations
	** nnodes send a message to nnodes
	** Each message is msg_size integers
	*/
	total= (long long)nnodes * (long long)n * (long long)msg_size *
		    (long long)sizeof(int) * (long long)(nnodes);
	if (total >= 1000000000000000)   {
	    unit= "Peta Bytes";
	    dtotal= total / 1000000000000000.0;
	} else if (total >= 1000000000000)   {
	    unit= "Tera Bytes";
	    dtotal= total / 1000000000000.0;
	} else if (total >= 1000000000)   {
	    unit= "Giga Bytes";
	    dtotal= total / 1000000000.0;
	} else if (total >= 1000000)   {
	    unit= "Mega Bytes";
	    dtotal= total / 1000000.0;
	} else if (total >= 1000)   {
	    unit= "Kilo Bytes";
	    dtotal= total / 1000.0;
	} else   {
	    unit= "Bytes";
	    dtotal= total;
	}

	elapsed= time(NULL) - start;
	tmp= elapsed;
	hh= tmp / (60 * 60);
	tmp= tmp - (hh * 60 * 60);
	mm= tmp / 60;
	tmp= tmp - (mm * 60);
	ss= tmp;
	printf("\n%3d:%02d:%02d %12.3f %-11s ", hh, mm, ss, dtotal, unit);
	if (elapsed > 0.0)   {
	    printf("%10.3f MB/s ", (total / 1000000.0) / elapsed);
	} else   {
	    printf("%10.3f MB/s ", 0.0);
	}
	fflush(stdout);

    } else if (dot_time >= 2)   {
        last_dot_time= current_time;
	printf(".");
	fflush(stdout);
    }

}  /* end of disp_progress() */

/* ===++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=== */

