
    /*        Fast GEMM routine for Alpha                  */
    /*           Linux, Digital UNIX and NT/Alpha          */
    /*                         date : 98.09.27             */
    /*        by Kazushige Goto <goto@statabo.rim.or.jp>   */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "common.h"

/*
    This routine is front end of compatibility of gemm.f.
    Checking the parameters, and initializing matrix C with BETA.
    At last, by Transposed or Non-Transposed, call subroutine
    separately.
*/

void GEMM_NN(int, int, int, FLOAT, FLOAT *, int, FLOAT* ,
	      int, FLOAT *, int);
void GEMM_TN(int, int, int, FLOAT, FLOAT *, int, FLOAT* ,
	      int, FLOAT *, int);
void GEMM_NT(int, int, int, FLOAT, FLOAT *, int, FLOAT* ,
	      int, FLOAT *, int);
void GEMM_TT(int, int, int, FLOAT, FLOAT *, int, FLOAT* ,
	      int, FLOAT *, int);

#ifndef TEST
int xerbla_(char *srname, int *info, int srname_len);
#endif

#define ZERO 0.0000
#define ONE  1.0000

/*     C := alpha * A x B + beta * C */

#ifndef C_VERSION
void GEMM_(char *TRANSA, char *TRANSB,
	    int *M, int *N, int *K,
	    FLOAT *ALPHA,
	    FLOAT *a, int *ldA,
	    FLOAT *b, int *ldB,
	    FLOAT *BETA,
	    FLOAT *c, int *ldC){
    
  int nota, notb, i, j, info, nrowa, nrowb;
  int lda, ldb, ldc, m, n, k;

  FLOAT alpha, beta;
  char transA, transB;
  FLOAT *c_offset, *cc_offset;

  FLOAT atemp1, atemp2, atemp3, atemp4;
  FLOAT atemp5, atemp6, atemp7, atemp8;
  FLOAT btemp1, btemp2, btemp3, btemp4;
  FLOAT btemp5, btemp6, btemp7, btemp8;

  int min_i, min_l;

  lda = *ldA;
  ldb = *ldB;
  ldc = *ldC;
  m   = *M;
  n   = *N;
  k   = *K;
  alpha = *ALPHA;
  beta  = *BETA;
  transA = *TRANSA;
  transB = *TRANSB;

  transA = toupper(transA);
  transB = toupper(transB);

  nota = (transA=='N');
  notb = (transB=='N');

#if 0 
printf("Information   M = %d, N = %d, K = %d\n", m,n,k);
printf("              Alpha = %f, Beta = %f  ", alpha, beta);
printf("TransA:%c  TransB:%c \n", transA, transB);
#endif

  if (nota) nrowa = m; else nrowa = k;
  if (notb) nrowb = k; else nrowb = n;
/*
*
*     Test the input parameters.
*
*/
  info = 0;

  if (!nota && (transA != 'C') && (transA != 'T')) info = 1;
  else
    if (!notb && (transB != 'C') && (transB != 'T')) info = 2;
  else
    if (m < 0) info = 3;
  else
    if (n < 0) info = 4;
  else
    if (k < 0) info = 5;
  else
    if (lda < nrowa) info = 8; 
  else
    if (ldb < nrowb) info = 10;
  else
    if (ldc < m) info = 13;

  if (info){
#ifndef TEST
#ifdef DGEMM
    xerbla_("DGEMM ", &info, 6L);
#else
    xerbla_("SGEMM ", &info, 6L);
#endif
#else
    fprintf(stderr, 
#ifdef DGEMM
	    " ** On entry to dgemm : parameter number %2d had an illegal value",
#else
	    " ** On entry to sgemm : parameter number %2d had an illegal value",
#endif
	    info);

#endif
    return;
  }


  if ((!(m)) || (!(n)) || 
      (((alpha==ZERO) || (k==0)) && (beta==ONE))) return;

  if (alpha == ZERO){
    if (beta == ZERO)
      for (j=0;j<n;j++)
	for(i=0;i<m;i++) c[ldc*j+i] = ZERO;
    else
      for (j=0;j<n;j++)
	for(i=0;i<m;i++) c[ldc*j+i] = beta*c[ldc*j+i];
    return;
  }
  
  cc_offset = c;
  min_i = (m>>3);
  min_l = (m &7);

  if (beta == ZERO){
    for (j = 0; j < n; j++) {
      c_offset = cc_offset;
      for (i = 0; i < min_i; i++) {
	*(c_offset+0) = ZERO;
	*(c_offset+1) = ZERO;
	*(c_offset+2) = ZERO;
	*(c_offset+3) = ZERO;
	*(c_offset+4) = ZERO;
	*(c_offset+5) = ZERO;
	*(c_offset+6) = ZERO;
	*(c_offset+7) = ZERO;
	c_offset +=8;
      }
      for (i = 0; i < min_l; i++) {
	*c_offset = ZERO;
	c_offset ++;
      }
      cc_offset += ldc;
    }
  } else{
    if(beta != ONE){
      for (j = 0; j < n; j++) {
	c_offset = cc_offset;
	for (i = 0; i < min_i; i++) {
	  atemp1 = *(c_offset+0);
	  atemp2 = *(c_offset+1);
	  atemp3 = *(c_offset+2);
	  atemp4 = *(c_offset+3);
	  atemp5 = *(c_offset+4);
	  atemp6 = *(c_offset+5);
	  atemp7 = *(c_offset+6);
	  atemp8 = *(c_offset+7);

	  btemp1 = beta * atemp1;
	  btemp2 = beta * atemp2;
	  btemp3 = beta * atemp3;
	  btemp4 = beta * atemp4;
	  btemp5 = beta * atemp5;
	  btemp6 = beta * atemp6;
	  btemp7 = beta * atemp7;
	  btemp8 = beta * atemp8;

	  *(c_offset+0) = btemp1;
	  *(c_offset+1) = btemp2;
	  *(c_offset+2) = btemp3;
	  *(c_offset+3) = btemp4;
	  *(c_offset+4) = btemp5;
	  *(c_offset+5) = btemp6;
	  *(c_offset+6) = btemp7;
	  *(c_offset+7) = btemp8;
	  c_offset +=8;
	}
	for (i = 0; i < min_l; i++) {
	  *c_offset = beta * *c_offset;
	  c_offset ++;
	}
	cc_offset += ldc;
      }
    }
  }

  if (notb) {
    if (nota) {
      GEMM_NN(m, n, k, alpha, a, lda, b, ldb, c, ldc);
    }
    else {
      GEMM_TN(m, n, k, alpha, a, lda, b, ldb, c, ldc);
    }
  }  
  else {
    if (nota) {
      GEMM_NT(m, n, k, alpha, a, lda, b, ldb, c, ldc);
    } 
    else {
      GEMM_TT(m, n, k, alpha, a, lda, b, ldb, c, ldc);
    }  
  }
  return;
}

