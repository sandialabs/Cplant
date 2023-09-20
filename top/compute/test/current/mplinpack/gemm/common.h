
    /*        Fast GEMM routine for Alpha                  */
    /*           Linux, Digital UNIX and NT/Alpha          */
    /*                         date : 98.09.27             */
    /*        by Kazushige Goto <goto@statabo.rim.or.jp>   */


#ifndef COMMON_H
#define COMMON_H

#ifdef DGEMM

#define GEMM	dgemm
#define GEMM_	dgemm_
#define GEMMC	dgemmc
#define GEMMC_	dgemmc_
#define GEMM_NN	dgemm_nn
#define GEMM_NT	dgemm_nt
#define GEMM_TN	dgemm_tn
#define GEMM_TT	dgemm_tt

#define FLOAT	double

#else

#define GEMM	sgemm
#define GEMM_	sgemm_
#define GEMMC	sgemmc
#define GEMMC_	sgemmc_
#define GEMM_NN	sgemm_nn
#define GEMM_NT	sgemm_nt
#define GEMM_TN	sgemm_tn
#define GEMM_TT	sgemm_tt

#define FLOAT	float
#endif

#endif
