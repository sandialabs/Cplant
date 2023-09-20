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
#ifdef OSF
#include <nx.h>
#endif
#ifdef MPI
#  include <mpi.h>
#endif
#include "../include/defines.h"
#include "BLAS_prototypes.h"
/*  #include "head.h"   */
#include "solve.h"
#include "macros.h"
#include "pcomm.h"

#ifdef MPI_ERR_CHECK
#include "mpiDebug.h"
#endif

extern int me;

extern int nrows_matrix;       /* number of rows in the matrix */
extern int ncols_matrix;       /* number of cols in the matrix */

extern int nprocs_col;		/* num of procs to which a col is assigned */
extern int nprocs_row;		/* num of procs to which a row is assigned */

extern int my_first_col;        /* proc position in a col */
extern int my_first_row;	/* proc position in a row */

extern BLAS_INT my_rows;	/* num of rows I own */
extern BLAS_INT my_cols;        /* num of cols I own */

extern DATA_TYPE *col1;		/* ptrs to col used for updating a col */
extern BLAS_INT col1_stride;
extern BLAS_INT mat_stride;


extern DATA_TYPE *row1;		/* ptr to diagonal row */
extern DATA_TYPE *row2;		/* ptr to pivot row */

extern int nrhs;                /* number of right hand sides */
extern BLAS_INT my_rhs;              /* number of right hand sides that I own */

extern BLAS_INT colcnt;	        /* number of columns stored for BLAS 3 ops */
extern BLAS_INT blksz;	       	/* block size for BLAS 3 operations */
extern int rhs_blksz;

#ifdef MPI
extern MPI_Comm col_comm;
#endif

#ifdef MPI
  MPI_Request msgrequest;
  MPI_Status msgstatus;
#endif

#define SOSTATUSINT 32768

#define SOCOLTYPE (1<<26)
#define SOROWTYPE (1<<27)
#define SOHSTYPE (SOCOLTYPE + SOROWTYPE)
#define PERMTYPE ((1 << 25) + (1 << 26))


