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
#include <stdio.h>
#include <stdlib.h>

#include "gmm.h"
/*  #include "mmaux.h"  */

#ifndef L2SIZE
#define L2SIZE 4194304
#endif
#define TREAL
double time00();

void dgemmf_(char *transA, char *transB, long *num_r, long  *num_c, long
 *colcnt,
        double *scaler, double *ptr2, long *stride, double *ptr4, long *
stride1,
        double *scaler1, double *ptr1, long *stride2);

void printmat(char *mat, int M, int N, TYPE *A, int lda)
{
   int i, j;

#ifdef TCPLX
   lda *= 2;
#endif
   printf("\n%s = \n",mat);
   for (i=0; i != M; i++)
   {
#ifdef TREAL
      for (j=0; j != N; j++) printf("%f  ",A[i+j*lda]);
#else
      for (j=0; j != N; j++) printf("(%f,%f)  ",A[2*i+j*lda], A[1+2*i+j*lda]);
#endif
      printf("\n");
   }
}

void matgen(int M, int N, TYPE *A, int lda, int seed)
{
   double drand48();
   int i, j;

   srand48(seed);
   for (j=N; j; j--)
   {
      for (i=0; i != M SHIFT; i++) A[i] = drand48();
      A += lda SHIFT;
   }
}


#define trusted_gemm(TA, TB, m, n, k, al, A, lda, B, ldb, be, C, ldc) \
   tname(PRE,gemmf_)(&TA, &TB, &m, &n, &k, SADD(al), A, &lda, B, &ldb, SADD(be), C, &ldc)

#define test_gemm(TA, TB, m, n, k, al, A, lda, B, ldb, be, C, ldc) \
   tname(PRE,gemmf_)(&TA, &TB, &m, &n, &k, SADD(al), A, &lda, B, &ldb, SADD(be), C, &ldc)