#else

void GEMM(char *TRANSA, char *TRANSB,
	    int m, int n, int k,
	    FLOAT alpha,
	    FLOAT *a, int lda,
	    FLOAT *b, int ldb,
	    FLOAT beta,
	    FLOAT *c, int ldc){
    
  int nota, notb, i, j;

  char transA, transB;

  transA = *TRANSA;
  transB = *TRANSB;

  transA = toupper(transA);
  transB = toupper(transB);

  nota = (transA=='N');
  notb = (transB=='N');

#if 0
printf("Information   M = %d, N = %d, K = %d\n", m,n,k);
printf("              Alpha = %f, Beta = %f  ", alpha, beta);
printf("TransA:%c  TransB:%c \n", transA, transB);
#endif

  if (beta == ZERO){
    for (j=0;j<n;j++){
      for (i=0; i<m; i++){
	c[j*ldc+i] = ZERO;
      }
    }
  }
  else if (beta != ONE){
    for (j=0;j<n;j++){
      for (i=0; i<m; i++){
	c[j*ldc+i] *= beta;
      }
    }
  }    
  
  if (alpha==ZERO) return;
  
  if (notb) {
    if (nota) {
      GEMM_NN(m, n, k, alpha, a, lda, b, ldb, c, ldc);
    }
    else {
      GEMM_TN(m, n, k, alpha, a, lda, b, ldb, c, ldc);
    }
  }  
  else {
    if (nota) {
      GEMM_NT(m, n, k, alpha, a, lda, b, ldb, c, ldc);
    } 
    else {
      GEMM_TT(m, n, k, alpha, a, lda, b, ldb, c, ldc);
    }  
  }
  return;
}

#endif