void
back_solve5(DATA_TYPE *fseg, DATA_TYPE *rhs)
{
  int i, j, l, k;               /* loop counters */

  int start_col;                /* col num to start row operations */
  BLAS_INT end_row;                  /* row num to end column operations */
  int num_rows;                 /* number of rows in a column operation */

  DATA_TYPE *ptr1,*ptr2;        /* running ptrs into mat and update cols */
  DATA_TYPE *ptr3, *ptr4;       /* running ptrs to diagonal and pivot rows */
  DATA_TYPE *ptr5;              /* running ptr into matrix */

  int bytes[16];                /* number of bytes in messages */
  int root;                     /* root processor for fanout */
  int type[16];                 /* mesage type for messages */
  int dest[16];                 /* dest for message sends */
  int hsbuf;                    /* buffer for hand shaking */

  BLAS_INT one = 1;                  /* constant for the daxpy routines */
  int zero = 0;                 /* constant for the daxpy routines */

  int num_c;	                /* col index for BLAS routines */
  int num_r;                    /* row index for BLAS routines */
  int num_k;	                /* inner index for BLAS routines */
  int j1,j2, tbl;
  int tblksz;
  int stride, stride1;          /* strides for BLAS routines */
  DATA_TYPE scaler, scaler1;    /* scalers for BLAS routines */
  char transA, transB;	        /* transpose flags for BLAS routine */
  double diag_mag;              /* magnitude of matrix diagonal */

#ifdef MPI
  MPI_Request creq[16];
  MPI_Status cstat[16];         /* flags for receives of matrix columns */
  int cflag[16];
#else
  int cflag[16];                /* flags for receives of matrix columns */
#endif
  int cpost[16];                /* post flags for columns */

  /* int cflag[16];                flags for receives of matrix columns */

  colcnt = 0;

  /* set the block size for backsolve */

  tblksz= blksz;
  if (tblksz > ((int) sqrt((double) nprocs_row))) {
     tblksz= ((int) sqrt((double) nprocs_row));
  }
  if (tblksz < 1) tblksz= 1;
  rhs_blksz = tblksz;

  /* set j2 to be first column in last group of columns */

  j2 = ( (int) ((ncols_matrix-1)/rhs_blksz) ) * rhs_blksz;
  
  /* send rhs to processor that owns col j2 of matrix */

  if (me == col_owner(0)) {
    dest[0] = col_owner(j2);
    if ( (me != dest[0]) && (j2 >= 0) ) {
      bytes[0] = my_rows*sizeof(DATA_TYPE);
      type[0] = SOROWTYPE + j2;
      send_msg(me, dest[0], (char *) rhs, bytes[0], type[0]);  
    }
  }


  for (j= ncols_matrix-1; j >= 0; j--) {

    start_col = j / nprocs_row;
    if (my_first_col < j%nprocs_row) ++start_col;
  
    end_row = j/nprocs_col;
    if (my_first_row <= j%nprocs_col) ++end_row;

    ptr5 = fseg + start_col*mat_stride;

    if (me == col_owner(j)) {

      if ( j % rhs_blksz == 0 ) {

        /* clear post arrays */
        for (i=rhs_blksz; i>=0; i--) cpost[i]=0;

        /* post for the rhs if I don't already have it */

        if (j == ( (int) ((ncols_matrix-1)/rhs_blksz) ) * rhs_blksz) {
          dest[0] = col_owner(0);
          bytes[0] = my_rows*sizeof(DATA_TYPE);
        } else {
          dest[0]= col_owner(j+rhs_blksz);
          end_row = (j+rhs_blksz-1)/nprocs_col;
          if (mesh_row(col_owner(j+rhs_blksz-1))<=(j+rhs_blksz-1)%nprocs_col) 
             ++end_row;
          bytes[0] = end_row*sizeof(DATA_TYPE);
        }
        if (me != dest[0]) {
          type[0] = SOROWTYPE + j;
          cflag[0] = 0;
/*          _nrecv((char *)rhs, &(bytes[0]), &(dest[0]), &(type[0]), 
                  &(cflag[0]), NULL);   */
           /*  crecv(type[0],(char *)rhs,bytes[0]);  */
          cpost[0] = 1;
        }

        /* post for additional columns */

        tbl = 1;
        for (j1=j+1; (j1<j+rhs_blksz) && (j1<ncols_matrix); j1++) {
          ptr2 = col1 + tbl*col1_stride; 
          dest[tbl] = col_owner(j1);
          bytes[tbl] = 0;
          type[tbl] = 2*(SOROWTYPE + j1);
          send_msg(me,dest[tbl],(char *)ptr2,bytes[tbl],type[tbl]);   

          end_row = (j1)/nprocs_col;
          if (mesh_row(col_owner(j1)) <= (j1)%nprocs_col) ++end_row;
          type[tbl] = SOROWTYPE + j1;
          bytes[tbl] = end_row*sizeof(DATA_TYPE);
          cflag[tbl] = 0;
/*          _nrecv((char *)ptr2, &(bytes[tbl]), &(dest[tbl]), &(type[tbl]), 
                  &(cflag[tbl]), NULL);   */
          /*  crecv(type[tbl],(char *)ptr2,bytes[tbl]);   */
          cpost[tbl] = 1;
          tbl++;
        }

        /* If we posted for columns or rhs, we wait until they arrive */

        for (i=rhs_blksz; i>=0; i--) if (cpost[i]) while (cflag[i] == 0);

        /* Do a DTRSV */

        for (j1 = j+tbl-1; j1 >= j; j1--) {
          if (j1 == j) {
            ptr2 = ptr5;
          } else {
            ptr2 = col1+(j1-j)*col1_stride; 
          }
          root = row_owner(j1);

          end_row = (j1)/nprocs_col;
          if (mesh_row(col_owner(j1)) <= (j1)%nprocs_col) ++end_row;

          if (me == root) {
            diag_mag = ABS_VAL(*(ptr2+end_row-1));
            DIVIDE(*(rhs+end_row-1),*(ptr2+end_row-1),diag_mag,*row1);
            *(rhs+end_row-1) = *row1;
            end_row--;
          }
          bytes[0] = sizeof(DATA_TYPE);
          type[0] = SOCOLTYPE+j;
          /* bcast_col(me, root, (char *) row1, bytes[0], type[0]);  */     
          NEGATIVE(*row1,scaler);
#ifdef COMPLEX
          XAXPY(end_row,scaler,ptr2,one,rhs,one); 
#else
          XAXPY(end_row,scaler,ptr2,one,rhs,one);  
/*
          for (i=0; i<end_row; i++)
            rhs[i] = rhs[i] + scaler * ptr2[i];
*/
#endif
        }
        
        /* forward the rhs */

        if ( j >= rhs_blksz ) {
          dest[0] = col_owner(j-rhs_blksz);
          if ( me != dest[0] ) {
            bytes[0] = end_row*sizeof(DATA_TYPE);
            type[0] = SOROWTYPE + j-rhs_blksz;
            send_msg(me, dest[0], (char *) rhs, bytes[0], type[0]);   
          }
        }
      } else {
        j2 = j - (j%rhs_blksz);
        dest[0] = col_owner(j2);
        if ( j2 >= 0 ) {
          bytes[0] = 0;
          type[0] = 2*(SOROWTYPE + j);
          recv_msg(me,dest[0],(char *)ptr5, bytes[0], type[0]);    

          type[0] = SOROWTYPE + j;
          bytes[0] = end_row*sizeof(DATA_TYPE);
          send_msg(me,dest[0],(char *)ptr5, bytes[0], type[0]);      
	}
      }
    }
    /*
    if (((j%SOSTATUSINT) == 0) && (me == 0))
      printf("Column %d completed.\n", j);
    */
  }
}

