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
** $Id: integrity.c,v 1.6 2001/03/16 06:32:43 rolf Exp $
** Send random data and fixed patterns to other nodes and checksum or
** fully check everything. On fault, show what is wrong.
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
int crc_check(int *bufr, int msg_size, int nnodes, int rank);
int full_check(int *bufr, int msg_size, int nnodes, int rank);
void fill_buffer(int *bufs, int msg_size);
void disp_progress(int n, int nnodes, int msg_size, time_t start, int *bufs);
int get_value(int seed);


/* ===++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=== */
/*
** Constants
*/
#define CHKSUM_SEED	(0xAAAA5555)
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
unsigned int chksum;
int do_check;
int rank, nnodes;


    MPI_Init (&argc, &argv);
    MPI_Comm_size (MPI_COMM_WORLD, &nnodes);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);

    if (argc == 1)   {
	do_check= 1;
    } else if (argc == 2)   {
	if (strcmp("crc", argv[1]) == 0)   {
	    do_check= 0;
	} else if (strcmp("full", argv[1]) == 0)   {
	    do_check= 1;
	} else   {
	    if (rank == 0)   {
		fprintf(stderr, "%s ERROR: Invalid argument (not \"crc\" or "
		    "\"full\")\n", argv[0]);
	    }
	    exit(-1);
	}
    } else   {
	if (rank == 0)   {
	    fprintf(stderr, "Usage:   %s [crc|full]\n", argv[0]);
	    fprintf(stderr, "       crc   Do a 32-bit CRC check of the data\n");
	    fprintf(stderr, "       full  Do a full check of the data\n");
	}
	exit(-1);
    }

    msg_size= MSG_SIZE;
    ntest= ITERATIONS;

    if (rank == 0)   {
	printf("\n\nData integrety test\n");
	printf("===++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++"
	    "=++=++=++=++===\n");
	printf("    There will be %d iterations\n", ntest);
	printf("    In each iteration each node will send %d integers (%ld "
	    "bytes each)\n", msg_size, sizeof(int));
	printf("    to each of the %d nodes\n", nnodes);
	if (do_check)   {
	    printf("    Doing a full check on the data (not just CRC)\n");
	} else   {
	    printf("    Doing a 32-bit CRC check on the data\n");
	}
	printf("    'a' through 'o' are specific patterns. '.' and ':' are "
	    "random patterns\n");
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


    /*
    ** Fill the send buffer.
    ** The first word is the seed to srandom().
    ** The second word is the checksum of all the data words.
    ** The third through the last words are data.
    */
    seed= (int)time(NULL) * (rank + 117);
    if ((seed >= 0) && (seed <= 15))   {
	/* Seed is one of the fixed patterns; don't use it */
	seed= seed + 16;
    }
    srandom(seed);
    chksum= CHKSUM_SEED;
    for (i = 2; i < msg_size; i++) {
	bufs[i] = (int)random();
	chksum= chksum ^ (unsigned int)bufs[i];
    }
    bufs[0]= seed;
    bufs[1]= chksum;

    /*
    ** Pre-post all receives
    */
    bufr= memset(bufr, 0xd6, nnodes * msg_size * sizeof (int));
    for (i= 0; i < nnodes; i++)   {
	bufptr= bufr + (i * msg_size);
	MPI_Irecv(bufptr, msg_size, MPI_INT, i, 1001, MPI_COMM_WORLD, &rreq[i]);
    }

    if (rank == 0)   {
	printf("\nElapsed    Iterations        Aggregate\n");
	printf("Time                         data sent\n");
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


	/* Check the received data */
	if (!do_check)   {
	    if (crc_check(bufr, msg_size, nnodes, rank) != 0)   {
		fprintf(stderr, "CRC check failed\n");
		exit(-1);
	    }
	} else   {
	    /* Fully Check the data */
	    if (full_check(bufr, msg_size, nnodes, rank) != 0)   {
		fprintf(stderr, "Full check failed\n");
		exit(-1);
	    }
	}


	/* Pre-post the receives */
	bufr= memset(bufr, 0xd6, nnodes * msg_size * sizeof (int));
	for (i= 0; i < nnodes; i++)   {
	    bufptr= bufr + (i * msg_size);
	    MPI_Irecv(bufptr, msg_size, MPI_INT, i, 1001, MPI_COMM_WORLD,
		&rreq[i]);
	}


	/* Make sure the sends have completed */
	MPI_Waitall(nnodes, sreq, MPI_STATUSES_IGNORE);


	/* Change the data patterns around once in a while */
	if (((n + 1) % 50) == 0)   {
	    fill_buffer(bufs, msg_size);
	}
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

int
get_value(int seed)
{

int value;


    switch (seed)   {
	case  0: value= 0x00000000; break;
	case  1: value= 0xffffffff; break;
	case  2: value= 0xaaaaaaaa; break;
	case  3: value= 0x55555555; break;
	case  4: value= 0x5a5a5a5a; break;
	case  5: value= 0xa5a5a5a5; break;
	case  6: value= 0xf0f0f0f0; break;
	case  7: value= 0x0f0f0f0f; break;
	case  8: value= 0xff00ff00; break;
	case  9: value= 0x00ff00ff; break;
	case 10: value= 0xffff0000; break;
	case 11: value= 0x0000ffff; break;
	case 12: value= 0xaa55aa55; break;
	case 13: value= 0x55aa55aa; break;
	case 14: value= 0xaaaa5555; break;
	case 15: value= 0x5555aaaa; break;
	default: value= (int)random();
    }

    return value;

}  /* end of get_value() */

/* ===++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=== */

int
crc_check(int *bufr, int msg_size, int nnodes, int rank)
{

int i, j;
int *bufptr;
int chksum;
int seed;
int error;
int abort;
int value;
int expected, gotten;
char err_line[1024];


    abort= 0;
    for (i= 0; i < nnodes; i++)   {
	bufptr= bufr + (i * msg_size);
	chksum= CHKSUM_SEED;
	for (j = 2; j < msg_size; j++) {
	    chksum= chksum ^ (unsigned int)bufptr[j];
	}
	if (chksum != bufptr[1])   {
	    seed= bufptr[0];
	    fprintf(stderr, "\n*** Checksum error on node %d from %d using "
		"pattern %d\n", rank, i, seed);

	    srandom(seed);
	    error= 0;
	    for (j = 2; j < msg_size; j++) {
		value= get_value(seed);
		if (bufptr[j] != value)   {
		    if ((error == 0) && (j > 2))   {
			/* Print last good compare as the lead in sequence */
			fprintf(stderr, "Node %-4d   Word %6d: wanted 0x%08x   "
			    "got 0x%08x\n", rank, j, expected, gotten);
		    }

		    sprintf(err_line, "Node %-4d   Word %6d: wanted 0x%08x   "
			"got 0x%08x\n", rank, j, value, bufptr[j]);
		    if (error < 10)   {
			fprintf(stderr, "%s", err_line);
		    }
		    error++;
		    abort= 1;
		} else   {
		    expected= value;
		    gotten= bufptr[j];
		    if (error)   {
			if (error > 10)   {
			    fprintf(stderr, "Node %-4d   ...\n", rank);
			    fprintf(stderr, "%s", err_line);
			}
			fprintf(stderr, "Node %-4d   Word %6d: wanted 0x%08x   "
			    "got 0x%08x\n", rank, j, expected, gotten);
			fprintf(stderr, "Node %-4d Data corruption in %d "
			    "words\n", rank, error);
		    }
		    error= 0;
		}
	    }
	    fprintf(stderr, "Node %-4d   ...\n", rank);
	    fprintf(stderr, "%s", err_line);
	    fprintf(stderr, "Node %-4d   Word %6d: wanted 0x%08x   "
		"got 0x%08x\n", rank, j, expected, gotten);
	    fprintf(stderr, "Node %-4d Data corruption in %d words\n",
		rank, error);
	}
    }

    if (abort)   {
	return -1;
    } else   {
	return 0;
    }

}  /* end of crc_check() */

/* ===++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=== */

int
full_check(int *bufr, int msg_size, int nnodes, int rank)
{

int i, j;
int *bufptr;
int chksum;
int seed;
int error;
int abort;
int value;
int expected, gotten;
char err_line[1024];


    abort= 0;
    for (i= 0; i < nnodes; i++)   {
	bufptr= bufr + (i * msg_size);
	chksum= CHKSUM_SEED;
	seed= bufptr[0];
	srandom(seed);
	error= 0;
	for (j = 2; j < msg_size; j++) {
	    value= get_value(seed);
	    if (bufptr[j] != value)   {
		if ((error == 0) && (j > 2))   {
		    /* Print the last good compare as the lead in sequence */
		    fprintf(stderr, "Node %-4d   Word %6d: wanted 0x%08x   "
			"got 0x%08x\n", rank, j, expected, gotten);
		}

		sprintf(err_line, "Node %-4d   Word %6d: wanted 0x%08x   got "
		    "0x%08x\n", rank, j, value, bufptr[j]);
		if (error < 10)   {
		    /* Print a few of the wrong data pairs */
		    fprintf(stderr, "%s", err_line);
		}
		error++;
		abort= 1;
	    } else   {
		expected= value;
		gotten= bufptr[j];
		if (error)   {
		    if (error > 10)   {
			fprintf(stderr, "Node %-4d   ...\n", rank);
			fprintf(stderr, "%s", err_line);
		    }
		    fprintf(stderr, "Node %-4d   Word %6d: wanted 0x%08x   "
			"got 0x%08x\n", rank, j, expected, gotten);
		    fprintf(stderr, "Node %-4d Data corruption in %d words\n",
			rank, error);
		    fprintf(stderr, "*** Compare error on node %d from %d "
			"using pattern %d\n", rank, i, seed);
		}
		error= 0;
	    }
	    chksum= chksum ^ (unsigned int)bufptr[j];
	}

	if (error)   {
	    fprintf(stderr, "Node %-4d   ...\n", rank);
	    fprintf(stderr, "%s", err_line);
	    fprintf(stderr, "Node %-4d   Word %6d: wanted 0x%08x   "
		"got 0x%08x\n", rank, j, expected, gotten);
	    fprintf(stderr, "Node %-4d Data corruption in %d words\n",
		rank, error);
	    fprintf(stderr, "*** Compare error on node %d from %d using "
		"pattern %d\n", rank, i, seed);
	    error= 0;
	}
	if (chksum != bufptr[1])   {
	    fprintf(stderr, "Node %-4d Check sum error\n", rank);
	    abort= 1;
	}
    }

    if (abort)   {
	return -1;
    } else   {
	return 0;
    }

}  /* end of full_check() */

/* ===++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=== */

void
fill_buffer(int *bufs, int msg_size)
{

int seed;
int chksum;
int i;
int value;


    seed= (int)time(NULL) ^ bufs[0];
    if ((seed >= 0) && (seed <= 15))   {
	/* Seed is one of the fixed patterns; don't use it */
	seed= seed + 16;
    }

    chksum= CHKSUM_SEED;
    if ((seed & 0x00000010) == 0x10)   {
	/* Use one of the fixed patterns */
	seed= seed & 0x0000000f;
	value= get_value(seed);
	for (i = 2; i < msg_size; i++) {
	    bufs[i] = value;
	    chksum= chksum ^ (unsigned int)bufs[i];
	}
    } else   {
	/* Use a random pattern */
	srandom(seed);
	for (i = 2; i < msg_size; i++) {
	    bufs[i] = (int)random();
	    chksum= chksum ^ (unsigned int)bufs[i];
	}
    }

    bufs[0]= seed;
    bufs[1]= chksum;

}  /* end of fill_buffer() */

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
int ss, mm, hh;
static int old_seed= 0;
static int flip= 0;


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
	hh= elapsed / (60 * 60);
	elapsed= elapsed - (hh * 60 * 60);
	mm= elapsed / 60;
	elapsed= elapsed - (mm * 60);
	ss= elapsed;
	printf("\n%3d:%02d:%02d  %-8d     %14.3f %-11s ", hh, mm, ss, n,
	    dtotal, unit);
	fflush(stdout);

    } else if (dot_time >= 2)   {
	last_dot_time= current_time;

	/*
	** Print a letter indicating whether we are sending random data
	** or one of the predefined patters.
	*/
	if ((bufs[0] >= 0) && (bufs[0] <= 15))   {
	    printf("%c", bufs[0] + 'a');
	} else   {
	    if (bufs[0] != old_seed)   {
		old_seed= bufs[0];
		flip= (flip ^ 0x1) & 0x1;
	    }
	    if (flip)   {
		printf(".");
	    } else   {
		printf(":");
	    }
	}
	fflush(stdout);
    }

}  /* end of disp_progress() */

/* ===++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=++=== */

