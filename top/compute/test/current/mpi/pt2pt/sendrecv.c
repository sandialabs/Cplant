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
 *  $Id: sendrecv.c,v 1.1 1997/10/29 22:45:16 bright Exp $
 *
 *  (C) 1993 by Argonne National Laboratory and Mississippi State University.
 *
 */
 

/* 
 * Program to test all of the features of MPI_Send and MPI_Recv
 *
 * *** What is tested? ***
 * 1. Sending and receiving all basic types and many sizes - check
 * 2. Tag selectivity - check
 * 3. Error return codes for
 *    a. Invalid Communicator
 *    b. Invalid destination or source
 *    c. Count out of range
 *    d. Invalid type
 */

#include "test.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

static int src;
static int dest;

static int do_test1 = 1;
static int do_test2 = 1;
static int do_test3 = 1;

#define MAX_TYPES 12
#if defined(__STDC__) 
static int ntypes = 11;
#else
static int ntypes = 11;
#endif
static MPI_Datatype BasicTypes[MAX_TYPES];

static int maxbufferlen = 10000;

void 
AllocateBuffers(bufferspace, buffertypes, num_types, bufferlen)
    void **bufferspace;
    MPI_Datatype *buffertypes;
    int num_types;
    int bufferlen;
{
    int i;
    for (i = 0; i < ntypes; i++) {
	if (buffertypes[i] == MPI_CHAR)
	    bufferspace[i] = malloc(bufferlen * sizeof(char));
	else if (buffertypes[i] == MPI_SHORT)
	    bufferspace[i] = malloc(bufferlen * sizeof(short));
	else if (buffertypes[i] == MPI_INT)
	    bufferspace[i] = malloc(bufferlen * sizeof(int));
	else if (buffertypes[i] == MPI_LONG)
	    bufferspace[i] = malloc(bufferlen * sizeof(long));
	else if (buffertypes[i] == MPI_UNSIGNED_CHAR)
	    bufferspace[i] = malloc(bufferlen * sizeof(unsigned char));
	else if (buffertypes[i] == MPI_UNSIGNED_SHORT)
	    bufferspace[i] = malloc(bufferlen * sizeof(unsigned short));
	else if (buffertypes[i] == MPI_UNSIGNED)
	    bufferspace[i] = malloc(bufferlen * sizeof(unsigned int));
	else if (buffertypes[i] == MPI_UNSIGNED_LONG)
	    bufferspace[i] = malloc(bufferlen * sizeof(unsigned long));
	else if (buffertypes[i] == MPI_FLOAT)
	    bufferspace[i] = malloc(bufferlen * sizeof(float));
	else if (buffertypes[i] == MPI_DOUBLE)
	    bufferspace[i] = malloc(bufferlen * sizeof(double));
#if defined(__STDC__) 
	else if (MPI_LONG_DOUBLE && buffertypes[i] == MPI_LONG_DOUBLE)
	    bufferspace[i] = malloc(bufferlen * sizeof(long double));
#endif
	else if (buffertypes[i] == MPI_BYTE)
	    bufferspace[i] = malloc(bufferlen * sizeof(unsigned char));
    }
}

void 
FreeBuffers(buffers, nbuffers)
    void **buffers;
    int nbuffers;
{
    int i;
    for (i = 0; i < nbuffers; i++)
	free(buffers[i]);
}

