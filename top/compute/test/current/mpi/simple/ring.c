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
#include <stdio.h>
#include "mpi.h"

#define TIMES_AROUND 1000
#define NUMFLOATS 1024*4   /* gives a 2-page buffer on the alpha */

int main (int argc, char* argv[])
{
  int my_node, num_nodes, other;
  int left_nbr, right_nbr;
  int ierr;
  int i, error=0;

  float token[NUMFLOATS];  /* pass this token around the ring of available
                            processes; each time it's received, it's
                            incremented */

  MPI_Status status;

  ierr = MPI_Init(&argc, &argv);
  if (ierr != MPI_SUCCESS) {
    printf("MPI initialization error\n");
  }
  
  MPI_Comm_size(MPI_COMM_WORLD, &num_nodes);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_node);

  printf("process %d: number of processes= %d\n", my_node, num_nodes);
//  printf("process %d: &token= 0x%x\n", my_node, token);

  left_nbr  = (my_node + num_nodes - 1) % num_nodes;
  right_nbr = (my_node + 1            ) % num_nodes;

  if (my_node == 0) { /* get things started */
    token[0] = 0.0;
    for (i=0; i<TIMES_AROUND; i++) {
      MPI_Send(token, 1, MPI_FLOAT, right_nbr, 0, MPI_COMM_WORLD);
      MPI_Recv(token, 1, MPI_FLOAT, left_nbr, 0, MPI_COMM_WORLD, &status);
      token[0]++;
    }
    if (token[0] == (float) (num_nodes*TIMES_AROUND)) {
      printf("token= %fl, matches TIMES_AROUND*num_nodes (things look ok).\n", token[0]); 
    }
    else {
      printf("error: value of token (%fl) does not match TIMES_AROUND*num_nodes (%d)!\n", token[0], num_nodes); 
    }
  } 
  else { /* swing your partner, yee-hah */
#if 0
    /* force exception */
    if (my_node % 2) {
      token[0] /= 0.0;
    }
#endif
    for (i=0; i<TIMES_AROUND; i++) {
      MPI_Recv(token, 1, MPI_FLOAT, left_nbr, 0, MPI_COMM_WORLD, &status);
      token[0]++;
      MPI_Send(token, 1, MPI_FLOAT, right_nbr, 0, MPI_COMM_WORLD);
    }
  }

  if (my_node == 0) { /* get things started */
    printf("doing long send...\n");

    for (i=0; i<NUMFLOATS; i++) {
      token[i] = 0.0;
    }
    MPI_Send(token, NUMFLOATS, MPI_FLOAT, right_nbr, 0, MPI_COMM_WORLD);
    MPI_Recv(token, NUMFLOATS, MPI_FLOAT, left_nbr, 0, MPI_COMM_WORLD, &status);
    for (i=0; i<NUMFLOATS; i++) {
      token[i]++;
      if (token[i] != (float) num_nodes) {
        error++;
        break;
      }
    }
    if ( !error ) {
      printf("...passed long send test.\n");
    }
    else {
      printf("...error: FAILED long send test.\n");
    }
  } 
  else { /* swing your partner, yee-hah */
    MPI_Recv(token, NUMFLOATS, MPI_FLOAT, left_nbr, 0, MPI_COMM_WORLD, &status);
    for (i=0; i<NUMFLOATS; i++) {
      token[i]++;
    }
    MPI_Send(token, NUMFLOATS, MPI_FLOAT, right_nbr, 0, MPI_COMM_WORLD);
  }

  return 0;
}