void
back_solve6(DATA_TYPE *fseg, DATA_TYPE *rhs)
{
  int i, j, l, k;               /* loop counters */

  int start_col;                /* col num to start row operations */
  BLAS_INT end_row;                  /* row num to end column operations */
  int num_rows;                 /* number of rows in a column operation */

  DATA_TYPE *ptr1,*ptr2;        /* running ptrs into mat and update cols */
  DATA_TYPE *ptr3, *ptr4;       /* running ptrs to diagonal and pivot rows */
  DATA_TYPE *ptr5;              /* running ptr into matrix */

  int bytes[16];                /* number of bytes in messages */
  int root;                     /* root processor for fanout */
  int type[16];                 /* mesage type for messages */
  int dest[16];                 /* dest for message sends */
  int hsbuf;                    /* buffer for hand shaking */

  BLAS_INT one = 1;                  /* constant for the daxpy routines */
  int zero = 0;                 /* constant for the daxpy routines */
  DATA_TYPE d_zero = CONST_ZERO;/* constant for initializations */
  DATA_TYPE d_one = CONST_ONE;  /* constant for the daxpy routines */
  DATA_TYPE d_min_one = CONST_MINUS_ONE; /* constant for the daxpy routines */

  int num_c;	                /* col index for BLAS routines */
  int num_r;                    /* row index for BLAS routines */
  int num_k;	                /* inner index for BLAS routines */
  int j1,j2, tbl;
  int tblksz;
  int stride, stride1;          /* strides for BLAS routines */
  DATA_TYPE scaler, scaler1;    /* scalers for BLAS routines */
  double diag_mag;              /* magnitude of matrix diagonal */

  BLAS_INT n_rhs_this;               /* num rhs that I currently own */
  int cpost[16];                /* post flags for columns */
  int cflag[16];                /* flags for receives of matrix columns */

  char transA = 'N';            /* all dgemm's don't transpose matrix A  */
  char transB = 'N';            /* nor transpose matrix B */

  int col_offset;               /* which processor starts the pipeline */
  int my_pos;                   /* my position in the new linup */
  int extra;                    /* extra loop to realign data after pipeline */
  int act_col;                  /* act this column (that I own) */
  int on_col;                   /* on this collection of rhs's */
  int global_col;               /* global col number for act_col */
  int max_bytes;                /* max number of bytes of rhs I can receive */

  int my_col_id, my_row_id, id_temp;
  int dest_right, dest_left;

  /* set the block size for backsolve */

  my_col_id = mesh_col(me);
  my_row_id = mesh_row(me);

  id_temp = my_col_id + 1;
  if (id_temp >= nprocs_row) id_temp = 0;
  dest_right = proc_num(my_row_id,id_temp);

  id_temp = my_col_id - 1;
  if (id_temp < 0) id_temp = nprocs_row-1;
  dest_left = proc_num(my_row_id,id_temp);


  /* set j2 to be first column in last group of columns */
  rhs_blksz=1;
  max_bytes = nrhs/nprocs_row;
  if (nrhs%nprocs_row > 0) max_bytes++;
  max_bytes = max_bytes*sizeof(DATA_TYPE)*my_rows;

  n_rhs_this = my_rhs;
  j2 = ncols_matrix-1;
  col_offset = (j2%nprocs_row);
  my_pos = my_first_col - col_offset;
  if (my_pos < 0) my_pos += nprocs_row;
  extra = (nprocs_row - (col_offset-1))%nprocs_row;
  
  act_col = my_cols-1;
  if (my_pos != 0) act_col++;

  on_col = my_pos;

  for (j = j2; j >= 1-nprocs_row-extra; j--) {
    if ((j+nprocs_row-1 >= 0) && (n_rhs_this > 0)) {

      if ((act_col < my_cols) && (act_col >= 0)) {

        global_col = act_col*nprocs_row + my_first_col;

        end_row = global_col/nprocs_col;
        if (my_first_row <= global_col%nprocs_col) ++end_row;

        ptr5 = fseg + act_col*my_rows;

        /* do an elimination step on the rhs that I own */

        ptr2 = ptr5;
        root = row_owner(global_col);

        if (me == root) {
          diag_mag = ABS_VAL(*(ptr2+end_row-1));
          for (j1=0; j1<n_rhs_this; j1++) {
            ptr3 = rhs + j1*my_rows + end_row - 1;
            ptr4 = row1 + j1;
            DIVIDE(*ptr3,*(ptr2+end_row-1),diag_mag,*ptr4);
            *ptr3 = *ptr4;
          }
          end_row--;
        }

        bytes[0] = n_rhs_this*sizeof(DATA_TYPE);
        type[0] = SOCOLTYPE+j;
#ifdef MPI
#ifdef MPI_ERR_CHECK
      mrc=MPI_Bcast((char *) row1, bytes[0], MPI_CHAR, mesh_row(root), col_comm);
          TEST_MRC(201)
#else
      MPI_Bcast((char *) row1, bytes[0], MPI_CHAR, mesh_row(root), col_comm);
#endif
#else
      bcast_col(me, root, (char *) row1, bytes[0], type[0]);
#endif
/*     Changed XGEMM_ to XGEMM   removed all &   */

        XGEMM_(&transA, &transB, &end_row, &n_rhs_this, &one, &d_min_one, 
               ptr5, &my_rows,
               row1, &one, &d_one, 
               rhs, &my_rows);  
      }
    }

    if (j != 1-nprocs_row-extra) {
      dest[0] = dest_right;
      if (me != dest[0]) {

/*        dest[1] = dest_right;
        bytes[1] = 0;
        type[1] = SOHSTYPE+j;

#ifdef MPI
#ifdef MPI_ERR_CHECK
          mrc=MPI_Send((char *) rhs, bytes[1], MPI_CHAR, dest[1], type[1],MPI_COMM_WORLD);
          TEST_MRC(202)
#else
          MPI_Send((char *) rhs, bytes[1], MPI_CHAR, dest[1], type[1],MPI_COMM_WORLD);
#endif
#else
          send_msg(me,dest[1],(char *) rhs,bytes[1],type[1]);
#endif

        dest[1] = dest_left;
        bytes[1] = 0;
        type[1] = SOHSTYPE+j;

#ifdef MPI
#ifdef MPI_ERR_CHECK
          mrc=MPI_Recv((char *) rhs, bytes[1], MPI_CHAR, dest[1], type[1],MPI_COMM_WORLD,&msgstatus);
          TEST_MRC(203)
#else
          MPI_Recv((char *) rhs, bytes[1], MPI_CHAR, dest[1], type[1],MPI_COMM_WORLD,&msgstatus);
#endif
#else
          recv_msg(me,dest[1],(char *) rhs, bytes[1],type[1]);
#endif

  */

        dest[1] = dest_left;
        bytes[1] = n_rhs_this * sizeof(DATA_TYPE) * my_rows;
        type[1] = SOROWTYPE+j;
#ifdef MPI
#ifdef MPI_ERR_CHECK
          mrc=MPI_Send((char *) rhs, bytes[1], MPI_CHAR, dest[1], type[1],MPI_COMM_WORLD); 
          TEST_MRC(204)
#else
          MPI_Send((char *) rhs, bytes[1], MPI_CHAR, dest[1], type[1],MPI_COMM_WORLD); 
#endif
#else 
          send_msg(me,dest[1],(char *) rhs,bytes[1],type[1]); 
#endif

        bytes[0] = max_bytes;
        type[0] = SOROWTYPE+j;

#ifdef MPI
  /*      MPI_Recv(&rhs,1,MPI_INT,MPI_ANY_SOURCE,type[0],MPI_COMM_WORLD,&msgstatus);   */
#ifdef MPI_ERR_CHECK
           mrc=MPI_Recv((char *)rhs,bytes[0],MPI_CHAR,MPI_ANY_SOURCE,type[0],MPI_COMM_WORLD,&msgstatus);
          TEST_MRC(205)
#else
 /* Changed MPI_ANY_SOURCE to dest[0]   */
           MPI_Recv((char *)rhs,bytes[0],MPI_CHAR,dest[0],type[0],MPI_COMM_WORLD,&msgstatus);
#endif
#else
 /*       _nrecv((char *)rhs, &(bytes[0]), &(dest[0]), &(type[0]), 
                NULL, NULL);    */
          crecv(type[0],(char *)rhs,bytes[0]);
#endif
        n_rhs_this = bytes[0]/sizeof(DATA_TYPE)/my_rows;
      }
      on_col++;
      if (on_col >= nprocs_row) {
        on_col = 0;
        act_col--;
      }
    }
  }
}