void 
FillBuffers(bufferspace, buffertypes, num_types, bufferlen)
    void **bufferspace; 
    MPI_Datatype *buffertypes;
    int num_types;
    int bufferlen;
{
    int i, j;
    for (i = 0; i < ntypes; i++) {
	for (j = 0; j < bufferlen; j++) {
	    if (buffertypes[i] == MPI_CHAR)
		((char *)bufferspace[i])[j] = (char)j;
	    else if (buffertypes[i] == MPI_SHORT)
		((short *)bufferspace[i])[j] = (short)j;
	    else if (buffertypes[i] == MPI_INT)
		((int *)bufferspace[i])[j] = (int)j;
	    else if (buffertypes[i] == MPI_LONG)
		((long *)bufferspace[i])[j] = (long)j;
	    else if (buffertypes[i] == MPI_UNSIGNED_CHAR)
		((unsigned char *)bufferspace[i])[j] = (unsigned char)j;
	    else if (buffertypes[i] == MPI_UNSIGNED_SHORT)
		((unsigned short *)bufferspace[i])[j] = (unsigned short)j;
	    else if (buffertypes[i] == MPI_UNSIGNED)
		((unsigned int *)bufferspace[i])[j] = (unsigned int)j;
	    else if (buffertypes[i] == MPI_UNSIGNED_LONG)
		((unsigned long *)bufferspace[i])[j] = (unsigned long)j;
	    else if (buffertypes[i] == MPI_FLOAT)
		((float *)bufferspace[i])[j] = (float)j;
	    else if (buffertypes[i] == MPI_DOUBLE)
		((double *)bufferspace[i])[j] = (double)j;
#if defined(__STDC__) 
	    else if (MPI_LONG_DOUBLE && buffertypes[i] == MPI_LONG_DOUBLE)
		((long double *)bufferspace[i])[j] = (long double)j;
#endif
	    else if (buffertypes[i] == MPI_BYTE)
		((unsigned char *)bufferspace[i])[j] = (unsigned char)j;
	}
    }
}

int
CheckBuffer(bufferspace, buffertype, bufferlen)
    void *bufferspace; 
    MPI_Datatype buffertype; 
    int bufferlen;
{
    int i, j;
    char valerr[256];
    valerr[0] = 0;
    for (j = 0; j < bufferlen; j++) {
	if (buffertype == MPI_CHAR) {
	    if (((char *)bufferspace)[j] != (char)j) {
		sprintf( valerr, "%x != %x", 
			((char *)bufferspace)[j], (char)j );
		break;
		}
	} else if (buffertype == MPI_SHORT) {
	    if (((short *)bufferspace)[j] != (short)j) {
		sprintf( valerr, "%d != %d", 
			((short *)bufferspace)[j], (short)j );
		break;
		}
	} else if (buffertype == MPI_INT) {
	    if (((int *)bufferspace)[j] != (int)j) {
		sprintf( valerr, "%d != %d", 
			((int *)bufferspace)[j], (int)j );
		break;
		}
	} else if (buffertype == MPI_LONG) {
	    if (((long *)bufferspace)[j] != (long)j) {
		break;
		}
	} else if (buffertype == MPI_UNSIGNED_CHAR) {
	    if (((unsigned char *)bufferspace)[j] != (unsigned char)j) {
		break;
		}
	} else if (buffertype == MPI_UNSIGNED_SHORT) {
	    if (((unsigned short *)bufferspace)[j] != (unsigned short)j) {
		break;
		}
	} else if (buffertype == MPI_UNSIGNED) {
	    if (((unsigned int *)bufferspace)[j] != (unsigned int)j) {
		break;
		}
	} else if (buffertype == MPI_UNSIGNED_LONG) {
	    if (((unsigned long *)bufferspace)[j] != (unsigned long)j) {
		break;
		}
	} else if (buffertype == MPI_FLOAT) {
	    if (((float *)bufferspace)[j] != (float)j) {
		break;
		}
	} else if (buffertype == MPI_DOUBLE) {
	    if (((double *)bufferspace)[j] != (double)j) {
		break;
		}
#if defined(__STDC__)
	} else if (MPI_LONG_DOUBLE && buffertype == MPI_LONG_DOUBLE) {
	    if (((long double *)bufferspace)[j] != (long double)j) {
		break;
		}
#endif
	} else if (buffertype == MPI_BYTE) {
	    if (((unsigned char *)bufferspace)[j] != (unsigned char)j) {
		break;
		}
	}
    }
    /* Return +1 so an error in the first location is > 0 */
    if (j < bufferlen) {
	if (valerr[0]) fprintf( stderr, "Different value[%d] = %s\n", 
			        j, valerr );
	else
	    fprintf( stderr, "Different value[%d]\n", j );
	return j+1;
	}
    return 0;
}

