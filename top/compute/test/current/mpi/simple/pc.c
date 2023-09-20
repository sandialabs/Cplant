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

#define PRODUCTQ 0
#define FLOWQ 1
#define NUM_PRODUCTS 1000

void produce(int,int);
void consume(int,int);

int main (int argc, char* argv[])
{
  int my_node, num_nodes, other;
  int ierr;

  ierr = MPI_Init(&argc, &argv);
  if (ierr != MPI_SUCCESS) {
    printf("MPI initialization error\n");
  }
  
  MPI_Comm_size(MPI_COMM_WORLD, &num_nodes);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_node);

  printf("number of processes= %d\n", num_nodes);
  if (num_nodes != 2) {
    printf("hey, there need to be exactly 2 processes...\n");
    exit(-1);
  }

  other = 1-my_node;
  if (my_node) {
    produce(my_node,other);
  }
  else {
    consume(my_node,other);
  }
  return 0;
}

void produce(int my_node, int other) {
  int x=0, recv, send;
  MPI_Status status;

  printf("i am producer: %d\n", my_node);
  while(x < NUM_PRODUCTS) {
    MPI_Recv(&recv, 1, MPI_INT, other, FLOWQ, MPI_COMM_WORLD, &status);
    x += 1;                            //produce
    MPI_Send(&x, 1, MPI_INT, other, PRODUCTQ, MPI_COMM_WORLD);
  }
  printf("producer done producing %d things.\n", x);
}

void consume(int my_node, int other) {
  int x=0, send;
  MPI_Status status;

  printf("i am consumer: %d\n", my_node);
  while (x < NUM_PRODUCTS) {
    MPI_Send(&send, 1, MPI_INT, other, FLOWQ, MPI_COMM_WORLD);
    MPI_Recv(&x, 1, MPI_INT, other, PRODUCTQ, MPI_COMM_WORLD, &status);
  }
  printf("consumer consumed %d items.\n", x);
}
