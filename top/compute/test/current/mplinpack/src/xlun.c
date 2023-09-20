#include <stdio.h>
#include <math.h>
#include "defines.h"
#include "BLAS_prototypes.h"
#include "time.h"
#include "xlu.h"
#include "multiproc.h"
#ifdef MPI
#include "mpi.h"
#include "time.h"
#endif
#include "vars.h"
#include "macros.h"
#include "malloc.h"

#define PERMTYPE ((1 << 5) + (1 << 4))


void XLU_(DATA_TYPE *matrix, int *matrix_size, int *num_procsr, 
    DATA_TYPE *rhsides, int *num_rhs, double *secs)
{
  int i, j, k, l;
  DATA_TYPE *mat;
  DATA_TYPE test;
  int MSPLIT;
  double run_secs;              /* time (in secs) during which the prog ran */
  double seconds();             /* function to generate timings */
  double tsecs;                 /* intermediate storage of timing info */

  char *waste0, *waste1;        /* to get memory aligned */
/*
   Determine who I am (me ) and the total number of nodes (nprocs_cube)
                                                                        */
#ifdef MPI

  me = mytask;
  nprocs_cube = numtasks;


#else
  me = mynode();
  nprocs_cube = numnodes();
#endif

  mat = matrix;
  rhs = rhsides;
  printf(" Entering xlu %d \n",me);
  nrows_matrix = *matrix_size;
  ncols_matrix = *matrix_size;
  nprocs_row = *num_procsr;

  nprocs_col = nprocs_cube/nprocs_row;
  max_procs = (nprocs_row < nprocs_col) ? nprocs_col : nprocs_row;

#ifdef MPI
    /* set up communicators for rows and columns */
    myrow = mesh_row(me);
    mycol = mesh_col(me);
    MPI_Comm_split(MPI_COMM_WORLD,myrow,mycol,&row_comm);
    MPI_Comm_split(MPI_COMM_WORLD,mycol,myrow,&col_comm);

    {int checkcol,checkrow;
     MPI_Comm_rank(col_comm, &checkrow) ;
     MPI_Comm_rank(row_comm, &checkcol) ;
     if (myrow != checkrow)
       printf("Node %d: my row = %d but rank in col = %d\n",me,myrow,checkrow);     if (mycol != checkcol)
       printf("Node %d: my col = %d but rank in row = %d\n",me,mycol,checkcol);   }

#endif

  my_first_col = mesh_col(me);
  my_first_row = mesh_row(me);

  my_rows = nrows_matrix / nprocs_col;
  if (my_first_row < nrows_matrix % nprocs_col)
    ++my_rows;
  my_cols = ncols_matrix / nprocs_row;
  if (my_first_col < ncols_matrix % nprocs_row)
    ++my_cols;

#ifdef TURBO
#ifdef COMPLEX
  blksz = 6;
  MSPLIT = 100;
#else
  blksz = 16;
  MSPLIT = 20;
#endif
#else
#ifdef COMPLEX
  blksz = 2;
  MSPLIT = 100;
#else
  blksz = 4;
  MSPLIT = 20;
#endif
#endif
  printf(" Node  %d: responding \n",me);
/*  waste0 = malloc(sizeof(DATA_TYPE));  */
/*  if ((waste0 && 0x7f) != 0x0) {
    waste1 = malloc(0x80-(waste0 && 0x7f));
  }   */

  nrhs = *num_rhs;
  my_rhs = nrhs / nprocs_row;
  if (my_first_col < nrhs % nprocs_row) ++my_rhs;

  pivot_vec = (int *) MALLOC_8(my_cols * sizeof(int));
  totmem += my_cols * sizeof(int);
  if (pivot_vec == NULL) {
    fprintf(stderr, "Node %d: Out of memory\n", me);
    exit(-1);
  }

  row3 = (DATA_TYPE *) MALLOC_8((my_cols + blksz + nrhs) * sizeof(DATA_TYPE));
  totmem += (my_cols + blksz + 1) * sizeof(DATA_TYPE);
  if (row3 == NULL) {
    fprintf(stderr, "Node %d: Out of memory\n", me);
    exit(-1);
  }

  row2 = (DATA_TYPE *) MALLOC_8((my_cols + blksz + nrhs) * sizeof(DATA_TYPE));
  totmem += (my_cols + blksz + 1) * sizeof(DATA_TYPE);
  if (row2 == NULL) {
    fprintf(stderr, "Node %d: Out of memory\n", me);
    exit(-1);
  }

  row1_stride = my_cols+blksz+1;
  row1 = (DATA_TYPE *) MALLOC_8(blksz*(my_cols+blksz+nrhs)*sizeof(DATA_TYPE));
  totmem += blksz * (my_cols + blksz + 1) * sizeof(DATA_TYPE);
  if (row1 == NULL) {
    fprintf(stderr, "Node %d: Out of memory\n", me);
    exit(-1);
  }

  col2 = (DATA_TYPE *) MALLOC_8((my_rows + 1) * sizeof(DATA_TYPE));
  totmem += (my_rows + 1) * sizeof(DATA_TYPE);
  if (col2 == NULL) {
    fprintf(stderr, "Node %d: Out of memory\n", me);
    exit(-1);
  }

  col1_stride = my_rows;
  col1 = (DATA_TYPE *) MALLOC_8(blksz * (my_rows + 1) * sizeof(DATA_TYPE));
  totmem += blksz * (my_rows + 1) * sizeof(DATA_TYPE);
  if (col1 == NULL) {
    fprintf(stderr, "Node %d: Out of memory\n", me);
    exit(-1);
  }

  mat_stride = my_rows;

  /* Factor and Solve the system */
  tsecs = seconds(0.0);
  initcomm();
  test = *mat;
  printf("Node %d:  first entry  val %g %g  \n",me,(test.r),(test.i));
  factor(mat);
  if (nrhs > 0) {
    back_solve6(mat, rhs);
  }
  tsecs = seconds(tsecs);
  run_secs = (double) tsecs;
  *secs = run_secs;

  free(col1);
  free(col2);
  free(row1);
  free(row2);
  free(row3);
  free(pivot_vec);
}
/*  11111111111111111111     Commented out perm for use with mpi  
void perm_(DATA_TYPE *vec, int *size1, int *size2, int *size3, 
          int *size4, int *size5, int *size6) 
{
  int i;
  int j;
  int k;

  int start_col;
  int end_row;
  int num_rows;    
  int stride;
  BLAS_INT one=1;

  int root;
  int bytes;
  int dest;
  int type;

  int my_rows1;
  int my_rows2;
  int my_rows3;
  int my_cols1;
  int my_cols2;
  int my_cols3;

  int index_off1;
  int index_off2;
  int index_off3;
  int index_off4;

  int global_index;
  int local_index;
  int p1_index;
  int p2_index;

  int change_count;

  DATA_TYPE buf[2];
  DATA_TYPE *ptr1;
  DATA_TYPE *ptr2;

  my_rows1 = *size1;
  my_rows2 = *size2;
  my_rows3 = *size3;
  my_rows = my_rows1 + my_rows2 + my_rows3;
  my_cols1 = *size4;
  my_cols2 = *size5;
  my_cols3 = *size6;
  my_cols = my_cols1 + my_cols2 + my_cols3;

  index_off1 = my_cols1*nprocs_row;
  index_off2 = (my_cols1+my_cols2)*nprocs_row;
  index_off3 = my_rows1*nprocs_col;
  index_off4 = (my_rows1+my_rows2)*nprocs_col;

  if (me == col_owner(0)) {

    ptr2 = vec;
    change_count = 0;
    for (i=0; i<my_rows; i++) {
      global_index = my_first_row + i*nprocs_col;
      if (global_index < index_off1) {
        p1_index = global_index/nprocs_row + 
                   (global_index%nprocs_row)*my_cols1;
      } else if (global_index < index_off2) {
        p1_index = index_off1 + (global_index-index_off1)/nprocs_row 
                     + ((global_index-index_off1)%nprocs_row)*my_cols2;
      } else {
        p1_index = index_off2 + (global_index-index_off2)/nprocs_row 
                     + ((global_index-index_off2)%nprocs_row)*my_cols3;
      }
      if (p1_index < index_off3) {
        p2_index = p1_index/my_rows1
                      + (p1_index%my_rows1)*nprocs_col;
      } else if (p1_index < index_off4) {
        p2_index = index_off3 + (p1_index-index_off3)/my_rows2 
                     + ((p1_index-index_off3)%my_rows2)*nprocs_col;
      } else {
        p2_index = index_off4 + (p1_index-index_off4)/my_rows3
                     + ((p1_index-index_off4)%my_rows3)*nprocs_col;
      }

      dest = row_owner(p2_index);
      local_index = p2_index/nprocs_col;
      if (dest == me) {
        *(col1+local_index) = *ptr2;
      } else {
        buf[0] = *ptr2;
#ifdef COMPLEX
        (buf[1]).r = local_index + 0.1;
#else
        buf[1] = local_index + 0.1;
#endif
        bytes = 2*sizeof(DATA_TYPE);
        type = PERMTYPE;
        send_msg(me, dest, (char *) buf, bytes, type);
        change_count++;
      }
      ptr2++;
    }
    for (i=0; i< change_count; i++) {
      bytes = 2*sizeof(DATA_TYPE);
      type = PERMTYPE;
      dest = -1;
      recv_msg(me, dest, (char *) buf, bytes, type);
#ifdef COMPLEX
      local_index = (buf[1]).r;
#else
      local_index = buf[1];
#endif
      *(col1 + local_index) = buf[0];
    }
    XCOPY(my_rows, col1, one, vec, one);
  }
}  
                                        1111111111111111111111111          */