void collect_vector(DATA_TYPE *vec)
{
  int j, k;

  int start_col;
  BLAS_INT end_row;

  int root;
  int bytes;
  int dest;
  int type;

  for (j=0; j<ncols_matrix; j++) {
    if (me == col_owner(j)) {
      start_col = (j) / nprocs_row;
      if (my_first_col < (j)%nprocs_row) ++start_col;

      if (j%rhs_blksz == 0) {
        for (k=0; k<rhs_blksz; k++) {
          end_row = (j+k)/nprocs_col;
          if (my_first_row <= (j+k)%nprocs_col) ++end_row;
          if (j+k < ncols_matrix) {
            if (me == row_owner(j+k)) {
              dest = col_owner(0);
              if (me != dest) {
                bytes = sizeof(DATA_TYPE);
                type = PERMTYPE + j + k;
/*                send_msg(me, dest, (char *) (vec+end_row-1), bytes, type);   */
#ifdef MPI
#ifdef MPI_ERR_CHECK
               mrc=MPI_Send((vec+end_row-1),bytes,MPI_BYTE,
                   dest,type,MPI_COMM_WORLD);
          TEST_MRC(206)
#else
               MPI_Send((vec+end_row-1),bytes,MPI_BYTE,
                   dest,type,MPI_COMM_WORLD);
#endif
#else
                send_msg(me,dest, (char *) (vec+end_row-1), bytes, type);
#endif
              }
            }
          }
        }
      }
    }
    if (me == col_owner(0)) {
      if (me == row_owner(j)) {
        end_row = (j)/nprocs_col;
        if (my_first_row <= (j)%nprocs_col) ++end_row;
        dest = col_owner((j/rhs_blksz)*rhs_blksz);
        if (me != dest) {
          bytes = sizeof(DATA_TYPE);
          type = PERMTYPE + j;
#ifdef MPI
#ifdef MPI_ERR_CHECK
          mrc=MPI_Recv((vec+end_row-1),bytes,MPI_BYTE,dest,
                   type,MPI_COMM_WORLD,&msgstatus);
          TEST_MRC(207)
#else
          MPI_Recv((vec+end_row-1),bytes,MPI_BYTE,dest,
                   type,MPI_COMM_WORLD,&msgstatus);
#endif
#else
          recv_msg(me,dest , (char *)(vec+end_row-1), bytes, type);
#endif
        }
      }
    }
  }      
}
