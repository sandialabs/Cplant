/* 
 *   $Id: deletef.c,v 1.1 2000/05/10 21:44:20 jrjohns Exp $    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#if _UNICOS
#include <fortran.h>
#endif
#include "mpio.h"
#include "adio.h"


#if defined(__MPIO_BUILD_PROFILING) || defined(HAVE_WEAK_SYMBOLS)
#ifdef FORTRANCAPS
#define mpi_file_delete_ PMPI_FILE_DELETE
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_delete_ pmpi_file_delete__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_delete pmpi_file_delete_
#endif
#define mpi_file_delete_ pmpi_file_delete
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF pmpi_file_delete_ pmpi_file_delete
#endif
#define mpi_file_delete_ pmpi_file_delete_
#endif

#if defined(HAVE_WEAK_SYMBOLS)
#if defined(HAVE_PRAGMA_WEAK)
#if defined(FORTRANCAPS)
#pragma weak MPI_FILE_DELETE = PMPI_FILE_DELETE
#elif defined(FORTRANDOUBLEUNDERSCORE)
#pragma weak mpi_file_delete__ = pmpi_file_delete__
#elif !defined(FORTRANUNDERSCORE)
#pragma weak mpi_file_delete = pmpi_file_delete
#else
#pragma weak mpi_file_delete_ = pmpi_file_delete_
#endif

#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#if defined(FORTRANCAPS)
#pragma _HP_SECONDARY_DEF PMPI_FILE_DELETE MPI_FILE_DELETE
#elif defined(FORTRANDOUBLEUNDERSCORE)
#pragma _HP_SECONDARY_DEF pmpi_file_delete__ mpi_file_delete__
#elif !defined(FORTRANUNDERSCORE)
#pragma _HP_SECONDARY_DEF pmpi_file_delete mpi_file_delete
#else
#pragma _HP_SECONDARY_DEF pmpi_file_delete_ mpi_file_delete_
#endif

#elif defined(HAVE_PRAGMA_CRI_DUP)
#if defined(FORTRANCAPS)
#pragma _CRI duplicate MPI_FILE_DELETE as PMPI_FILE_DELETE
#elif defined(FORTRANDOUBLEUNDERSCORE)
#pragma _CRI duplicate mpi_file_delete__ as pmpi_file_delete__
#elif !defined(FORTRANUNDERSCORE)
#pragma _CRI duplicate mpi_file_delete as pmpi_file_delete
#else
#pragma _CRI duplicate mpi_file_delete_ as pmpi_file_delete_
#endif

/* end of weak pragmas */
#endif
/* Include mapping from MPI->PMPI */
#include "mpioprof.h"
#endif

#else

#ifdef FORTRANCAPS
#define mpi_file_delete_ MPI_FILE_DELETE
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_file_delete_ mpi_file_delete__
#elif !defined(FORTRANUNDERSCORE)
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_delete mpi_file_delete_
#endif
#define mpi_file_delete_ mpi_file_delete
#else
#if defined(__HPUX) || defined(__SPPUX)
#pragma _HP_SECONDARY_DEF mpi_file_delete_ mpi_file_delete
#endif
#endif
#endif

#if _UNICOS
void mpi_file_delete_(_fcd filename_fcd, MPI_Fint *info, int *__ierr)
{
    char *filename = _fcdtocp(filename_fcd);
    int str_len = _fcdlen(filename_fcd);
#else
void mpi_file_delete_(char *filename, MPI_Fint *info, int *__ierr, int str_len)
{
#endif
    char *newfname;
    int real_len, i;
    MPI_Info info_c;

    info_c = MPI_Info_f2c(*info);

    /* strip trailing blanks */
    if (filename <= (char *) 0) {
        printf("MPI_File_delete: filename is an invalid address\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    for (i=str_len-1; i>=0; i--) if (filename[i] != ' ') break;
    if (i < 0) {
        printf("MPI_File_delete: filename is a blank string\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    real_len = i + 1;

    newfname = (char *) ADIOI_Malloc((real_len+1)*sizeof(char));
    strncpy(newfname, filename, real_len);
    newfname[real_len] = '\0';

    *__ierr = MPI_File_delete(newfname, info_c);

    ADIOI_Free(newfname);
}
