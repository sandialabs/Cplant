/*
 *           Automatically Tuned Linear Algebra Software v0.1beta
 *                   Written by R. Clint Whaley
 *                   (C) 1997 All Rights Reserved
 *
 *                              NOTICE
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted
 * provided that the above copyright notice appear in all copies and
 * that both the copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * Neither the University of Tennessee nor the Author make any
 * representations about the suitability of this software for any
 * purpose.  This software is provided ``as is'' without express or
 * implied warranty.
 *
 */

#ifndef GMM_H
#define GMM_H
#define DREAL
#if defined(SREAL)
#define EPS 5.0e-7
#define TYPE float
#define PRE s
#define PATL ATL_s
#include "smm.h"
#include "sXover.h"
#elif defined(DREAL)
#define EPS 1.0e-15
#define TYPE double
#define PRE d
#define PATL ATL_d
#include "dmm.h"
#include "dXover.h"
#elif defined(SCPLX)
   #define EPS 5.0e-7
   #define TYPE float
   #define PRE c
   #define PATL ATL_c
   #include "cmm.h"
   #include "cXover.h"
#elif defined(DCPLX)
   #define TYPE double
   #define PRE z
   #define PATL ATL_z
   #define EPS 1.0e-15
   #include "zmm.h"
   #include "zXover.h"
#endif
#if ( defined(SREAL) || defined(DREAL) )
#define TREAL
#define SHIFT
#define SCALAR TYPE
#define SADD &
#define SVAL
#define SVVAL *
#define SCALAR_IS_ONE(M_scalar) ((M_scalar) == 1.0)
#define SCALAR_IS_NONE(M_scalar) ((M_scalar) == -1.0)
#define SCALAR_IS_ZERO(M_scalar) ((M_scalar) == 0.0)
#else
   #define TCPLX
   #define CMULT(a, b, c) \
   { \
      *(c) = *(a) * *(b) - *(a+1) * *(b+1); \
      *(c+1) = *(a) * *(b+1) + *(a+1) * *(b); \
   }
   #define CMULTR(a,b) ( *(a) * *(b) - *(a+1) * *(b+1) )
   #define CMULTI(a,b) ( *(a) * *(b+1) + *(a+1) * *(b) )
/*
 * c = b*c + v;
 */
   #define CMULT2(v, a, b, tmp) \
   { \
      tmp = *(a) * *(b) - *(a+1) * *(b+1); \
      *(b+1) = *(a) * *(b+1) + *(a+1) * *(b) + *(v+1); \
      *(b) = tmp + *v; \
   }
/*\
 * b = a*b;\
 */\
#define CMULT3(a, b, tmp) \
{ \
   tmp = *(a) * *(b) - *(a+1) * *(b+1); \
   *(b+1) = *(a) * *(b+1) + *(a+1) * *(b); \
   *(b) = tmp; \
}
   #define SHIFT << 1
   #define SCALAR TYPE *
   #define SADD
   #define SVAL *
   #define SVVAL
   #define SCALAR_IS_ONE(M_scalar) \
      ( (*(M_scalar) == 1.0) && ((M_scalar)[1] == 0.0) )
   #define SCALAR_IS_NONE(M_scalar) \
      ( (*(M_scalar) == -1.0) && ((M_scalar)[1] == 0.0) )
   #define SCALAR_IS_ZERO(M_scalar) \
      ( (*(M_scalar) == 0.0) && ((M_scalar)[1] == 0.0) )
#endif

#ifndef NN_CROSSOVER
#define NN_CROSSOVER (M <= NB && N <= NB && K <= NB)
#endif
#ifndef NT_CROSSOVER
#define NT_CROSSOVER (M <= NB && N <= NB && K <= NB)
#endif
#ifndef TN_CROSSOVER
#define TN_CROSSOVER (M <= NB && N <= NB && K <= NB)
#endif
#ifndef TT_CROSSOVER
#define TT_CROSSOVER (M <= NB && N <= NB && K <= NB)
#endif

#if defined(ALPHA1)
#define ALPHA
#define NM _a1
#elif defined (ALPHAN1)
#define ALPHA -
#define NM _an1
#else
#define ALPHA alpha*
#define NM _aX
#endif
#if defined(BETA1)
#define MSTAT A[i] += v[i]
#define BNM _b1
#elif defined(BETAN1)
#define MSTAT A[i] = v[i] - A[i]
#define BNM _bn1
#elif defined(BETA0)
#define MSTAT A[i] = v[i]
#define BNM _b0
#else
#define MSTAT A[i] = beta*A[i] + v[i]
#define BNM _bX
#endif

#ifndef ATL_MaxMalloc
#define ATL_MaxMalloc 8388608
#endif

#define tname(pre, nam) my_join(pre, nam)
#define my_join(pre, nam) pre ## nam

#define NBmm tname(PRE,NBmm)

#define Rabs(x) ( (x) < 0 ? (x) * -1 : (x) )
#define Rmin(x, y) ( (x) > (y) ? (y) : (x) )
#define Rmax(x, y) ( (x) > (y) ? (x) : (y) )

#define Mstr2(m) # m
#define Mstr(m) Mstr2(m)

typedef void (*MAT2BLK)(int, int, TYPE*, int, TYPE*, SCALAR);

#endif
