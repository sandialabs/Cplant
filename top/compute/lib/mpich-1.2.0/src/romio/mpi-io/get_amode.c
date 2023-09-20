/* 
 *   $Id: get_amode.c,v 1.1 2000/05/10 21:43:59 jrjohns Exp $    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
//#pragma weak MPI_File_get_amode = PMPI_File_get_amode
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
//#pragma _HP_SECONDARY_DEF PMPI_File_get_amode MPI_File_get_amode
#elif defined(HAVE_PRAGMA_CRI_DUP)
//#pragma _CRI duplicate MPI_File_get_amode as PMPI_File_get_amode
/* end of weak pragmas */
#endif

/* Include mapping from MPI->PMPI */
//#define __MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

/*@
    MPI_File_get_amode - Returns the file access mode

Input Parameters:
. fh - file handle (handle)

Output Parameters:
. amode - access mode (integer)

.N fortran
@*/
int MPI_File_get_amode(MPI_File fh, int *amode)
{
    if ((fh <= (MPI_File) 0) || (fh->cookie != ADIOI_FILE_COOKIE)) {
	printf("MPI_File_get_amode: Invalid file handle\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    *amode = fh->access_mode;
    return MPI_SUCCESS;
}
