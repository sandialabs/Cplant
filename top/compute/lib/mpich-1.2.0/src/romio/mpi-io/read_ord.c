/* 
 *   $Id: read_ord.c,v 1.1 2000/05/10 21:44:01 jrjohns Exp $    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
//#pragma weak MPI_File_read_ordered = PMPI_File_read_ordered
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
//#pragma _HP_SECONDARY_DEF PMPI_File_read_ordered MPI_File_read_ordered
#elif defined(HAVE_PRAGMA_CRI_DUP)
//#pragma _CRI duplicate MPI_File_read_ordered as PMPI_File_read_ordered
/* end of weak pragmas */
#endif

/* Include mapping from MPI->PMPI */
//#define __MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

/* status object not filled currently */

/*@
    MPI_File_read_ordered - Collective read using shared file pointer

Input Parameters:
. fh - file handle (handle)
. count - number of elements in buffer (nonnegative integer)
. datatype - datatype of each buffer element (handle)

Output Parameters:
. buf - initial address of buffer (choice)
. status - status object (Status)

.N fortran
@*/
int MPI_File_read_ordered(MPI_File fh, void *buf, int count, 
                          MPI_Datatype datatype, MPI_Status *status)
{
    int error_code, datatype_size, nprocs, myrank, i, incr;
    ADIO_Offset shared_fp;

    if ((fh <= (MPI_File) 0) || (fh->cookie != ADIOI_FILE_COOKIE)) {
	printf("MPI_File_read_ordered: Invalid file handle\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (count < 0) {
	printf("MPI_File_read_ordered: Invalid count argument\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (datatype == MPI_DATATYPE_NULL) {
        printf("MPI_File_read_ordered: Invalid datatype\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Type_size(datatype, &datatype_size);
    if ((count*datatype_size) % fh->etype_size != 0) {
        printf("MPI_File_read_ordered: Only an integral number of etypes can be accessed\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if ((fh->file_system == ADIO_PIOFS) || (fh->file_system == ADIO_PVFS)) {
	printf("MPI_File_read_ordered: Shared file pointer not supported on PIOFS and PVFS\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_size(fh->comm, &nprocs);
    MPI_Comm_rank(fh->comm, &myrank);

    incr = (count*datatype_size)/fh->etype_size;
    for (i=0; i<nprocs; i++) {
	if (i == myrank) {
	    ADIO_Get_shared_fp(fh, incr, &shared_fp, &error_code);
	    if (error_code != MPI_SUCCESS) {
		printf("MPI_File_read_ordered: Could not access shared file pointer!\n");
		MPI_Abort(MPI_COMM_WORLD, 1);
	    }
	}
	MPI_Barrier(fh->comm);
    }

    ADIO_ReadStridedColl(fh, buf, count, datatype, ADIO_EXPLICIT_OFFSET,
			 shared_fp, status, &error_code);

    return error_code;
}