int mmcase(int TEST, char TA, char TB, int M, int N, int K, SCALAR alpha, 
           TYPE *A, int lda, TYPE *B, int ldb, SCALAR beta, TYPE *C, int ldc,
           TYPE *D, int ldd)
{
   char *pc;
#ifdef TREAL
   char *form="%4d   %c   %c %4d %4d %4d  %5.1f  %5.1f  %6.2f %6.1f %4.2f   %3s\n";
#define MALPH alpha
#define MBETA beta
#else
   #define MALPH *alpha, alpha[1]
   #define MBETA *beta, beta[1]
   char *form="%4d   %c   %c %4d %4d %4d  %5.1f %5.1f  %5.1f %5.1f  %6.2f %6.1f %4.2f   %3s\n";
#endif
   int i, j=0, PASSED;
   double t0, t1, t2, t3, mflop;
   TYPE maxval, f1, ferr;
   long  M_l,N_l,K_l,lda_l,ldb_l,ldc_l,ldd_l; 
   static TYPE feps=0.0;
   static int itst=1;
   int *L2, nL2=(1.3*L2SIZE)/sizeof(int);

   if (nL2) L2 = malloc(nL2*sizeof(int));
   if (TA == 'n' || TA == 'N') matgen(M, K, A, lda, K*1112);
   else matgen(K, M, A, lda, K*1112);
   if (TB == 'n' || TA == 'N') matgen(K, N, B, ldb, N*2238);
   else matgen(N, K, B, ldb, N*2238);
   matgen(M, N, C, ldc, M*N);

#ifdef DEBUG
   printmat("A0", M, K, A, lda);
   printmat("B0", K, N, B, ldb);
   printmat("C0", M, N, C, ldc);
#endif

   if (L2) /* invalidate L2 cache */
   {
      for (i=0; i != nL2; i++) L2[i] = 0.0; 
      for (i=0; i != nL2; i++) j += L2[i];
   }

   t0 = time00();
   M_l=M;
   N_l=N;
   K_l=K;
   lda_l=lda;
   ldb_l=ldb;
   ldc_l=ldc;
   trusted_gemm(TA, TB, M_l, N_l, K_l, alpha, A, lda_l, B, ldb_l, beta, C, ldc_l);
   t1 = time00() - t0;
   if (t1 <= 0.0) mflop = t1 = 0.0;
   else mflop = ( ((2.0*M)*N)*K ) / (t1*1000000.0);
   printf(form, itst, TA, TB, M, N, K, MALPH, MBETA, t1, mflop, 1.0, "---");

#ifdef DEBUG
   printmat("C", M, N, C, ldc);
#endif

#ifndef TIMEONLY
   matgen(M, N, D, ldd, M*N);

   if (L2)
   {
      for (i=0; i != nL2; i++) L2[i] = 0.0;  /* invalidate L2 cache */
      for (i=0; i != nL2; i++) j += L2[i];  /* invalidate L2 cache */
   }
   ldd_l=ldd;

   t0 = time00();

   /* test_gemm(TA, TB, M_l, N_l, K_l, alpha, A, lda_l, B, ldb_l, beta, D, ldd_l);   */
   
   t2 = time00() - t0;
   if (t2 <= 0.0) t2 = mflop = 0.0;
   else mflop = ( ((2.0*M)*N)*K ) / (t2*1000000.0);
#ifdef DEBUG
   printmat("D", M, N, D, ldd);
#endif
   if (TEST)
   {
      if (feps == 0.0)
      {
#if 0
         f1 = feps = 0.5;
         do
         {
            feps = f1;
            f1 *= 0.5;
            maxval = 1.0 + f1;
         }
         while (maxval != 1.0);
         printf("feps=%e\n",feps);
#else
         feps = EPS;
#endif
#ifdef DEBUG
         printf("feps=%e\n",feps);
#endif
      }
#ifdef TREAL
      ferr = 2.0 * (Rabs(alpha) * 2.0*K*feps + Rabs(beta) * feps) + feps;
#else
      f1 = Rabs(*alpha) + Rabs(alpha[1]);
      maxval = Rabs(*beta) + Rabs(beta[1]);
      ferr = 6.0 * (f1*2.0*K*feps + maxval*feps) + feps;
#endif
      PASSED = 1;
      maxval = 0.0;
      pc = "YES";
      for (j=0; j != N; j++)
      {
         for (i=0; i != M; i++)
         {
            f1 = D[i] - C[i];
            if (f1 < 0.0) f1 = -f1;
            if (f1 > ferr)
            {
               PASSED = 0;
               pc = "NO!";
               if (f1 > maxval) maxval=f1;
            }
         }
         D += ldd;
         C += ldc;
      }
      if (maxval != 0.0) fprintf(stderr, "ERROR: maxval=%e\n",maxval);
   }
   else pc = "---";
   if (t1 == t2) t3 = 1.0;
   else if (t2 != 0.0) t3 = t1/t2;
   else t3 = 0.0;
   printf(form, itst++, TA, TB, M, N, K, MALPH, MBETA, t2, mflop, t3, pc);
#else
   itst++;
   PASSED = 1;
#endif
   free(L2);
   return(PASSED);
}
___main(){}
__main(){}
MAIN__(){}
_MAIN_(){}
main(int nargs, char *args[])
/*
 *  tst <tst> <# TA> <TA's> <# TB's> <TB's> <M0> <MN> <incM> <N0> <NN> <incN>
 *      <K0> <KN> <incK> <# alphas> <alphas> <# betas> <betas>
 *          
 */
{
   int M0, MN, incM, N0, NN, incN, K0, KN, incK, lda, ldb, ldc; 
   int i, k, m, n, ita, itb, ia, ib, nTA, nTB, nalph, nbeta, TEST;
   int itst=0, ipass=0, SAME, RANK_K;
   char TA[3], TB[3];
   TYPE *alph, *beta, *A, *B, *C, *D;
#ifdef TREAL
   if (nargs > 18)
#else
   if (nargs > 20)
#endif
   {
      k = 1;
      TEST = atoi(args[k++]);
      nTA = atoi(args[k++]);
      for (i=0; i != nTA; i++) TA[i] = *args[k++];
      nTB = atoi(args[k++]);
      for (i=0; i != nTB; i++) TB[i] = *args[k++];
      M0   = atoi(args[k++]);
      MN   = atoi(args[k++]);
      incM = atoi(args[k++]);
      N0   = atoi(args[k++]);
      NN   = atoi(args[k++]);
      incN = atoi(args[k++]);
      K0   = atoi(args[k++]);
      KN   = atoi(args[k++]);
      incK = atoi(args[k++]);
      nalph = atoi(args[k++]);
      alph = malloc(nalph * sizeof(TYPE) SHIFT);
      for (i=0; i != nalph SHIFT; i++) alph[i] = atof(args[k++]);
      nbeta = atoi(args[k++]);
      beta = malloc(nbeta * sizeof(TYPE) SHIFT);
      for (i=0; i != nbeta SHIFT; i++) beta[i] = atof(args[k++]);
   }
   else
   {
      fprintf(stderr, "Usage: %s <tst> <# TA> <TA's> <# TB's> <TB's> <M0> <MN> <incM> <N0> <NN> <incN> <K0> <KN> <incK> <# alphas> <alphas> <# betas> <betas>\n", args[0]);
      exit(-1);
   }
   if (SAME = N0 < 0)
   {
      NN = KN = MN;
      RANK_K = (K0 == NB);
   }
   if (TEST) i = 2;
   else i = 1;

   A = malloc( (i*MN*NN + MN*KN + NN*KN)*sizeof(*A) SHIFT );
   if (!A)
   {
      fprintf(stderr, "Not enough memory to run tests!!\n");
      exit(-1);
   }
   B = &A[MN*KN SHIFT];
   C = &B[NN*KN SHIFT];
   D = &C[(i-1)*MN*NN SHIFT];

#ifdef TREAL
   printf("\nTEST  TA  TB    M    N    K  alpha   beta    Time  Mflop  SpUp  PASS\n");
   printf("====  ==  ==  ===  ===  ===  =====  =====  ======  =====  ====  ====\n\n");
#else
   printf("\nTEST  TA  TB    M    N    K        alpha         beta    Time  Mflop  SpUp  PASS\n");
   printf("====  ==  ==  ===  ===  ===  ===== =====  ===== =====  ======  =====  ====  ====\n\n");
#endif
   for (m=M0; m <= MN; m += incM)
   {
      if (SAME)
      {
         K0 = KN = incK = N0 = NN = incN = m;
         if (RANK_K) K0 = KN = incK = NB;
      }
      for (n=N0; n <= NN; n += incN)
      {
         for (k=K0; k <= KN; k += incK)
         {
            for (ita=0; ita != nTA; ita++)
            {
               for (itb=0; itb != nTB; itb++)
               {
                  for (ia=0; ia != nalph; ia++)
                  {
                     for (ib=0; ib != nbeta; ib++)
                     {
                        itst++;
#ifdef LDA_IS_M
                           if (TA[ita] == 'n' || TA[ita] == 'N') lda = m SHIFT;
                           else lda = k SHIFT;
                           if (TB[itb] == 'n' || TB[itb] == 'N') ldb = k SHIFT;
                           else ldb = n SHIFT;
                           ldc = m;
#else
                           if (TA[ita] == 'n' || TA[ita] == 'N') lda = MN SHIFT;
                           else lda = KN SHIFT;
                           if (TB[itb] == 'n' || TB[itb] == 'N') ldb = KN SHIFT;
                           else ldb = NN SHIFT;
                           ldc = MN;
#endif
                        
#ifdef TREAL
                        ipass += mmcase(TEST, TA[ita], TB[itb], m, n, k,
                                        alph[ia], A, lda, B, ldb, beta[ib],
                                        C, ldc, D, ldc);
#else
                        ipass += mmcase(TEST, TA[ita], TB[itb], m, n, k,
                                        &alph[ia], A, lda, B, ldb, &beta[ib],
                                        C, ldc, D, ldc);
#endif
                     }
                  }
               }
            }
         }
      }
   }
   if (TEST) printf("\nNTEST=%d, NUMBER PASSED=%d, NUMBER FAILURES=%d\n",
                    itst, ipass, itst-ipass);
   else printf("\nDone with %d timing runs\n",itst);
   free(alph);
   free(beta);
   free(A);
   exit(0);
}
