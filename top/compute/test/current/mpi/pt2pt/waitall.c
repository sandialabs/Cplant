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
 *  $Id: waitall.c,v 1.1 1997/10/29 22:45:48 bright Exp $
 *
 *  (C) 1993 by Argonne National Laboratory and Mississippi State University.
 *
 */
 

/*
 * This code test waitall; in one version of MPICH, it uncovered some
 * problems with the ADI Test calls.
 */
/* #define i_ntotin 256  */ /* ok    */
/* #define i_ntotin 257  */ /* fails */
#define i_ntotin 256  /* fails */

#include <stdio.h>
#include "mpi.h"

#define DAR 32  /* ``Data: ARray''  */


int main(argc, argv)
int argc;
char** argv;
 {
  int locId ;
  int data [i_ntotin] ;

  MPI_Init(&argc, &argv) ;
  MPI_Comm_rank(MPI_COMM_WORLD, &locId) ;

  if(locId == 0) {

    /* The server... */

    MPI_Status status[2] ;
    MPI_Request events [2] ;

    int eventId ;

    int dstId = 1 ;

    int i ;

    for(i = 0 ; i < i_ntotin ; i++)
      data [i] = i + 1 ;

    events [0] = MPI_REQUEST_NULL ;
    events [1] = MPI_REQUEST_NULL ;

    MPI_Isend(data, i_ntotin, MPI_INT, dstId, DAR,
              MPI_COMM_WORLD, events + 1) ;
        /* enable send of data */

    /*_begin_trace_code  */
    /* printf("locId = %d: MPI_Isend(%x, %d, %x, %d, %d, %x, %x)\n",
      locId, data, i_ntotin, MPI_INT, dstId, DAR, MPI_COMM_WORLD, events [1]); 
      */
    /*_end_trace_code  */

    /*_begin_trace_code  */
    /* printf("locId = %d: MPI_Waitany(%d, [%x, %x], %x %x)...",
      locId, 2, events [0], events [1], &eventId, &status) ; */
    /*_end_trace_code  */

    MPI_Waitany(2, events, &eventId, status) ;

    /*_begin_trace_code  */
    printf("done.  eventId = %x\n", eventId) ;
    /*_end_trace_code  */
  }

  if(locId == 1) {

    /* The Client...  */

    MPI_Status status ;

    int srcId = MPI_ANY_SOURCE ;

    /*_begin_trace_code  */
    /*
    printf("locId = %d: MPI_Recv(%x, %d, %x, %d, %d, %x, %x)...",
      locId, data, i_ntotin, MPI_INT, srcId, DAR, MPI_COMM_WORLD, &status) ;
      */
    /*_end_trace_code  */

    MPI_Recv(data, i_ntotin, MPI_INT, srcId, DAR,
             MPI_COMM_WORLD, &status) ;

    /*_begin_trace_code  */
    /*printf("done.\n") ;*/
    /*_end_trace_code  */

    /*
    printf("locId = %d: data [0] = %d, data [%d] = %d\n",
      locId, data [0], i_ntotin - 1, data [i_ntotin - 1]) ;
       */
  }

  MPI_Barrier( MPI_COMM_WORLD );
  if (locId == 0)
      printf( "Test complete\n" );
  MPI_Finalize() ;
}



