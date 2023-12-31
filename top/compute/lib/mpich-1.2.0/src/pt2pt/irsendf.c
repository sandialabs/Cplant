/* irsend.c */
/* Custom Fortran interface file */
#include "mpiimpl.h"

#ifdef _CRAY
#include <fortran.h>
#include <stdarg.h>
#endif


#if defined(MPI_BUILD_PROFILING) || defined(HAVE_WEAK_SYMBOLS)

#if defined(HAVE_WEAK_SYMBOLS)
#if defined(HAVE_PRAGMA_WEAK)
#if defined(FORTRANCAPS)
#pragma weak MPI_IRSEND = PMPI_IRSEND
EXPORT_MPI_API void MPI_IRSEND ( void *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint * );
#elif defined(FORTRANDOUBLEUNDERSCORE)
#pragma weak mpi_irsend__ = pmpi_irsend__
EXPORT_MPI_API void mpi_irsend__ ( void *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint * );
#elif !defined(FORTRANUNDERSCORE)
#pragma weak mpi_irsend = pmpi_irsend
EXPORT_MPI_API void mpi_irsend ( void *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint * );
#else
#pragma weak mpi_irsend_ = pmpi_irsend_
EXPORT_MPI_API void mpi_irsend_ ( void *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint * );
#endif

#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#if defined(FORTRANCAPS)
#pragma _HP_SECONDARY_DEF PMPI_IRSEND  MPI_IRSEND
#elif defined(FORTRANDOUBLEUNDERSCORE)
#pragma _HP_SECONDARY_DEF pmpi_irsend__  mpi_irsend__
#elif !defined(FORTRANUNDERSCORE)
#pragma _HP_SECONDARY_DEF pmpi_irsend  mpi_irsend
#else
#pragma _HP_SECONDARY_DEF pmpi_irsend_  mpi_irsend_
#endif

#elif defined(HAVE_PRAGMA_CRI_DUP)
#if defined(FORTRANCAPS)
#pragma _CRI duplicate MPI_IRSEND as PMPI_IRSEND
#elif defined(FORTRANDOUBLEUNDERSCORE)
#pragma _CRI duplicate mpi_irsend__ as pmpi_irsend__
#elif !defined(FORTRANUNDERSCORE)
#pragma _CRI duplicate mpi_irsend as pmpi_irsend
#else
#pragma _CRI duplicate mpi_irsend_ as pmpi_irsend_
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
#define mpi_irsend_ PMPI_IRSEND
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_irsend_ pmpi_irsend__
#elif !defined(FORTRANUNDERSCORE)
#define mpi_irsend_ pmpi_irsend
#else
#define mpi_irsend_ pmpi_irsend_
#endif

#else

#ifdef FORTRANCAPS
#define mpi_irsend_ MPI_IRSEND
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_irsend_ mpi_irsend__
#elif !defined(FORTRANUNDERSCORE)
#define mpi_irsend_ mpi_irsend
#endif
#endif


#ifdef _CRAY
#ifdef _TWO_WORD_FCD
#define NUMPARAMS 8

 void mpi_irsend_( void *unknown, ...)
{
void             *buf;
int*count;
MPI_Datatype     *datatype;
int*dest;
int*tag;
MPI_Comm         *comm;
MPI_Request      *request;
int *__ierr;
MPI_Request lrequest;
va_list ap;
int		buflen;

va_start(ap, unknown);
buf = unknown;
if (_numargs() == NUMPARAMS+1) {
        buflen = va_arg(ap, int) /8;          /* This is in bits. */
}
count =         va_arg (ap, int *);
datatype =      va_arg(ap, MPI_Datatype*);
dest =          va_arg(ap, int *);
tag =           va_arg(ap, int *);
comm =          va_arg(ap, MPI_Comm *);
request =       va_arg(ap, MPI_Request *);
__ierr =        va_arg(ap, int *);

*__ierr = MPI_Irsend(MPIR_F_PTR(buf),*count,*datatype,*dest,*tag,*comm,
		     &lrequest);
*(int*)request = MPIR_FromPointer(lrequest);
}

#else

 void mpi_irsend_( buf, count, datatype, dest, tag, comm, request, __ierr )
void             *buf;
int*count;
MPI_Datatype     *datatype;
int*dest;
int*tag;
MPI_Comm         *comm;
MPI_Request      *request;
int *__ierr;
{
MPI_Request lrequest;
_fcd temp;
if (_isfcd(buf)) {
	temp = _fcdtocp(buf);
	buf = (void *)temp;
}
*__ierr = MPI_Irsend(MPIR_F_PTR(buf),*count,*datatype,*dest,*tag,*comm,
		     &lrequest);
*(int*)request = MPIR_FromPointer(lrequest);
}

#endif
#else
/* Prototype to suppress warnings about missing prototypes */
EXPORT_MPI_API void mpi_irsend_ ANSI_ARGS(( void *, MPI_Fint *, MPI_Fint *, MPI_Fint *, 
                             MPI_Fint *, MPI_Fint *, MPI_Fint *, 
                             MPI_Fint * ));

EXPORT_MPI_API void mpi_irsend_( void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *__ierr )
{
MPI_Request lrequest;
*__ierr = MPI_Irsend(MPIR_F_PTR(buf),(int)*count,MPI_Type_f2c(*datatype),
                     (int)*dest,(int)*tag,
                     MPI_Comm_f2c(*comm),&lrequest);
*request = MPI_Request_c2f(lrequest);
}
#endif
