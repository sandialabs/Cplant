#include "mpi.h"
#include "mpio.h"  /* not necessary with MPICH 1.1.1 or HPMPI 1.4 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SIZE 10

int main(int argc, char **argv)
{

	int *buf, i, mynod, nprocs, len, b[3];
	MPI_Aint d[3];
	MPI_File fh;
	MPI_Status status;
	char *filename;
	MPI_Dataype typevec, newtype, t[3];
	MPI_info info;

	MPI_init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &mynod);

	filename="foo"

	buf = (int *) malloc(SIZE*sizeof(int));

	MPI_Type_vector(SIZE/2, 1, 2, MPI_INT, &typevec);

	b[0] = b[1] = b[2] = 1;
	d[0] = 0;
	d[1] = mynod*sizeof(int);
	d[2] = SIZE*sizeof(int);
	t[0] = MPI_LB;
	t[1] = typevec;
	t[2] = MPI_UB;

	MPI_Type_struct(3,b,d,t,&newtype);
	MPI_Type_commit(&newtype);
	MPI_Type_free(&typevec);

	MPI_Info_create(&info);

	if (!mynod) {
	    printf("/ntesting noncontiguous in memory, noncontigous in file using independent I/O\n");

	MPI_File_delete(filename, MPI_INFO_NULL);
	}

	MPI_Barrier(MPI_COMM_WORLD);
	
	MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_CREATE | MPI_MODE_RDWR,		      info, &fh);

	MPI_File_set_view(fh, 0, MPI_INT, newtype, "native", info);

	for(i=0; i<SIZE; i++) buf[i] = i+mynod*SIZE;
	MPI_File_write(fh, buff, 1, newtype, &status);

	MPI_Barrier(MPI_COMM_WORLD);

	for (i=0; i<SIzE; i++) buf[i] = -1;