void 
SetupBasicTypes()
{
    BasicTypes[0] = MPI_CHAR;
    BasicTypes[1] = MPI_SHORT;
    BasicTypes[2] = MPI_INT;
    BasicTypes[3] = MPI_LONG;
    BasicTypes[4] = MPI_UNSIGNED_CHAR;
    BasicTypes[5] = MPI_UNSIGNED_SHORT;
    BasicTypes[6] = MPI_UNSIGNED;
    BasicTypes[7] = MPI_UNSIGNED_LONG;
    BasicTypes[8] = MPI_FLOAT;
    BasicTypes[9] = MPI_DOUBLE;
    BasicTypes[10] = MPI_BYTE;
    /* By making the BYTE type LAST, we make it easier to handle heterogeneous
       systems that may not support all of the types */
#if defined (__STDC__)
    if (MPI_LONG_DOUBLE) {
	BasicTypes[11] = MPI_LONG_DOUBLE;
	}
    else {
	ntypes = 11;
	}
#endif

}

void 
SenderTest1()
{
    void *bufferspace[MAX_TYPES];
    int Curr_Type, i, j;

    AllocateBuffers(bufferspace, BasicTypes, ntypes, maxbufferlen);
    FillBuffers(bufferspace, BasicTypes, ntypes, maxbufferlen);
    for (i = 0; i < ntypes; i++) {
	MPI_Send( (void *)0, 0, BasicTypes[i], dest, 2000, MPI_COMM_WORLD );
	for (j = 0; j < maxbufferlen; j += 500)
	    MPI_Send(bufferspace[i], j, BasicTypes[i], dest, 
		     2000, MPI_COMM_WORLD);
    }
    FreeBuffers(bufferspace, ntypes);
}

void
ReceiverTest1()
{
    void *bufferspace[MAX_TYPES];
    int Curr_Type, i, j;
    char message[81];
    MPI_Status Stat;
    int dummy, passed;

    AllocateBuffers(bufferspace, BasicTypes, ntypes, maxbufferlen);
    for (i = 0; i < ntypes; i++) {
	passed = 1;
	MPI_Recv( (void *)0, 0, BasicTypes[i], src, 
		     2000, MPI_COMM_WORLD, &Stat);
	if (Stat.MPI_SOURCE != src) {
	    fprintf(stderr, "*** Incorrect Source returned. ***\n");
	    Test_Failed(message);
	    passed = 0;
	} else if (Stat.MPI_TAG != 2000) {	
	    fprintf(stderr, "*** Incorrect Tag returned. ***\n");	    
	    Test_Failed(message);
	    passed = 0;
	} else if (MPI_Get_count(&Stat, BasicTypes[i], &dummy) ||
		   dummy != 0) {
	    fprintf(stderr, 
		    "*** Incorrect Count returned, Count = %d. ***\n", 
		    dummy);
	    Test_Failed(message);
	    passed = 0;
	    }
	/* Try different sized messages */
	for (j = 0; j < maxbufferlen; j += 500) {
	    MPI_Recv(bufferspace[i], j, BasicTypes[i], src, 
		     2000, MPI_COMM_WORLD, &Stat);
	    sprintf(message, "Send-Receive Test, Type %d, Count %d",
		    i, j);
	    if (Stat.MPI_SOURCE != src) {
		fprintf(stderr, "*** Incorrect Source returned. ***\n");
		Test_Failed(message);
		passed = 0;
	    } else if (Stat.MPI_TAG != 2000) {	
		fprintf(stderr, "*** Incorrect Tag returned. ***\n");	    
		Test_Failed(message);
		passed = 0;
	    } else if (MPI_Get_count(&Stat, BasicTypes[i], &dummy) ||
		       dummy != j) {
		fprintf(stderr, 
	    "*** Incorrect Count returned, Count = %d (should be %d). ***\n", 
			dummy, j);
		Test_Failed(message);
		passed = 0;
	    } else if(CheckBuffer(bufferspace[i], BasicTypes[i], j)) {
		fprintf(stderr, 
	       "*** Incorrect Message received (type = %d, count = %d). ***\n",
			i, j );
		Test_Failed(message);
		passed = 0;
	    } else fprintf(stderr, 
	       "Message of count %d, type %d received correctly.\n", j, i);
	}
	sprintf(message, "Send-Receive Test, Type %d",
		i);
	if (passed) 
	    Test_Passed(message);
	else 
	    Test_Failed(message);
    }
    FreeBuffers(bufferspace, ntypes);
}

