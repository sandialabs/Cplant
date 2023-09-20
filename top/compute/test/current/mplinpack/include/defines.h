/*
#define COMPLEX
*/
#undef DEBUG
#undef TURBO
#define PROCS 1
#define  OVERLAP
#undef  PRINT_STATUS
#define  TIMING0 
#undef BLAS_LONG_INTS
void *MALLOC_8(size_t size, char** ptr);
void *MALLOC_32(size_t size, char** ptr);
void *MALLOC_64(size_t size, char** ptr);

/*
** Do BLAS library calls expect long ints or regular ints.
** Do MPI library calls expect long ints or regular ints.
**
** It matters since long and int can be different sizes
** on some architectures.
*/
#ifdef BLAS_LONG_INTS
 
typedef long BLAS_INT;
 
#else
 
typedef int BLAS_INT;
 
#endif

#ifdef COMPLEX
typedef struct {
      double r;
      double i;
} dcomplex ;

#define DATA_TYPE dcomplex
#define CONST_ONE {1.0, 0.0}
#define CONST_MINUS_ONE {-1.0, 0.0}
#define CONST_ZERO {0.0, 0.0}
#define NEGATIVE(X,Y) (Y).r=-(X).r;(Y).i=-(X).i
#define ABS_VAL(X) ((X).r * (X).r + (X).i * (X).i)
#define INVERSE(X,W,Z) (Z).r=(X).r/(W);(Z).i=-(X).i/(W)
#define MULTIPLY(X,Y,Z) (Z).r=(X).r*(Y).r-(X).i*(Y).i;(Z).i=(X).r*(Y).i+(X).i*(Y).r
#define DIVIDE(X,Y,W,Z) (Z).r=((X).r*(Y).r+(X).i*(Y).i)/(W);(Z).i=((X).i*(Y).r-(X).r*(Y).i)/(W)

#include "zblas.sp.h"
#define XGEMM_  zgemm_
#define XGEMM  zgemm

#define XLU_ zlu_

#if (PROCS == 2)
#define XGEMM_ init_fp(); zgemm_
#define XGEMM init_fp(); zgemm
#endif

#if (PROCS == 1)
#define XMM_ zmm1_
#define XMM zmm1
#endif
#if (PROCS == 2)
#define XMM_ init_fp();  zmm2_
#define XMM init_fp(); zmm2
#endif
#if (PROCS == 3)
#define XMM_ init_fp(); zmm3_
#define XMM init_fp();  zmm3
#endif

#else

#define DATA_TYPE double
#define CONST_ONE 1.0
#define CONST_MINUS_ONE -1.0
#define CONST_ZERO 0.0
#define NEGATIVE(X,Y) (Y)=-(X)
#define ABS_VAL(X) (fabs(X))
#define INVERSE(X,W,Z) (Z)=CONST_ONE/(X)
#define MULTIPLY(X,Y,Z) (Z)=(X)*(Y)
#define DIVIDE(X,Y,W,Z) (Z)=(X)/(Y)

#define BY_REFERENCE
#ifdef BY_REFERENCE

#define XCOPY(n, dx, incx, dy, incy) dcopy_(&(n), dx, &(incx), dy, &(incy))
#define XSCAL(n, da, dx, incx)       dscal_(&(n), &(da), dx, &(incx)) 
#define XAXPY(n, da, dx, incx, dy, incy)  daxpy_(&(n),&(da),dx, &(incx),dy,&(incy))
#define IXAMAX(n, dx, incx)          idamax_(&(n), dx, &(incx))
#define XASUM(n, dx, incx)           dasum_(&(n), dx, &(incx))
#define XDOT(n, dx, incx, dy, incy)  ddot_(&(n), dx, &(incx), dy, &(incy))
#define XGEMM(flagA,flagB,l1,l2,l3,scal1,A,s1,B,s2,scal2,C,s3) dgemm(&flagA,&flagB,&l1,&l2,&l3,&scal1,A,&s1,B,&s2,&scal2,C,&s3)
/* #define XGEMM_  dgemmf_ */
#define XGEMM_  dgemm_

#else

#define XCOPY dcopy_
#define XSCAL dscal_
#define XAXPY daxpy_
#define IXAMAX idamax_
#define XASUM  dasum_
#define XDOT  ddot_
#define XGEMM ATL_dgemm
#endif

#define XLU_ dlu_
#if (PROCS == 1)
#define XMM_ dmm1_
#define XMM dmm1
#endif
#if (PROCS == 2)
#define XMM_ dmm2_
#define XMM dmm2
#endif
#if (PROCS == 3)
#define XMM_ dmm3_
#define XMM dmm3
#endif

#endif

