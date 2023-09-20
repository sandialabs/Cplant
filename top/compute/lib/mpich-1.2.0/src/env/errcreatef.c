/* errcreate.c */
/* Custom Fortran interface file */
#include "mpiimpl.h"


#if defined(MPI_BUILD_PROFILING) || defined(HAVE_WEAK_SYMBOLS)

#if defined(HAVE_WEAK_SYMBOLS)
#if defined(HAVE_PRAGMA_WEAK)
#if defined(FORTRANCAPS)
#pragma weak MPI_ERRHANDLER_CREATE = PMPI_ERRHANDLER_CREATE
#ifdef FORTRAN_SPECIAL_FUNCTION_PTR
EXPORT_MPI_API void MPI_ERR_HANDLER_CREATE ( MPI_Handler_function **, MPI_Fint *, MPI_Fint * );
#else
EXPORT_MPI_API void MPI_ERRHANDLER_CREATE ( MPI_Handler_function *, MPI_Fint *, MPI_Fint * );
#endif
#elif defined(FORTRANDOUBLEUNDERSCORE)
#pragma weak mpi_errhandler_create__ = pmpi_errhandler_create__
#ifdef FORTRAN_SPECIAL_FUNCTION_PTR
EXPORT_MPI_API void mpi_errhandler_create__ ( MPI_Handler_function **, MPI_Fint *, MPI_Fint * );
#else
EXPORT_MPI_API void mpi_errhandler_create__ ( MPI_Handler_function *, MPI_Fint *, MPI_Fint * );
#endif
#elif !defined(FORTRANUNDERSCORE)
#pragma weak mpi_errhandler_create = pmpi_errhandler_create
#ifdef FORTRAN_SPECIAL_FUNCTION_PTR
EXPORT_MPI_API void mpi_errhandler_create ( MPI_Handler_function **, MPI_Fint *, MPI_Fint * );
#else
EXPORT_MPI_API void mpi_errhandler_create ( MPI_Handler_function *, MPI_Fint *, MPI_Fint * );
#endif
#else
#pragma weak mpi_errhandler_create_ = pmpi_errhandler_create_
#ifdef FORTRAN_SPECIAL_FUNCTION_PTR
EXPORT_MPI_API void mpi_errhandler_create_ ( MPI_Handler_function **, MPI_Fint *, MPI_Fint * );
#else
EXPORT_MPI_API void mpi_errhandler_create_ ( MPI_Handler_function *, MPI_Fint *, MPI_Fint * );
#endif
#endif

#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#if defined(FORTRANCAPS)
#pragma _HP_SECONDARY_DEF PMPI_ERRHANDLER_CREATE  MPI_ERRHANDLER_CREATE
#elif defined(FORTRANDOUBLEUNDERSCORE)
#pragma _HP_SECONDARY_DEF pmpi_errhandler_create__  mpi_errhandler_create__
#elif !defined(FORTRANUNDERSCORE)
#pragma _HP_SECONDARY_DEF pmpi_errhandler_create  mpi_errhandler_create
#else
#pragma _HP_SECONDARY_DEF pmpi_errhandler_create_  mpi_errhandler_create_
#endif

#elif defined(HAVE_PRAGMA_CRI_DUP)
#if defined(FORTRANCAPS)
#pragma _CRI duplicate MPI_ERRHANDLER_CREATE as PMPI_ERRHANDLER_CREATE
#elif defined(FORTRANDOUBLEUNDERSCORE)
#pragma _CRI duplicate mpi_errhandler_create__ as pmpi_errhandler_create__
#elif !defined(FORTRANUNDERSCORE)
#pragma _CRI duplicate mpi_errhandler_create as pmpi_errhandler_create
#else
#pragma _CRI duplicate mpi_errhandler_create_ as pmpi_errhandler_create_
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
#define mpi_errhandler_create_ PMPI_ERRHANDLER_CREATE
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_errhandler_create_ pmpi_errhandler_create__
#elif !defined(FORTRANUNDERSCORE)
#define mpi_errhandler_create_ pmpi_errhandler_create
#else
#define mpi_errhandler_create_ pmpi_errhandler_create_
#endif

#else

#ifdef FORTRANCAPS
#define mpi_errhandler_create_ MPI_ERRHANDLER_CREATE
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_errhandler_create_ mpi_errhandler_create__
#elif !defined(FORTRANUNDERSCORE)
#define mpi_errhandler_create_ mpi_errhandler_create
#endif
#endif


/* Prototype to suppress warnings about missing prototypes */
#ifdef FORTRAN_SPECIAL_FUNCTION_PTR
EXPORT_MPI_API void mpi_errhandler_create_ ANSI_ARGS(( MPI_Handler_function **, 
					MPI_Fint *, MPI_Fint * ));
#else
EXPORT_MPI_API void mpi_errhandler_create_ ANSI_ARGS(( MPI_Handler_function *, 
					MPI_Fint *, MPI_Fint * ));
#endif

EXPORT_MPI_API void mpi_errhandler_create_(
#ifdef FORTRAN_SPECIAL_FUNCTION_PTR
	MPI_Handler_function **function,
#else
	MPI_Handler_function *function,
#endif
	MPI_Fint *errhandler, MPI_Fint *__ierr)
{

    MPI_Errhandler l_errhandler;
#ifdef FORTRAN_SPECIAL_FUNCTION_PTR
    *__ierr = MPI_Errhandler_create( *function, &l_errhandler );
#else
    *__ierr = MPI_Errhandler_create( function, &l_errhandler );
#endif
    *errhandler = MPI_Errhandler_c2f(l_errhandler);
}
