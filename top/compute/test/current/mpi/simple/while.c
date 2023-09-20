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
/* while.c -- leader waits for some user input and
              then sends a token to all others. then
              they quit. dumb but useful.
*/
#include <stdio.h>
#include "mpi.h"

int main (int argc, char* argv[])
{
  int my_node, num_nodes, other;
  int left_nbr, right_nbr;
  int ierr;
  int i;

  float token; /* leader sends this token to everyone else */

  MPI_Status status;

  ierr = MPI_Init(&argc, &argv);
  if (ierr != MPI_SUCCESS) {
    printf("MPI initialization error\n");
  }
  
  MPI_Comm_size(MPI_COMM_WORLD, &num_nodes);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_node);

  printf("process %d: number of processes= %d\n", my_node, num_nodes);

  left_nbr  = (my_node + num_nodes - 1) % num_nodes;
  right_nbr = (my_node + 1            ) % num_nodes;

  if (my_node == 0) { /* get things started */
    token = 0.0;
    getchar();
    for (i=1; i<num_nodes; i++) {
      token++;
      MPI_Send(&token, 1, MPI_FLOAT, i, 0, MPI_COMM_WORLD);
    }
  } 
  else { /* wait for receipt of token */
#if 0
    /* force exception */
    if (my_node % 2) {
      token /= 0.0;
    }
#endif
    MPI_Recv(&token, 1, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, &status);
    printf("process %d: got token %fl\n", my_node, token);
  }
  return 0;
}
