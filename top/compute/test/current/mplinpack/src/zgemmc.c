/*
 * This routine caluculates C = C-A*B.  It assumes that transa = 'N',
 * transb = 'N', alpha = (-1,0) and beta = (1,0)
 */

#include <math.h>
#include "defines.h"
#include "zgemmc.h"


void zgemmc(char *transa, char *transb, int *m, int *n, int *k, dcomplex *alpha,
            dcomplex *A, int *lda, dcomplex *B, int *ldb,
            dcomplex *beta, dcomplex *C, int *ldc)
{

  register int i1,j1,k1;
  register int strideA = *lda;
  register int strideB = *ldb;
  register int strideC = *ldc;
  register int sizem = *m;
  register int sizen = *n;
  register int sizek = *k;

  dcomplex *p;
  dcomplex *pc;
  dcomplex *pr;

  double p1cr;
  double p1ci;
  double p1rr;
  double p1ri;

  register double p1r;
  register double p1i;

  for(j1=0; j1<sizen; j1++) {
    p = C + j1*strideA;
    for (i1=0; i1<sizem; i1++) {
      pc = A + i1;
      pr = B + j1*strideB;
      p1r = (*p).r;
      p1i = (*p).i;
      for (k1=sizek; k1; k1--) {

	p1cr = (*pc).r;
	p1ci = (*pc).i;
	p1rr = (*pr).r;
	p1ri = (*pr).i;

	p1r = p1r - p1cr*p1rr + p1ci*p1ri;
	p1i = p1i - p1cr*p1ri - p1ci*p1rr;

        pc += strideA;
        pr++;
      }

      (*p).r = p1r;
      (*p).i = p1i;
      p++;
    }
  }
}