#define MAX_ORDER_TAG 2010
/* Test Tag Selectivity.
   Note that we must use non-blocking sends here, since otherwise we could 
   deadlock waiting to receive/send the first message
*/
void 
SenderTest2()
{
    int *buffer;
    int i;
    MPI_Request r[10];
    MPI_Status  s[10];

    buffer = (int *)malloc(maxbufferlen * sizeof(int));
    for (i = 0; i < maxbufferlen; i++)
	buffer[i] = i;
    
    for (i = 2001; i <= MAX_ORDER_TAG; i++)
	MPI_Isend(buffer, maxbufferlen, MPI_INT, dest,
		 i, MPI_COMM_WORLD, &r[i-2001] );
    
    MPI_Waitall( MAX_ORDER_TAG-2001+1, r, s );
    free(buffer);
    
    return;
}

void
ReceiverTest2()
{
    int *buffer;
    int i, j;
    char message[81];
    MPI_Status Stat;
    int dummy, passed;
    int errloc;

    buffer = (int *)calloc(maxbufferlen,sizeof(int));
    passed = 1;

    for (i = MAX_ORDER_TAG; i >= 2001; i--) {
	MPI_Recv(buffer, maxbufferlen, MPI_INT, src, 
		 i, MPI_COMM_WORLD, &Stat);
	sprintf(message, "Tag Selectivity Test, Tag %d",
		i);
	if (Stat.MPI_SOURCE != src) {
	    fprintf(stderr, "*** Incorrect Source returned. ***\n");
	    Test_Failed(message);
	} else if (Stat.MPI_TAG != i) {	
	    fprintf(stderr, "*** Incorrect Tag returned. ***\n");	    
	    Test_Failed(message);
	} else if (MPI_Get_count(&Stat, MPI_INT, &dummy) ||
		   dummy != maxbufferlen) {
	    fprintf(stderr, 
		    "*** Incorrect Count returned, Count = %d. ***\n", 
		    dummy);
	    Test_Failed(message);
	} else if((errloc = 
		   CheckBuffer((void*)buffer, MPI_INT, maxbufferlen))) {
	    fprintf(stderr, 
		    "*** Incorrect Message received at %d (tag=%d). ***\n",
		    errloc-1, i);
	    Test_Failed(message);
	    passed = 0;
	}
	/* Clear out the buffer */
	for (j = 0; j < maxbufferlen; j++)
	    buffer[j] = -1;
    }
    strncpy(message, "Tag Selectivity Test", 81);
    if (passed)
	Test_Passed(message);
    else
	Test_Failed(message);
    free(buffer);
    return;
}

void
SenderTest3()
{
    return;
}

