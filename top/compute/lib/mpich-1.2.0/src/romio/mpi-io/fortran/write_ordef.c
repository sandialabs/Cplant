/* 
 *   $Id: write_ordef.c,v 1.1 2000/05/10 21:44:23 jrjohns Exp $    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpio.h"
#include "adio.h"


#if defined(__MPIO_BUILD_PROFILING) || defined(HAVE_WEAK_SYMBOLS)
#ifdef FORTRANCAPS
#define mpi_file_write_ordered_end_ PMPI_FILE_WRITE_ORDERED_END
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_write_ordered_end_ pmpi_file_write_ordered_end__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_write_ordered_end pmpi_file_write_ordered_end_
#endif
#define mpi_file_write_ordered_end_ pmpi_file_write_ordered_end
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_write_ordered_end_ pmpi_file_write_ordered_end
#endif
#define mpi_file_write_ordered_end_ pmpi_file_write_ordered_end_
#endif

#if defined(HAVE_WEAK_SYMBOLS)
#if defined(HAVE_PRAGMA_WEAK)
#if defined(FORTRANCAPS)
#pragma weak MPI_FILE_WRITE_ORDERED_END = PMPI_FILE_WRITE_ORDERED_END
#elif defined(FORTRANDOUBLEUNDERSCORE)
#pragma weak mpi_file_write_ordered_end__ = pmpi_file_write_ordered_end__
#elif !defined(FORTRANUNDERSCORE)
#pragma weak mpi_file_write_ordered_end = pmpi_file_write_ordered_end
#else
#pragma weak mpi_file_write_ordered_end_ = pmpi_file_write_ordered_end_
#endif

#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#if defined(FORTRANCAPS)
#pragma _HP_SECONDARY_DEF PMPI_FILE_WRITE_ORDERED_END MPI_FILE_WRITE_ORDERED_END
#elif defined(FORTRANDOUBLEUNDERSCORE)
#pragma _HP_SECONDARY_DEF pmpi_file_write_ordered_end__ mpi_file_write_ordered_end__
#elif !defined(FORTRANUNDERSCORE)
#pragma _HP_SECONDARY_DEF pmpi_file_write_ordered_end mpi_file_write_ordered_end
#else
#pragma _HP_SECONDARY_DEF pmpi_file_write_ordered_end_ mpi_file_write_ordered_end_
#endif

#elif defined(HAVE_PRAGMA_CRI_DUP)
#if defined(FORTRANCAPS)
#pragma _CRI duplicate MPI_FILE_WRITE_ORDERED_END as PMPI_FILE_WRITE_ORDERED_END
#elif defined(FORTRANDOUBLEUNDERSCORE)
#pragma _CRI duplicate mpi_file_write_ordered_end__ as pmpi_file_write_ordered_end__
#elif !defined(FORTRANUNDERSCORE)
#pragma _CRI duplicate mpi_file_write_ordered_end as pmpi_file_write_ordered_end
#else
#pragma _CRI duplicate mpi_file_write_ordered_end_ as pmpi_file_write_ordered_end_
#endif

/* end of weak pragmas */
#endif
/* Include mapping from MPI->PMPI */
#include "mpioprof.h"
#endif

#else

#ifdef FORTRANCAPS
#define mpi_file_write_ordered_end_ MPI_FILE_WRITE_ORDERED_END
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_write_ordered_end_ mpi_file_write_ordered_end__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_write_ordered_end mpi_file_write_ordered_end_
#endif
#define mpi_file_write_ordered_end_ mpi_file_write_ordered_end
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_write_ordered_end_ mpi_file_write_ordered_end
#endif
#endif
#endif

void mpi_file_write_ordered_end_(MPI_Fint *fh,void *buf,MPI_Status *status, int *__ierr ){
    MPI_File fh_c;
    
    fh_c = MPI_File_f2c(*fh);

    *__ierr = MPI_File_write_ordered_end(fh_c,buf,status);
}
