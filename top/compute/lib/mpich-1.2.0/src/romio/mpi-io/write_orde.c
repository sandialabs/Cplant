/* 
 *   $Id: write_orde.c,v 1.1 2000/05/10 21:44:02 jrjohns Exp $    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
//#pragma weak MPI_File_write_ordered_end = PMPI_File_write_ordered_end
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
//#pragma _HP_SECONDARY_DEF PMPI_File_write_ordered_end MPI_File_write_ordered_end
#elif defined(HAVE_PRAGMA_CRI_DUP)
//#pragma _CRI duplicate MPI_File_write_ordered_end as PMPI_File_write_ordered_end
/* end of weak pragmas */
#endif

/* Include mapping from MPI->PMPI */
//#define __MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

/*@
    MPI_File_write_ordered_end - Complete a split collective write using shared file pointer

Input Parameters:
. fh - file handle (handle)

Output Parameters:
. buf - initial address of buffer (choice)
. status - status object (Status)

.N fortran
@*/
int MPI_File_write_ordered_end(MPI_File fh, void *buf, MPI_Status *status)
{

    if ((fh <= (MPI_File) 0) || (fh->cookie != ADIOI_FILE_COOKIE)) {
	printf("MPI_File_write_ordered_end: Invalid file handle\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (!(fh->split_coll_count)) {
        printf("MPI_File_write_ordered_end: Does not match a previous MPI_File_write_ordered_begin\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    fh->split_coll_count = 0;

    return MPI_SUCCESS;
}
