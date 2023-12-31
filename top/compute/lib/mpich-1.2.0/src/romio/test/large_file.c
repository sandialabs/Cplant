#include "mpi.h"
#include "mpio.h"  /* not necessary with MPICH 1.1.1 or HPMPI 1.4 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* writes a file of size 4 Gbytes and reads it back. 
   should be run on one process only*/
/* The file name is taken as a command-line argument. */
/* Can be used only on file systems on which ROMIO supports large files, 
   i.e., PIOFS, XFS, SFS, and HFS. */
   
#define SIZE 1048576*4     /* no. of long longs in each write/read */
#define NTIMES 128         /* no. of writes/reads */

int main(int argc, char **argv)
{
    MPI_File fh;
    MPI_Status status;
    MPI_Offset size;
    long long *buf, i;
    char *filename;
    int j, mynod, nprocs, len, flag, err;

    MPI_Init(&argc,&argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &mynod);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (nprocs != 1) {
	printf("Run this program on one process only\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    i = 1;
    while ((i < argc) && strcmp("-fname", *argv)) {
	i++;
	argv++;
    }
    if (i >= argc) {
	printf("\n*#  Usage: large -fname filename\n\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }
    argv++;
    len = strlen(*argv);
    filename = (char *) malloc(len+1);
    strcpy(filename, *argv);
    printf("This program creates an 4 Gbyte file. Don't run it if you don't have that much disk space!\n");

    buf = (long long *) malloc(SIZE * sizeof(long long));
    if (!buf) {
	printf("not enough memory to allocate buffer\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_File_open(MPI_COMM_SELF, filename, MPI_MODE_CREATE | MPI_MODE_RDWR,
                  MPI_INFO_NULL, &fh);

    for (i=0; i<NTIMES; i++) {
	for (j=0; j<SIZE; j++)
	    buf[j] = i*SIZE + j;
	
	err = MPI_File_write(fh, buf, SIZE, MPI_DOUBLE, &status);
        /* MPI_DOUBLE because not all MPI implementations define
           MPI_LONG_LONG_INT, even though the C compiler supports long long. */
        if (err != MPI_SUCCESS) {
	    printf("MPI_File_write returned error\n");
	    MPI_Abort(MPI_COMM_WORLD, 1);
	}
    }

    MPI_File_get_size(fh, &size);
    printf("file size = %ld bytes\n", size);

    MPI_File_seek(fh, 0, MPI_SEEK_SET);

    for (j=0; j<SIZE; j++) buf[j] = -1;

    flag = 0;
    for (i=0; i<NTIMES; i++) {
	err = MPI_File_read(fh, buf, SIZE, MPI_DOUBLE, &status);
        /* MPI_DOUBLE because not all MPI implementations define
           MPI_LONG_LONG_INT, even though the C compiler supports long long. */
        if (err != MPI_SUCCESS) {
	    printf("MPI_File_write returned error\n");
	    MPI_Abort(MPI_COMM_WORLD, 1);
	}
	for (j=0; j<SIZE; j++) 
	    if (buf[j] != i*SIZE + j) {
		printf("error: buf %d is %ld, should be %ld \n", j, buf[j], 
                                 i*SIZE + j);
		flag = 1;
	    }
    }

    if (!flag) printf("Data read back is correct\n");
    MPI_File_close(&fh);

    free(buf);
    free(filename);
    MPI_Finalize(); 
    return 0;
}