void
ReceiverTest3()
{
    int err_code;
    int buffer[20];
    MPI_Datatype bogus_type = MPI_DATATYPE_NULL;
    MPI_Status status;
    int myrank;
    int *tag_ubp;
    int large_tag, flag, small_tag;

    MPI_Errhandler_set(MPI_COMM_WORLD, MPIR_ERRORS_WARN);

    MPI_Comm_rank( MPI_COMM_WORLD, &myrank );

    if (myrank == 0) {
	fprintf( stderr, 
"There should be eight error messages about invalid communicator\n\
count argument, datatype argument, tag, rank, buffer send and buffer recv\n" );
	}
    if (MPI_Send(buffer, 20, MPI_INT, dest,
		 1, MPI_COMM_NULL) == MPI_SUCCESS){
	Test_Failed("NULL Communicator Test");
    }
    else
	Test_Passed("NULL Communicator Test");

    if (MPI_Send(buffer, -1, MPI_INT, dest,
		 1, MPI_COMM_WORLD) == MPI_SUCCESS){
	Test_Failed("Invalid Count Test");
    }
    else
	Test_Passed("Invalid Count Test");

    if (MPI_Send(buffer, 20, bogus_type, dest,
		 1, MPI_COMM_WORLD) == MPI_SUCCESS){
	Test_Failed("Invalid Type Test");
    }
    else
	Test_Passed("Invalid Type Test");

    small_tag = -1;
    if (small_tag == MPI_ANY_TAG) small_tag = -2;
    if (MPI_Send(buffer, 20, MPI_INT, dest, 
		 small_tag, MPI_COMM_WORLD) == MPI_SUCCESS) {
        Test_Failed("Invalid Tag Test");
    }
    else
	Test_Passed("Invalid Tag Test");

    /* Form a tag that is too large */
    MPI_Attr_get( MPI_COMM_WORLD, MPI_TAG_UB, (void **)&tag_ubp, &flag );
    if (!flag) Test_Failed("Could not get tag ub!" );
    large_tag = *tag_ubp + 1;
    if (large_tag > *tag_ubp) {
	if (MPI_Send(buffer, 20, MPI_INT, dest, 
		     -1, MPI_COMM_WORLD) == MPI_SUCCESS) {
	    Test_Failed("Invalid Tag Test");
	    }
	else
	    Test_Passed("Invalid Tag Test");
	}

    if (MPI_Send(buffer, 20, MPI_INT, 300,
		 1, MPI_COMM_WORLD) == MPI_SUCCESS) {
	Test_Failed("Invalid Destination Test");
    }
    else
	Test_Passed("Invalid Destination Test");

    if (MPI_Send((void *)0, 10, MPI_INT, dest,
		 1, MPI_COMM_WORLD) == MPI_SUCCESS){
	Test_Failed("Invalid Buffer Test (send)");
    }
    else
	Test_Passed("Invalid Buffer Test (send)");

    /* A receive test might not fail until it is triggered... */
    if (MPI_Recv((void *)0, 10, MPI_INT, dest,
		 1, MPI_COMM_WORLD, &status) == MPI_SUCCESS){
	Test_Failed("Invalid Buffer Test (recv)");
    }
    else
	Test_Passed("Invalid Buffer Test (recv)");



    return;
}

int 
main(argc, argv)
    int argc;
    char **argv;
{
    int myrank, mysize;
    int rc, itemp;

    MPI_Init(&argc, &argv);
/*
PUMA_Set_recv_debug_flag(1);
PUMA_Set_send_debug_flag(1);
*/
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &mysize);
    Test_Init("sendrecv", myrank);
    SetupBasicTypes();

    if (mysize != 2) {
	fprintf(stderr, 
		"*** This test program requires exactly 2 processes.\n");
	exit(-1);
    }

    /* Get the min of the basic types */
    itemp = ntypes;
    MPI_Allreduce( &itemp, &ntypes, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD );

    /* dest writes out the received stats; for the output to be
       consistant (with the final check), it should be procees 0 */
    if (argc > 1 && argv[1] && strcmp( "-alt", argv[1] ) == 0) {
	dest = 1;
	src  = 0;
	}
    else {
	src  = 1;
	dest = 0;
	}
    /* Turn stdout's buffering to line buffered so it mixes right with
       stderr in output files. (hopefully) */
    /*setvbuf(stdout, NULL, _IOLBF, 0);*/
    /*setvbuf(stderr, NULL, _IOLBF, 0);*/
    
    if (myrank == src) {
	if (do_test1)
	    SenderTest1();
	if (do_test2)
	    SenderTest2();
	if (do_test3)
	    SenderTest3(); 
    } else if (myrank == dest) {
	if (do_test1)
	    ReceiverTest1();
	if (do_test2)
	    ReceiverTest2();
	if (do_test3) 
	    ReceiverTest3();
    } else {
	fprintf(stderr, "*** This program uses exactly 2 processes! ***\n");
	exit(-1);
    }
    if (myrank == dest) {
	rc = Summarize_Test_Results();
	Test_Finalize();
    }
    else {
	Test_Finalize();
	rc = 0;
    }
    Test_Waitforall( );
    MPI_Finalize();
    return rc;
}
