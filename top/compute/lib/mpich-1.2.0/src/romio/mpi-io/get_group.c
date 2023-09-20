/* 
 *   $Id: get_group.c,v 1.1 2000/05/10 21:44:00 jrjohns Exp $    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
//#pragma weak MPI_File_get_group = PMPI_File_get_group
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
//#pragma _HP_SECONDARY_DEF PMPI_File_get_group MPI_File_get_group
#elif defined(HAVE_PRAGMA_CRI_DUP)
//#pragma _CRI duplicate MPI_File_get_group as PMPI_File_get_group
/* end of weak pragmas */
#endif

/* Include mapping from MPI->PMPI */
//#define __MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

/*@
    MPI_File_get_group - Returns the group of processes that 
                         opened the file

Input Parameters:
. fh - file handle (handle)

Output Parameters:
. group - group that opened the file (handle)

.N fortran
@*/
int MPI_File_get_group(MPI_File fh, MPI_Group *group)
{
    if ((fh <= (MPI_File) 0) || (fh->cookie != ADIOI_FILE_COOKIE)) {
	printf("MPI_File_get_group: Invalid file handle\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    return MPI_Comm_group(fh->comm, group);
}