void indmap_(int *gindex, int *size1, int *size2, int *size3, 
          int *size4, int *size5, int *size6, int *proc, int *local_index) 
{
  int my_rows1;
  int my_rows2;
  int my_rows3;
  int my_cols1;
  int my_cols2;
  int my_cols3;

  int index_off1;
  int index_off2;
  int index_off3;
  int index_off4;

  int global_index;
  int p1_index;
  int p2_index;

  global_index = *gindex;
  my_rows1 = *size1;
  my_rows2 = *size2;
  my_rows3 = *size3;
  my_rows = my_rows1 + my_rows2 + my_rows3;
  my_cols1 = *size4;
  my_cols2 = *size5;
  my_cols3 = *size6;
  my_cols = my_cols1 + my_cols2 + my_cols3;

  index_off1 = my_cols1*nprocs_row;
  index_off2 = (my_cols1+my_cols2)*nprocs_row;
  index_off3 = my_rows1*nprocs_col;
  index_off4 = (my_rows1+my_rows2)*nprocs_col;

  if (global_index < index_off3) {
    p1_index = global_index/nprocs_col
               + (global_index%nprocs_col)*my_rows1;
  } else if (global_index < index_off4) {
    p1_index = index_off1 + (global_index-index_off1)/nprocs_col 
               + ((global_index-index_off1)%nprocs_col)*my_rows2;
  } else {
    p1_index = index_off2 + (global_index-index_off2)/nprocs_col 
               + ((global_index-index_off2)%nprocs_col)*my_rows3;
  }
  if (p1_index < index_off1) {
    p2_index = p1_index/my_cols1
               + (p1_index%my_cols1)*nprocs_row;
    *proc = p2_index/my_rows2;
    *local_index = p2_index%my_rows2;
    *proc *= nprocs_row;
  } else if (p1_index < index_off2) {
    p2_index = index_off1 + (p1_index-index_off1)/my_cols2 
               + ((p1_index-index_off1)%my_cols2)*nprocs_row;
    *proc = (p2_index-index_off1)/my_rows2;
    *local_index = my_rows1 + (p2_index-index_off2)%my_rows2;
    *proc *= nprocs_row;
  } else {
    p2_index = index_off2 + (p1_index-index_off2)/my_cols3
               + ((p1_index-index_off2)%my_cols3)*nprocs_row;
    *proc = (p2_index-index_off2)/my_rows3;
    *local_index = my_rows1 + my_rows2 + (p2_index-index_off2)%my_rows3;
    *proc *= nprocs_row;
  }
}

