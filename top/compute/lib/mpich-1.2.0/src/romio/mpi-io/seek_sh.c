/* 
 *   $Id: seek_sh.c,v 1.1 2000/05/10 21:44:02 jrjohns Exp $    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
//#pragma weak MPI_File_seek_shared = PMPI_File_seek_shared
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
//#pragma _HP_SECONDARY_DEF PMPI_File_seek_shared MPI_File_seek_shared
#elif defined(HAVE_PRAGMA_CRI_DUP)
//#pragma _CRI duplicate MPI_File_seek_shared as PMPI_File_seek_shared
/* end of weak pragmas */
#endif

/* Include mapping from MPI->PMPI */
//#define __MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

/*@
    MPI_File_seek_shared - Updates the shared file pointer

Input Parameters:
. fh - file handle (handle)
. offset - file offset (integer)
. whence - update mode (state)

.N fortran
@*/
int MPI_File_seek_shared(MPI_File fh, MPI_Offset offset, int whence)
{
    int error_code=MPI_SUCCESS, tmp_whence, myrank;
    MPI_Offset curr_offset, eof_offset, tmp_offset;

    if ((fh <= (MPI_File) 0) || (fh->cookie != ADIOI_FILE_COOKIE)) {
	printf("MPI_File_seek_shared: Invalid file handle\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (fh->access_mode & MPI_MODE_SEQUENTIAL) {
        printf("MPI_File_seek_shared: Can't use this function because file was opened with MPI_MODE_SEQUENTIAL\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if ((fh->file_system == ADIO_PIOFS) || (fh->file_system == ADIO_PVFS)) {
	printf("MPI_File_seek_shared: Shared file pointer not supported on PIOFS and PVFS\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    tmp_offset = offset;
    MPI_Bcast(&tmp_offset, 1, ADIO_OFFSET, 0, fh->comm);
    if (tmp_offset != offset) {
        printf("MPI_File_seek_shared: offset must be the same on all processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    tmp_whence = whence;
    MPI_Bcast(&tmp_whence, 1, MPI_INT, 0, fh->comm);
    if (tmp_whence != whence) {
        printf("MPI_File_seek_shared: whence argument must be the same on all processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_rank(fh->comm, &myrank);

    if (!myrank) {
	switch(whence) {
	case MPI_SEEK_SET:
	    if (offset < 0) {
		printf("MPI_File_seek_shared: Invalid offset argument\n");
		MPI_Abort(MPI_COMM_WORLD, 1);
	    }
	    break;
	case MPI_SEEK_CUR:
	    /* get current location of shared file pointer */
	    ADIO_Get_shared_fp(fh, 0, &curr_offset, &error_code);
	    if (error_code != MPI_SUCCESS) {
		printf("MPI_File_seek_shared: Could not access shared file pointer!\n");
		MPI_Abort(MPI_COMM_WORLD, 1);
	    }
	    offset += curr_offset;
	    if (offset < 0) {
		printf("MPI_File_seek_shared: offset points to a negative location in the file\n");
		MPI_Abort(MPI_COMM_WORLD, 1);
	    }
	    break;
	case MPI_SEEK_END:
	    /* find offset corr. to end of file */
	    ADIOI_Get_eof_offset(fh, &eof_offset);
	    offset += eof_offset;
	    if (offset < 0) {
		printf("MPI_File_seek_shared: offset points to a negative location in the file\n");
		MPI_Abort(MPI_COMM_WORLD, 1);
	    }
	    break;
	default:
	    printf("MPI_File_seek_shared: Invalid whence argument\n");
	    MPI_Abort(MPI_COMM_WORLD, 1);
	}

	ADIO_Set_shared_fp(fh, offset, &error_code);
    }

    MPI_Barrier(fh->comm);

    return error_code;
}
