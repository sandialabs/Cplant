/* 
 *   $Id: get_size.c,v 1.1 2000/05/10 21:44:00 jrjohns Exp $    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
//#pragma weak MPI_File_get_size = PMPI_File_get_size
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
//#pragma _HP_SECONDARY_DEF PMPI_File_get_size MPI_File_get_size
#elif defined(HAVE_PRAGMA_CRI_DUP)
//#pragma _CRI duplicate MPI_File_get_size as PMPI_File_get_size
/* end of weak pragmas */
#endif

/* Include mapping from MPI->PMPI */
//#define __MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

/*@
    MPI_File_get_size - Returns the file size

Input Parameters:
. fh - file handle (handle)

Output Parameters:
. size - size of the file in bytes (nonnegative integer)

.N fortran
@*/
int MPI_File_get_size(MPI_File fh, MPI_Offset *size)
{
    ADIO_Fcntl_t *fcntl_struct;
    int error_code;
#ifdef MPI_hpux
    int fl_xmpi;

    HPMP_IO_START(fl_xmpi, BLKMPIFILEGETSIZE, TRDTBLOCK, fh,
		  MPI_DATATYPE_NULL, -1);
#endif /* MPI_hpux */

    if ((fh <= (MPI_File) 0) || (fh->cookie != ADIOI_FILE_COOKIE)) {
	printf("MPI_File_get_size: Invalid file handle\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    fcntl_struct = (ADIO_Fcntl_t *) ADIOI_Malloc(sizeof(ADIO_Fcntl_t));
    ADIO_Fcntl(fh, ADIO_FCNTL_GET_FSIZE, fcntl_struct, &error_code);
    *size = fcntl_struct->fsize;
    ADIOI_Free(fcntl_struct);

#ifdef MPI_hpux
    HPMP_IO_END(fl_xmpi, fh, MPI_DATATYPE_NULL, -1);
#endif /* MPI_hpux */
    return error_code;
}
