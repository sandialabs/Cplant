/* 
 *   $Id: fsyncf.c,v 1.1 2000/05/10 21:44:20 jrjohns Exp $    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpio.h"
#include "adio.h"


#if defined(__MPIO_BUILD_PROFILING) || defined(HAVE_WEAK_SYMBOLS)
#ifdef FORTRANCAPS
#define mpi_file_sync_ PMPI_FILE_SYNC
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_sync_ pmpi_file_sync__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_sync pmpi_file_sync_
#endif
#define mpi_file_sync_ pmpi_file_sync
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_sync_ pmpi_file_sync
#endif
#define mpi_file_sync_ pmpi_file_sync_
#endif

#if defined(HAVE_WEAK_SYMBOLS)
#if defined(HAVE_PRAGMA_WEAK)
#if defined(FORTRANCAPS)
#pragma weak MPI_FILE_SYNC = PMPI_FILE_SYNC
#elif defined(FORTRANDOUBLEUNDERSCORE)
#pragma weak mpi_file_sync__ = pmpi_file_sync__
#elif !defined(FORTRANUNDERSCORE)
#pragma weak mpi_file_sync = pmpi_file_sync
#else
#pragma weak mpi_file_sync_ = pmpi_file_sync_
#endif

#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#if defined(FORTRANCAPS)
#pragma _HP_SECONDARY_DEF PMPI_FILE_SYNC MPI_FILE_SYNC
#elif defined(FORTRANDOUBLEUNDERSCORE)
#pragma _HP_SECONDARY_DEF pmpi_file_sync__ mpi_file_sync__
#elif !defined(FORTRANUNDERSCORE)
#pragma _HP_SECONDARY_DEF pmpi_file_sync mpi_file_sync
#else
#pragma _HP_SECONDARY_DEF pmpi_file_sync_ mpi_file_sync_
#endif

#elif defined(HAVE_PRAGMA_CRI_DUP)
#if defined(FORTRANCAPS)
#pragma _CRI duplicate MPI_FILE_SYNC as PMPI_FILE_SYNC
#elif defined(FORTRANDOUBLEUNDERSCORE)
#pragma _CRI duplicate mpi_file_sync__ as pmpi_file_sync__
#elif !defined(FORTRANUNDERSCORE)
#pragma _CRI duplicate mpi_file_sync as pmpi_file_sync
#else
#pragma _CRI duplicate mpi_file_sync_ as pmpi_file_sync_
#endif

/* end of weak pragmas */
#endif
/* Include mapping from MPI->PMPI */
#include "mpioprof.h"
#endif

#else

#ifdef FORTRANCAPS
#define mpi_file_sync_ MPI_FILE_SYNC
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_sync_ mpi_file_sync__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_sync mpi_file_sync_
#endif
#define mpi_file_sync_ mpi_file_sync
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_sync_ mpi_file_sync
#endif
#endif
#endif

void mpi_file_sync_(MPI_Fint *fh, int *__ierr )
{
    MPI_File fh_c;
    
    fh_c = MPI_File_f2c(*fh);
    *__ierr = MPI_File_sync(fh_c);
}
