/*
Copyright (c) 1993 Sandia National Laboratories
 
  Redistribution and use in source and binary forms are permitted
  provided that source distributions retain this entire copyright
  notice.  Neither the name of Sandia National Laboratories nor
  the name of the author may be used to endorse or promote products
  derived from this software without specific prior written permission.
 
Warranty:
  This software is provided "as is" and without any express or
  implied warranties, including, without limitation, the implied
  warranties of merchantability and fitness for a particular
  purpose.

The README file contains addition information
*/

#include <math.h>
#include <stdio.h>
#ifdef MPI
#include <mpi.h>
#endif
#include "defines.h"
#include "init.h"
#include "pcomm.h"
/* #include "BLAS_prototypes.h"   */
#include "macros.h"

#define INITTYPE1 ((1 << 17) + 1)
#define INITTYPE2 ((1 << 17) + 2)
#define INITTYPE3 ((1 << 17) + 3)
#define INITTYPE4 ((1 << 17) + 4)
#define INITTYPE5 ((1 << 17) + 5)
#define INITTYPE6 ((1 << 17) + 6)

#define MATVECTYPE ((1 << 25) + (1 << 26))

#undef SHOWINPUT


extern BLAS_INT my_cols;            /* num of cols I own */
extern int my_cols_seg;        /* num of cols I own in a seg */
extern BLAS_INT my_cols_last;       /* num of cols I own in the rightmost seg */

extern int nsegs_row;          /* num of segs to which each row is assigned */
extern int ncols_seg;          /* num of cols in each segment */
extern int ncols_last;         /* num of cols in last segment */
extern BLAS_INT my_rows;            /* num of rows I own */
extern int nprocs_row;         /* num of procs to which a row is assigned */
extern int my_first_col;       /* proc position in a col */
extern int nprocs_col;         /* num of procs to which a col is assigned */
extern int my_first_row;       /* proc position in a row */
extern int nrows_matrix;       /* number of rows in the matrix */
extern int nrhs;               /* number of right hand sides */
extern BLAS_INT my_rhs;             /* number of right hand sides I own */
extern int me;                 /* who I am */
extern int rhs_blksz;          /* block size for backsolve */
extern DATA_TYPE *col1, *col2;
extern DATA_TYPE *row1, *row2;

extern int inputtype;

extern BLAS_INT one;
#ifdef MPI
extern MPI_Comm row_comm,col_comm;
#endif

void init_seg(DATA_TYPE *seg, int seg_num)
{

  int k, l;          /* loop counters */
  int my_cols_this;  /* num of cols I have in a seg */
  DATA_TYPE *tp;        /* temp ptr for stepping through the matrix entries */
  double drand48();  /* random number generator */
  long seedval;
  int indi,indj;

  
  if (me == 0) {
    switch (inputtype) {
    case 0:
      printf("Using random matrix \n");
      break;
    case 1:
      printf("USING TOEPLITZ MATRIX\n");
      break;
    case 2:
      printf("USING ONEM MATRIX\n");
      break;
    case 3:
      printf("USING ALL ONES MATRIX\n");
    }
  }

  
#ifdef IO
  my_cols_this = (seg_num == nsegs_row-1) ? my_cols_last : my_cols_seg;
#else
  my_cols_this = my_cols;
#endif
    
  seedval = nrows_matrix + me;
  srand48(seedval);

#ifdef SHOWINPUT
  printf("Node %d: My portion of the input \n",me);
#endif

  for ( l = 0; l < my_cols_this; l++) {
    tp = seg + l*(my_rows);
    for (k = 0 ; k < my_rows; k++) {
#ifdef COMPLEX
      switch (inputtype) {
      case 0:
	(*tp).r = 1.0 - 2.0*drand48();
	(*tp).i = 1.0 - 2.0*drand48();
	tp++;
	break;
      case 1:
	(*tp).r = ( seg_num*ncols_seg + l*nprocs_row + my_first_col + 
		   k*nprocs_col + my_first_row ) % nrows_matrix;
	(*tp).i = 0.0;
	tp++;
	break;
      case 2:
	indj = l*nprocs_row + my_first_col + 1;
	indi = k*nprocs_col + my_first_row + 1;
	(*tp).r = (indj < indi) ? indj : indi;
	(*tp++).i = 0.0;
	break;
      case 3:
	(*tp).r = 1.0;
	(*tp++).i = 0.0;
      }
      
#else
      switch (inputtype) {
      case 0:
	*tp++ = 1.0 - 2.0*drand48();
	break;
      case 1:
	*tp++ = ( seg_num*ncols_seg + l*nprocs_row + my_first_col + 
		 k*nprocs_col + my_first_row ) % nrows_matrix;
	break;
      case 2:
	indj = l*nprocs_row + my_first_col + 1;
	indi = k*nprocs_col + my_first_row + 1;
	*tp++ = (indj < indi) ? indj : indi;
	break;
      case 3:
	*tp++ = 1.0;
      }
#ifdef SHOWINPUT
      printf("Node %d: row %d, col %d = %g \n",me,k,l,*(tp -1));
#endif
     
#endif
    }
  }
}

void init_rhs(DATA_TYPE *rhs, DATA_TYPE *seg, int seg_num)
{

  int k, l;          /* loop counters */
  int stride;        /* stride for dasum routine */
  DATA_TYPE *tp,*tp1;   /* temp ptr for stepping through the matrix entries */
  int my_cols_this;  /* num of cols I have in a seg */

#ifdef IO
  my_cols_this = (seg_num == nsegs_row-1) ? my_cols_last : my_cols_seg;
#else
  my_cols_this = my_cols;
#endif

#ifdef COMPLEX
  stride = my_rows;
  for (k= 0; k < my_rows; k++) {
    tp = rhs + k;
    tp1 = seg + k;
    (*tp).r = 0.0;
    (*tp).i = 0.0;
    for (l=0; l < my_cols_this; l++) {
      (*tp).r += (*tp1).r;
      (*tp).i += (*tp1).i;
      tp1 += stride;
    }
    tp++;
  }
 #ifdef MPI
  MPI_Allreduce((double *)rhs,(double *)col1,2*my_rows,MPI_DOUBLE,MPI_SUM,row_comm);
 #else
  sum_row( me, (double *)rhs, (double *)col1, 2*my_rows, INITTYPE1 );
 #endif
#ifdef ONES
  for (k = 0; k < my_rows; k++) {
    *(rhs+k) = nrows_matrix - (k*nprocs_col + my_first_row);
  }
#endif

#else
  stride = my_rows;
  for (k= 0; k < my_rows; k++) {
    tp = rhs + k;
    tp1 = seg + k;
    *tp = 0.0;
    for (l=0; l < my_cols_this; l++) {
      *tp += *tp1;
      tp1 += stride;
    }
    tp++;
  }
 #ifdef MPI
  MPI_Allreduce(rhs,col1,my_rows,MPI_DOUBLE,MPI_SUM,row_comm);
 #else
  sum_row( me, rhs, col1, my_rows, INITTYPE1 );
 #endif
#ifdef ONES
  for (k = 0; k < my_rows; k++) {
    *(rhs+k) = nrows_matrix - (k*nprocs_col + my_first_row);
  }
#endif
#endif
}
