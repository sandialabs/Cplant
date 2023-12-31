/*
 *  $Id: getpnamef.c,v 1.1 2000/02/18 03:24:41 rbbrigh Exp $
 *
 *  (C) 1993 by Argonne National Laboratory and Mississipi State University.
 *      See COPYRIGHT in top-level directory.
 */
  
/*
 * Update log
 * Nov 29 1996 jcownie@dolphinics.com: Use MPIR_cstr2fstr to get the blank padding right.
 */

#include "mpiimpl.h"
#ifdef _CRAY
#include <fortran.h>
#endif


#if defined(MPI_BUILD_PROFILING) || defined(HAVE_WEAK_SYMBOLS)

#if defined(HAVE_WEAK_SYMBOLS)
#if defined(HAVE_PRAGMA_WEAK)
#if defined(FORTRANCAPS)
#pragma weak MPI_GET_PROCESSOR_NAME = PMPI_GET_PROCESSOR_NAME
EXPORT_MPI_API void MPI_GET_PROCESSOR_NAME ( char *, MPI_Fint *, MPI_Fint *, MPI_Fint );
#elif defined(FORTRANDOUBLEUNDERSCORE)
#pragma weak mpi_get_processor_name__ = pmpi_get_processor_name__
EXPORT_MPI_API void mpi_get_processor_name__ ( char *, MPI_Fint *, MPI_Fint *, MPI_Fint );
#elif !defined(FORTRANUNDERSCORE)
#pragma weak mpi_get_processor_name = pmpi_get_processor_name
EXPORT_MPI_API void mpi_get_processor_name ( char *, MPI_Fint *, MPI_Fint *, MPI_Fint );
#else
#pragma weak mpi_get_processor_name_ = pmpi_get_processor_name_
EXPORT_MPI_API void mpi_get_processor_name_ ( char *, MPI_Fint *, MPI_Fint *, MPI_Fint );
#endif

#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#if defined(FORTRANCAPS)
#pragma _HP_SECONDARY_DEF PMPI_GET_PROCESSOR_NAME  MPI_GET_PROCESSOR_NAME
#elif defined(FORTRANDOUBLEUNDERSCORE)
#pragma _HP_SECONDARY_DEF pmpi_get_processor_name__  mpi_get_processor_name__
#elif !defined(FORTRANUNDERSCORE)
#pragma _HP_SECONDARY_DEF pmpi_get_processor_name  mpi_get_processor_name
#else
#pragma _HP_SECONDARY_DEF pmpi_get_processor_name_  mpi_get_processor_name_
#endif

#elif defined(HAVE_PRAGMA_CRI_DUP)
#if defined(FORTRANCAPS)
#pragma _CRI duplicate MPI_GET_PROCESSOR_NAME as PMPI_GET_PROCESSOR_NAME
#elif defined(FORTRANDOUBLEUNDERSCORE)
#pragma _CRI duplicate mpi_get_processor_name__ as pmpi_get_processor_name__
#elif !defined(FORTRANUNDERSCORE)
#pragma _CRI duplicate mpi_get_processor_name as pmpi_get_processor_name
#else
#pragma _CRI duplicate mpi_get_processor_name_ as pmpi_get_processor_name_
#endif

/* end of weak pragmas */
#endif

/* Include mapping from MPI->PMPI */
#include "mpiprof.h"
/* Insert the prototypes for the PMPI routines */
#undef __MPI_BINDINGS
#include "binding.h"
#endif

#ifdef FORTRANCAPS
#define mpi_get_processor_name_ PMPI_GET_PROCESSOR_NAME
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_get_processor_name_ pmpi_get_processor_name__
#elif !defined(FORTRANUNDERSCORE)
#define mpi_get_processor_name_ pmpi_get_processor_name
#else
#define mpi_get_processor_name_ pmpi_get_processor_name_
#endif

#else

#ifdef FORTRANCAPS
#define mpi_get_processor_name_ MPI_GET_PROCESSOR_NAME
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_get_processor_name_ mpi_get_processor_name__
#elif !defined(FORTRANUNDERSCORE)
#define mpi_get_processor_name_ mpi_get_processor_name
#endif
#endif


#define LOCAL_MIN(a,b) ((a) < (b) ? (a) : (b))

/*
  MPI_GET_PROCESSOR_NAME - Gets the name of the processor for Fortran

*/
#ifdef _CRAY
void mpi_get_processor_name_( name_fcd, len, ierr )
int *len, *ierr;
_fcd name_fcd;
{
    char *name = _fcdtocp(name_fcd);
    long reslen= _fcdlen(name_fcd);
    char cres[MPI_MAX_PROCESSOR_NAME];

    MPID_Node_name( cres, MPI_MAX_PROCESSOR_NAME );

    /* This handles blank padding required by Fortran */
    MPIR_cstr2fstr(name, reslen, cres );
    *len  = LOCAL_MIN (strlen( cres ), reslen);
    *ierr = MPI_SUCCESS;
}

#else
/* Prototype to suppress warnings about missing prototypes */
EXPORT_MPI_API void mpi_get_processor_name_ ANSI_ARGS(( char *, MPI_Fint *, 
                                         MPI_Fint *, MPI_Fint ));

EXPORT_MPI_API void mpi_get_processor_name_( char *name, MPI_Fint *len, MPI_Fint *ierr, MPI_Fint d )
{
  char cres[MPI_MAX_PROCESSOR_NAME];
  int l_len;

    MPID_Node_name( cres, MPI_MAX_PROCESSOR_NAME );

    /* This handles blank padding required by Fortran */
    MPIR_cstr2fstr( name, (int)d, cres );
    l_len  = LOCAL_MIN( strlen( cres ), (int)d );
    *len = l_len;
    *ierr = MPI_SUCCESS;
}
#endif

