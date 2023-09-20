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
/* This program provides some simple verification of the MPI_Barrier
 * program.  All of the clients send a message to indicate that they
 * are alive (a simple character string) and then the all of the
 * clients enter an MPI_Barrier.  The server then Iprobes for a while
 * to make sure that none of the "through barrier" messages that the
 * clients send after leaving the barrier arive before the server enters 
 * the barrier. The server then enters the barrier, and upon leaving,
 * waits for a message from each client.
 */

#include "test.h"
#include "mpi.h"

#define WAIT_TIMES 500

int
main(argc, argv)
int argc;
char **argv;
{
    int rank, size, i, recv_flag, ret, passed;
    MPI_Status Status;
    char message[17];
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
	Test_Init("barrier", rank);
	/* Receive the startup messages from each of the 
	   other clients */
	for (i = 0; i < size - 1; i++) {
	    MPI_Recv(message, 17, MPI_CHAR, MPI_ANY_SOURCE, 2000, 
		     MPI_COMM_WORLD, &Status);
	}

	/* Now use Iprobe to make sure no more messages arive for a
	    while */
	passed = 1;
	for (i = 0; i < WAIT_TIMES; i++){
	    recv_flag = 0;
	    MPI_Iprobe(MPI_ANY_SOURCE, 2000, MPI_COMM_WORLD, 
		       &recv_flag, &Status);
	    if (recv_flag)
		passed = 0;
	}

	if (passed)
	    Test_Passed("Barrier Test 1");
	else
	    Test_Failed("Barrier Test 1");

	/* Now go into the barrier myself */
	MPI_Barrier(MPI_COMM_WORLD);

	/* And get everyones message who came out */
	for (i = 0; i < size - 1; i++) {
	    MPI_Recv(message, 13, MPI_CHAR, MPI_ANY_SOURCE, 2000, 
		     MPI_COMM_WORLD, &Status);
	}

	/* Now use Iprobe to make sure no more messages arive for a
	    while */
	passed = 1;
	for (i = 0; i < WAIT_TIMES; i++){
	    recv_flag = 0;
	    MPI_Iprobe(MPI_ANY_SOURCE, 2000, MPI_COMM_WORLD, 
		       &recv_flag, &Status);
	    if (recv_flag)
		passed = 0;
	}
	if (passed)
	    Test_Passed("Barrier Test 2");
	else
	    Test_Failed("Barrier Test 2");

	Test_Waitforall( );
	ret = Summarize_Test_Results();
	Test_Finalize();
	MPI_Finalize();
	return ret;
    } else {
	MPI_Send("Entering Barrier", 17, MPI_CHAR, 0, 2000, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Send("Past Barrier", 13, MPI_CHAR, 0, 2000, MPI_COMM_WORLD);
	Test_Waitforall( );
	MPI_Finalize();
	return 0;
    }
}
