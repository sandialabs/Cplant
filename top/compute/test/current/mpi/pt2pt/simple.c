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
** $Id: simple.c,v 1.5 2001/03/24 00:49:29 jsotto Exp $
*/
#include<stdio.h>
#include<stdlib.h>
#include"mpi.h"
#include "puma.h"


main( int ac, char **av )
{

  MPI_Status  stat;
  int         rank,i;
  int         data[2000];
  char  *tmalloc;

  printf("simple: in main, call MPI_Init\n");

  MPI_Init( &ac, &av );

  printf("simple: in main, call MPI_Comm_rank\n");

  MPI_Comm_rank( MPI_COMM_WORLD, &rank );

  printf("simple: nid %d pid %d rank %d (%d)\n",
             _my_pnid, _my_ppid, _my_rank, rank);

  if ( rank == 0 ) {

    printf("simple: pnid %d, rank %d SENDING\n",_my_pnid,rank);

    MPI_Send( data, 2000, MPI_INT, 1, 0, MPI_COMM_WORLD );

    printf("simple: pnid %d, rank %d DONE SENDING\n",_my_pnid,rank);

  }
  else {

    printf("simple: pnid %d, rank %d RECEIVING\n",_my_pnid,rank);

    MPI_Recv( data, 2000, MPI_INT, 0, 0, MPI_COMM_WORLD, &stat );

    printf("simple: pnid %d, rank %d DONE RECEIVING\n",_my_pnid,rank);
  }

  MPI_Finalize();

}
  
