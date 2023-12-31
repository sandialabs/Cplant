/* create_recv.c */
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
#pragma weak MPI_RECV_INIT = PMPI_RECV_INIT
EXPORT_MPI_API void MPI_RECV_INIT ( void *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint * );
#elif defined(FORTRANDOUBLEUNDERSCORE)
#pragma weak mpi_recv_init__ = pmpi_recv_init__
EXPORT_MPI_API void mpi_recv_init__ ( void *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint * );
#elif !defined(FORTRANUNDERSCORE)
#pragma weak mpi_recv_init = pmpi_recv_init
EXPORT_MPI_API void mpi_recv_init ( void *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint * );
#else
#pragma weak mpi_recv_init_ = pmpi_recv_init_
EXPORT_MPI_API void mpi_recv_init_ ( void *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint * );
#endif

#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#if defined(FORTRANCAPS)
#pragma _HP_SECONDARY_DEF PMPI_RECV_INIT  MPI_RECV_INIT
#elif defined(FORTRANDOUBLEUNDERSCORE)
#pragma _HP_SECONDARY_DEF pmpi_recv_init__  mpi_recv_init__
#elif !defined(FORTRANUNDERSCORE)
#pragma _HP_SECONDARY_DEF pmpi_recv_init  mpi_recv_init
#else
#pragma _HP_SECONDARY_DEF pmpi_recv_init_  mpi_recv_init_
#endif

#elif defined(HAVE_PRAGMA_CRI_DUP)
#if defined(FORTRANCAPS)
#pragma _CRI duplicate MPI_RECV_INIT as PMPI_RECV_INIT
#elif defined(FORTRANDOUBLEUNDERSCORE)
#pragma _CRI duplicate mpi_recv_init__ as pmpi_recv_init__
#elif !defined(FORTRANUNDERSCORE)
#pragma _CRI duplicate mpi_recv_init as pmpi_recv_init
#else
#pragma _CRI duplicate mpi_recv_init_ as pmpi_recv_init_
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
#define mpi_recv_init_ PMPI_RECV_INIT
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_recv_init_ pmpi_recv_init__
#elif !defined(FORTRANUNDERSCORE)
#define mpi_recv_init_ pmpi_recv_init
#else
#define mpi_recv_init_ pmpi_recv_init_
#endif

#else

#ifdef FORTRANCAPS
#define mpi_recv_init_ MPI_RECV_INIT
#elif defined(FORTRANDOUBLEUNDERSCORE)
#define mpi_recv_init_ mpi_recv_init__
#elif !defined(FORTRANUNDERSCORE)
#define mpi_recv_init_ mpi_recv_init
#endif
#endif


#ifdef _CRAY
#ifdef _TWO_WORD_FCD
#define NUMPARAMS  8

 void mpi_recv_init_( void *unknown, ...)
{
void         *buf;
int*count;
MPI_Request  *request;
MPI_Datatype *datatype;
int*source;
int*tag;
MPI_Comm     *comm;
int *__ierr;
int	    buflen;
va_list ap;
MPI_Request 	lrequest;

va_start(ap, unknown);
buf = unknown;
if (_numargs() == NUMPARAMS+1) {
        buflen = (va_arg(ap, int)) / 8;          /* This is in bits. */
}
count =         va_arg (ap, int *);
datatype =      va_arg(ap, MPI_Datatype *);
source =          va_arg(ap, int *);
tag =           va_arg(ap, int *);
comm =          va_arg(ap, MPI_Comm *);
request =	va_arg(ap, MPI_Request *);
__ierr =        va_arg(ap, int *);


*__ierr = MPI_Recv_init(MPIR_F_PTR(buf),*count,	*datatype,*source,*tag,
	*comm,&lrequest);
*(int*)request = MPIR_FromPointer( lrequest );
}

#else

 void mpi_recv_init_( buf, count, datatype, source, tag, comm, request, __ierr )
void         *buf;
int*count;
MPI_Request  *request;
MPI_Datatype *datatype;
int*source;
int*tag;
MPI_Comm     *comm;
int *__ierr;
{

MPI_Request lrequest;
_fcd temp;
if (_isfcd(buf)) {
	temp = _fcdtocp(buf);
	buf = (void *)temp;
}
*__ierr = MPI_Recv_init(MPIR_F_PTR(buf),*count,	*datatype,*source,*tag,
	*comm,&lrequest);
*(int*)request = MPIR_FromPointer( lrequest );
}

#endif
#else

/* Prototype to suppress warnings about missing prototypes */
EXPORT_MPI_API void mpi_recv_init_ ANSI_ARGS(( void *, MPI_Fint *, MPI_Fint *, MPI_Fint *, 
                                MPI_Fint *, MPI_Fint *, MPI_Fint *, 
                                MPI_Fint * ));
EXPORT_MPI_API void mpi_recv_init_( void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *__ierr )
{
    MPI_Request lrequest;
    *__ierr = MPI_Recv_init(MPIR_F_PTR(buf),(int)*count,
                            MPI_Type_f2c(*datatype),(int)*source,(int)*tag,
			    MPI_Comm_f2c(*comm),&lrequest);
    *request = MPI_Request_c2f(lrequest);
}
#endif
