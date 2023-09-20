/* 
 *   $Id: write_atf.c,v 1.1 2000/05/10 21:44:22 jrjohns Exp $    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpio.h"
#include "adio.h"


#if defined(__MPIO_BUILD_PROFILING) || defined(HAVE_WEAK_SYMBOLS)
#ifdef FORTRANCAPS
#define mpi_file_write_at_ PMPI_FILE_WRITE_AT
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_write_at_ pmpi_file_write_at__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_write_at pmpi_file_write_at_
#endif
#define mpi_file_write_at_ pmpi_file_write_at
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_write_at_ pmpi_file_write_at
#endif
#define mpi_file_write_at_ pmpi_file_write_at_
#endif

#if defined(HAVE_WEAK_SYMBOLS)
#if defined(HAVE_PRAGMA_WEAK)
#if defined(FORTRANCAPS)
#pragma weak MPI_FILE_WRITE_AT = PMPI_FILE_WRITE_AT
#elif defined(FORTRANDOUBLEUNDERSCORE)
#pragma weak mpi_file_write_at__ = pmpi_file_write_at__
#elif !defined(FORTRANUNDERSCORE)
#pragma weak mpi_file_write_at = pmpi_file_write_at
#else
#pragma weak mpi_file_write_at_ = pmpi_file_write_at_
#endif

#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#if defined(FORTRANCAPS)
#pragma _HP_SECONDARY_DEF PMPI_FILE_WRITE_AT MPI_FILE_WRITE_AT
#elif defined(FORTRANDOUBLEUNDERSCORE)
#pragma _HP_SECONDARY_DEF pmpi_file_write_at__ mpi_file_write_at__
#elif !defined(FORTRANUNDERSCORE)
#pragma _HP_SECONDARY_DEF pmpi_file_write_at mpi_file_write_at
#else
#pragma _HP_SECONDARY_DEF pmpi_file_write_at_ mpi_file_write_at_
#endif

#elif defined(HAVE_PRAGMA_CRI_DUP)
#if defined(FORTRANCAPS)
#pragma _CRI duplicate MPI_FILE_WRITE_AT as PMPI_FILE_WRITE_AT
#elif defined(FORTRANDOUBLEUNDERSCORE)
#pragma _CRI duplicate mpi_file_write_at__ as pmpi_file_write_at__
#elif !defined(FORTRANUNDERSCORE)
#pragma _CRI duplicate mpi_file_write_at as pmpi_file_write_at
#else
#pragma _CRI duplicate mpi_file_write_at_ as pmpi_file_write_at_
#endif

/* end of weak pragmas */
#endif
/* Include mapping from MPI->PMPI */
#include "mpioprof.h"
#endif

#else

#ifdef FORTRANCAPS
#define mpi_file_write_at_ MPI_FILE_WRITE_AT
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_write_at_ mpi_file_write_at__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_write_at mpi_file_write_at_
#endif
#define mpi_file_write_at_ mpi_file_write_at
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_write_at_ mpi_file_write_at
#endif
#endif
#endif

#if defined(__MPIHP) || defined(__MPILAM)
void mpi_file_write_at_(MPI_Fint *fh,MPI_Offset *offset,void *buf,
                      int *count,MPI_Fint *datatype,
                      MPI_Status *status, int *__ierr )
{
    MPI_File fh_c;
    MPI_Datatype datatype_c;
    
    fh_c = MPI_File_f2c(*fh);
    datatype_c = MPI_Type_f2c(*datatype);

    *__ierr = MPI_File_write_at(fh_c,*offset,buf,*count,datatype_c,status);
}
#else
void mpi_file_write_at_(MPI_Fint *fh,MPI_Offset *offset,void *buf,
                      int *count,MPI_Datatype *datatype,
                      MPI_Status *status, int *__ierr )
{
    MPI_File fh_c;
    
    fh_c = MPI_File_f2c(*fh);
    *__ierr = MPI_File_write_at(fh_c,*offset,buf,*count,*datatype,status);
}
#endif