void perm1_(DATA_TYPE *vec, int *num_my_rhs)
{
  int i;
  int j;
  int k;

  int start_col;
  int end_row;
  int num_rows;
  int stride;
  BLAS_INT one=1;
  BLAS_INT my_rhs;

  int root;
  int bytes;
  int dest;
  int type;

  int global_index;
  int local_index;
  int my_index;

  int col_offset, row_offset;
  int ncols_proc1, ncols_proc2, nprocs_row1;
  int nrows_proc1, nrows_proc2, nprocs_col1;

  int change_count; 
  int change_nosend;

#ifdef MPI
  MPI_Request msgrequest;
  MPI_Status msgstatus;
#endif

  DATA_TYPE buf[2];
  DATA_TYPE *ptr1;
  DATA_TYPE *ptr2;


  my_rhs=*num_my_rhs;
  /*
  printf("me=%d, my_col1=%d, my_row1=%d, my_rhs=%d, my_rows=%d\n",
          me, my_first_col, my_first_row, my_rhs, my_rows);
   */
  if (my_rhs > 0) {
    ncols_proc1 = ncols_matrix/nprocs_row;
    ncols_proc2 = ncols_proc1;
    if (ncols_matrix%nprocs_row > 0) ncols_proc1++;
    nprocs_row1 = (ncols_matrix%nprocs_row);
    row_offset = ncols_proc1 * nprocs_row1;

    nrows_proc1 = nrows_matrix/nprocs_col;
    nrows_proc2 = nrows_proc1;
    if (nrows_matrix%nprocs_col > 0) nrows_proc1++;
    nprocs_col1 = (nrows_matrix%nprocs_col);
    col_offset = nrows_proc1 * nprocs_col1;

    ptr2 = vec;
    change_count = 0;
    change_nosend = 0;

    for (i=0; i<my_rows; i++) {
      global_index = my_first_row + i*nprocs_col;

      /* break down global index using torus wrap in row direction */
      dest = global_index%nprocs_row;
      local_index = global_index/nprocs_row;

      /* rebuild global index using block in row direction */
      if (dest < nprocs_row1) {
        global_index = dest*ncols_proc1 + local_index;
      } else {
        global_index = row_offset + (dest-nprocs_row1)*ncols_proc2 
                       + local_index;
      }

      /* break down global index using blocks in the column direction */
      if (global_index < col_offset) {
        dest = global_index/nrows_proc1;
        local_index = global_index%nrows_proc1;
      } else {
        dest = (global_index - col_offset)/nrows_proc2 + nprocs_col1;
        local_index = (global_index - col_offset)%nrows_proc2;
      }

      dest = dest*nprocs_row + my_first_col;
      if ((local_index != i) && (dest == me)) {
        XCOPY(my_rhs, ptr2, my_rows, row1, one);
#ifdef COMPLEX
        (*(row1+my_rhs)).r = local_index + 0.1;
#else
        *(row1+my_rhs) = local_index + 0.1;
#endif
        bytes = (my_rhs + 1)*sizeof(DATA_TYPE);
        type = PERMTYPE;        
#ifdef MPI
        printf(" send type %d, dest %d %d, me %d \n",type,dest,i,me);
        MPI_Send((char *) row1,bytes,MPI_BYTE,dest,
                 type,MPI_COMM_WORLD);
#else
        _nsend((char *) row1, bytes, dest, type, NULL, 0);
#endif
        change_count++;  
      }
      ptr2++;  
    }
    for (i=0; i< change_count; i++) {     
      bytes = (my_rhs + 1)*sizeof(DATA_TYPE);
      type = PERMTYPE;
      dest = -1;
#ifdef MPI
      printf(" recv type %d, dest %d  %d, me %d\n",type,dest,i,me);
      MPI_Recv((char *) row1,bytes,MPI_BYTE,MPI_ANY_SOURCE,
                MPI_ANY_TAG,MPI_COMM_WORLD,&msgstatus);
#else
      /* _nrecv(NULL, &bytes, &dest, &type, NULL, &gpivot_row);  */
      _nrecv((char *) row1, &bytes, &dest, &type, NULL, NULL);
#endif
#ifdef COMPLEX
      local_index = (*(row1+my_rhs)).r;
#else
      local_index = *(row1+my_rhs);
#endif
      XCOPY(my_rhs,row1,one,(vec+local_index),my_rows);
    }                                            
  }
/*       Redo for those Solutions going to different nodes     */
   my_rhs=*num_my_rhs;
    printf(" myrhs %d,  me = %d\n",my_rhs,me);
  if (my_rhs > 0) {
    ncols_proc1 = ncols_matrix/nprocs_row;
    ncols_proc2 = ncols_proc1;
    if (ncols_matrix%nprocs_row > 0) ncols_proc1++;
    nprocs_row1 = (ncols_matrix%nprocs_row);
    row_offset = ncols_proc1 * nprocs_row1;

    nrows_proc1 = nrows_matrix/nprocs_col;
    nrows_proc2 = nrows_proc1;
    if (nrows_matrix%nprocs_col > 0) nrows_proc1++;
    nprocs_col1 = (nrows_matrix%nprocs_col);
    col_offset = nrows_proc1 * nprocs_col1;

    ptr2 = vec;
    change_count = 0;
    change_nosend = 0;

   for (i=0; i<my_rows; i++) {
      global_index = my_first_row + i*nprocs_col;

      /* break down global index using torus wrap in row direction */
      dest = global_index%nprocs_row;
      local_index = global_index/nprocs_row;

      /* rebuild global index using block in row direction */
      if (dest < nprocs_row1) {
        global_index = dest*ncols_proc1 + local_index;
      } else {
        global_index = row_offset + (dest-nprocs_row1)*ncols_proc2 
                       + local_index;
      }

      /* break down global index using blocks in the column direction */
      if (global_index < col_offset) {
        dest = global_index/nrows_proc1;
        local_index = global_index%nrows_proc1;
      } else {
        dest = (global_index - col_offset)/nrows_proc2 + nprocs_col1;
        local_index = (global_index - col_offset)%nrows_proc2;
      }

      dest = dest*nprocs_row + my_first_col;

      if ((local_index != i) && (dest != me)) {
        XCOPY(my_rhs, ptr2, my_rows, row1, one);
#ifdef COMPLEX
        (*(row1+my_rhs)).r = local_index + 0.1;
#else
        *(row1+my_rhs) = local_index + 0.1;
#endif
        bytes = (my_rhs + 1)*sizeof(DATA_TYPE);
        type = PERMTYPE;        
#ifdef MPI
        printf(" send type %d, dest %d %d, me %d \n",type,dest,i,me);
        MPI_Send((char *) row1,bytes,MPI_BYTE,dest,
                 type,MPI_COMM_WORLD);
#else
        _nsend((char *) row1, bytes, dest, type, NULL, 0);
#endif
        change_count++; 
      }
      ptr2++;  
   }
     type  = PERMTYPE;
     printf(" change count  %d  , me %d\n",change_count,me);
    for (i=0; i< change_count; i++) {
      bytes = (my_rhs + 1)*sizeof(DATA_TYPE);
      type = PERMTYPE;
      dest = -1;
#ifdef MPI
      printf(" recv type %d, dest %d  %d, me %d\n",type,dest,i,me);
      MPI_Recv((char *) row1,bytes,MPI_BYTE,MPI_ANY_SOURCE,
                MPI_ANY_TAG,MPI_COMM_WORLD,&msgstatus);
#else
      /* _nrecv(NULL, &bytes, &dest, &type, NULL, &gpivot_row);  */
      _nrecv((char *) row1, &bytes, &dest, &type, NULL, NULL);
#endif
#ifdef COMPLEX
      local_index = (*(row1+my_rhs)).r;
#else
      local_index = *(row1+my_rhs);
#endif
      XCOPY(my_rhs,row1,one,(vec+local_index),my_rows);
    }
  }
}  
