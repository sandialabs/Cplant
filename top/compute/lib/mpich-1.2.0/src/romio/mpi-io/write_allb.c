/* 
 *   $Id: write_allb.c,v 1.1 2000/05/10 21:44:02 jrjohns Exp $    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
//#pragma weak MPI_File_write_all_begin = PMPI_File_write_all_begin
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
//#pragma _HP_SECONDARY_DEF PMPI_File_write_all_begin MPI_File_write_all_begin
#elif defined(HAVE_PRAGMA_CRI_DUP)
//#pragma _CRI duplicate MPI_File_write_all_begin as PMPI_File_write_all_begin
/* end of weak pragmas */
#endif

/* Include mapping from MPI->PMPI */
//#define __MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

/*@
    MPI_File_write_all_begin - Begin a split collective write using individual file pointer

Input Parameters:
. fh - file handle (handle)
. buf - initial address of buffer (choice)
. count - number of elements in buffer (nonnegative integer)
. datatype - datatype of each buffer element (handle)

.N fortran
@*/
int MPI_File_write_all_begin(MPI_File fh, void *buf, int count, 
                            MPI_Datatype datatype)
{
    int error_code, datatype_size;
    MPI_Status status;

    if ((fh <= (MPI_File) 0) || (fh->cookie != ADIOI_FILE_COOKIE)) {
	printf("MPI_File_write_all_begin: Invalid file handle\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (count < 0) {
	printf("MPI_File_write_all_begin: Invalid count argument\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (datatype == MPI_DATATYPE_NULL) {
        printf("MPI_File_write_all_begin: Invalid datatype\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (fh->access_mode & MPI_MODE_SEQUENTIAL) {
        printf("MPI_File_write_all_begin: Can't use this function because file was opened with MPI_MODE_SEQUENTIAL\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (fh->split_coll_count) {
        printf("MPI_File_write_all_begin: Only one active split collective I/O operation allowed per file handle\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    fh->split_coll_count = 1;

    MPI_Type_size(datatype, &datatype_size);
    if ((count*datatype_size) % fh->etype_size != 0) {
        printf("MPI_File_write_all_begin: Only an integral number of etypes can be accessed\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    ADIO_WriteStridedColl(fh, buf, count, datatype, ADIO_INDIVIDUAL,
			  0, &status, &error_code);
    return error_code;
}
