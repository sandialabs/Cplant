/* 
 *   $Id: malloc.c,v 1.1 2000/05/10 21:42:48 jrjohns Exp $    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

/* These are routines for allocating and deallocating memory.
   They should be called as ADIOI_Malloc(size) and
   ADIOI_Free(ptr). In adio.h, they are macro-replaced to 
   ADIOI_Malloc(size,__LINE__,__FILE__) and 
   ADIOI_Free(ptr,__LINE__,__FILE__).

   Later on, add some tracing and error checking, similar to 
   MPID_trmalloc. */

/* can't include adio.h here, because of the macro */

#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>
#include "mpipr.h"

void *ADIOI_Malloc(size_t size, int lineno, char *fname)
{
    void *new;

#ifdef __XFS
    new = (void *) memalign(__XFS_MEMALIGN, size);
#else
    new = (void *) malloc(size);
#endif
    if (!new) {
	printf("Out of memory in file %s, line %d\n", fname, lineno);
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    return new;
}


void *ADIOI_Calloc(size_t nelem, size_t elsize, int lineno, char *fname)
{
    void *new;

    new = (void *) calloc(nelem, elsize);
    if (!new) {
	printf("Out of memory in file %s, line %d\n", fname, lineno);
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    return new;
}


void *ADIOI_Realloc(void *ptr, size_t size, int lineno, char *fname)
{
    void *new;

    new = (void *) realloc(ptr, size);
    if (!new) {
	printf("realloc failed in file %s, line %d\n", fname, lineno);
	MPI_Abort(MPI_COMM_WORLD, 1);
    }
    return new;
}


void ADIOI_Free(void *ptr, int lineno, char *fname)
{
    if (!ptr) {
	printf("Attempt to free null pointer in file %s, line %d\n", fname, lineno);
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    free(ptr);
}

