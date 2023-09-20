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

#define TIMES_AROUND 1

int main (int argc, char* argv[])
{
  int my_node, num_nodes, other;
  int left_nbr, right_nbr;
  int ierr;
  int i, error=0;

  float token;  /* pass this token around the ring of available
                            processes; each time it's received, it's
                            incremented */

  MPI_Status status;

  ierr = MPI_Init(&argc, &argv);
  if (ierr != MPI_SUCCESS) {
    printf("MPI initialization error\n");
  }
  
  MPI_Comm_size(MPI_COMM_WORLD, &num_nodes);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_node);

  left_nbr  = (my_node + num_nodes - 1) % num_nodes;
  right_nbr = (my_node + 1            ) % num_nodes;

  if (my_node == 0) { /* get things started */
    printf("enter something and we'll finish up...\n");
    getchar();
    token = 0.0;
    for (i=0; i<TIMES_AROUND; i++) {
      MPI_Send(&token, 1, MPI_FLOAT, right_nbr, 0, MPI_COMM_WORLD);
      MPI_Recv(&token, 1, MPI_FLOAT, left_nbr, 0, MPI_COMM_WORLD, &status);
      token++;
    }
    if (token == (float) (num_nodes*TIMES_AROUND)) {
      printf("token= %fl, matches TIMES_AROUND*num_nodes (things look ok).\n", token); 
    }
    else {
      printf("error: value of token (%fl) does not match TIMES_AROUND*num_nodes (%d)!\n", token, num_nodes); 
    }
  } 
  else { /* swing your partner, yee-hah */
    for (i=0; i<TIMES_AROUND; i++) {
      MPI_Recv(&token, 1, MPI_FLOAT, left_nbr, 0, MPI_COMM_WORLD, &status);
      token++;
      MPI_Send(&token, 1, MPI_FLOAT, right_nbr, 0, MPI_COMM_WORLD);
    }
  }
  MPI_Finalize();

  return 0;
}
