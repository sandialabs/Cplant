int ii,iii;
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
 
Title:
  complex double precision LU factorization for the Intel Paragon
 
General Description:
  This code contains all the routines to do LU factorization with
  pivoting double precision real matrices.  The matrix L is not
  available explicitly, but rather as a product of Gauss matrices and
  permutation matrices.  Routines for triangular solves are included.
  Results are reported for the linpack benchmark

Files Required:
  BLAS_prototypes.h
  README
  factor.c           factorization of matrix
  factor.h
  init.c             initialization of matrix and right hand side
  init.h
  macros.h           macros for processor assignment and decomposition
  malloc.c           aligned mallocs
  malloc.h
  pcomm.c            paragon communication routines
  pcomm.h
  mat.c              main program
  solve.c            triangular solve
  solve.h
  time.c             timing routines
  time.h

Modification History:
  09/30/93 - initial port to Intel Paragon completed
  10/28/93 - linpack residual checks incorporated
  10/29/93 - out-of-core facilities removed from code

Authors: 
  1. David E. Womble (primary author)
     Sandia National Laboratories, Dept. 1422
     P.O. Box 5800
     Albuquerque, NM 87185-5800
 
     dewombl@cs.sandia.gov
     (505) 845-7471

  2. David S. Greenberg
     Sandia National Laboratories, Dept. 1423
     P.O. Box 5800
     Albuquerque, NM 87185-5800
 
     dsgreen@cs.sandia.gov
     (505) 845-7601
 
  3. Rolf E. Riesen
     Sandia National Laboratories, Dept. 1424
     P.O. Box 5800
     Albuquerque, NM 87185-5800
 
     reriese@cs.sandia.gov
     (505) 845-7749
 
  4. Greg Henry
     Intel SSD, Bldg. CO6-08
     14924 Greenbrier Parkway
     Beaverton, OR 97006

     henry@ssd.intel.com
     (503) 531-5679

  5. Joe Kotulski
     Sandia National Laboratories, Dept 9352
     P. O. Box 5800
     Albuquerque, NM 87185-1166

     jdkotul@sandia.gov
     (505)-845-7955
     (Ported the solver to the Tflop machine, MPI changed to pass bytes
       for the matrix entries)
 
Authors' conditions of release:
  1. The authors receive acknowledgement of authorship in any publication 
     or in any distributions of code in which this code or any part of this
     code is used;
  2. The authors is notified of any problems encountered with the
     code;
*/

#include <math.h>
#include <stdio.h>
#ifdef MPI
#include <mpi.h>
#endif
#include "../include/defines.h"
#include "BLAS_prototypes.h"
#include "init.h"
#include "time.h"
#include "factor.h"
#include "solve.h"
#include "macros.h"
#include "malloc.h"
#include "pcomm.h"

#define TIMERTYPE1 2048
#define TIMERTYPE2 2049
#define TIMERTYPE3 2050
#define TIMERTYPE4 2051
#define TIMERTYPE5 2052
#define TIMERTYPE6 2053
#define TIMERTYPE7 2054
#define TIMERTYPE8 2055
#define TIMERTYPE9 2056

#define BCASTTYPE 2060

#define RESIDTYPE0 2070
#define RESIDTYPE1 2071
#define RESIDTYPE2 2072

#define DEFBLKSZ  32
#define NUMARGS 8
#define MINHEAP 14000000


#ifdef POINTERS_64_BITS

#define ALIGNED_MALLOC    MALLOC_64

#else

#define ALIGNED_MALLOC    MALLOC_32

#endif POINTERS_64_BITS

/* the number of integer arguments passed by node 0 to rest of nodes 
   currently are matrix size, row cube size, segment size, flags, 
   BLAS blocksize, number of disk controllers */

int   dsg;
int   me, me_proc, host;	/* processor id information */

int   nprocs_cube;		/* num of procs in the allocated cube */
int   nprocs_row;		/* num of procs to which a row is assigned */
int   nprocs_col;		/* num of procs to which a col is assigned */
int   max_procs;                /* max num of procs in any dimension */


int   nrows_matrix;		/* number of rows in the matrix */
int   ncols_matrix;		/* number of cols in the matrix */
int   matrix_size;		/* order of matrix=nrows_matrix=ncols_matrix */
int   DISKS;		        /* number of disk controllers */


/* the number of integer arguments passed by node 0 to rest of nodes 
   currently are matrix size, row cube size, segment size, flags, 
   BLAS blocksize, number of disk controllers */

int   dsg;
int   me, me_proc, host;	/* processor id information */

int   nprocs_cube;		/* num of procs in the allocated cube */
int   nprocs_row;		/* num of procs to which a row is assigned */
int   nprocs_col;		/* num of procs to which a col is assigned */
int   max_procs;                /* max num of procs in any dimension */
int errcode ;
#ifdef MPI
int  myrow,mycol;
MPI_Comm row_comm,col_comm;
#endif


int   nrows_matrix;		/* number of rows in the matrix */
int   ncols_matrix;		/* number of cols in the matrix */
int   matrix_size;		/* order of matrix=nrows_matrix=ncols_matrix */
int   DISKS;		        /* number of disk controllers */

int   my_first_row;		/* proc position in a row */
int   my_first_col;		/* proc position in a col */

BLAS_INT my_rows;			/* num of rows I own */
BLAS_INT my_cols;			/* num of cols I own */

int   nrhs;                     /* number of right hand sides in the matrix */
BLAS_INT my_rhs;                   /* number of right hand sides that I own */

int   nsegs_row;		/* num of segs to which each row is assigned */
int   ncols_seg;		/* num of cols in each segment */
int   ncols_last;		/* num of cols in the rightmost segment */

#ifdef IO
int   bytes_per_seg;		/* number of bytes in a segment */
#endif

BLAS_INT i_one = 1;

int   my_cols_seg;		/* num of cols I own in a seg */
BLAS_INT my_cols_last;		/* num of cols I own in the rightmost seg */

BLAS_INT mat_stride;               /* stride to second dimension of mat */
BLAS_INT col1_stride;              /* stride to second dimension of col1 */
BLAS_INT row1_stride;              /* stride to second dimension of row1 */
int   size_of_entry;            /* number of bytes in a double or complex */

int   *pivot_vec;               /* stores pivot information */
char  *pvec;
char  *prow3;
char  *pcol1a;
char  *prow1a;
char  *dumb;

DATA_TYPE *mat;			/* incore storage for col being factored */
DATA_TYPE *rhs;                 /* storage for right hand sides */
DATA_TYPE *rhs_copy;            /* store the original rhs for comparison */
DATA_TYPE *col1,*col1a;	        /* ptrs to col used for updating a col */
DATA_TYPE *col2;		/* ptr to col received in message buf */
DATA_TYPE *row1,*row1a;		/* ptr to diagonal row */
DATA_TYPE *row2;		/* ptr to pivot row */
DATA_TYPE *row3;                /* ptr to row used for pivots */
double    resid, tresid;        /* residuals */

int zpflag;

int debug_display;

BLAS_INT blksz;			/* block size for BLAS 3 operations */
int   rhs_blksz;                /* agglomeration block size for backsolve */
BLAS_INT colcnt;			/* number of columns stored for BLAS 3 ops */
int   totmem = 0;		/* Total memory (heap) used */

#ifdef COMPLEX
volatile int   MSPLIT=100;      /* ZGEMM splitting parameter */
#else
volatile int   MSPLIT=20 ;      /* DGEMM splitting parameter */
#endif

volatile int goflag=0;
volatile int goflag1=0;

extern int _my_rank;

int 
main(int argc, char *argv[])
{
    extern char *optarg;
    int   error;
    char  ch;
    static int buf[NUMARGS];
    int   nnodes;		/* number of nodes */
    int   mlen;
    int   i, j;

    double normA1;              /* 1-norm of the matrix */
    double normAinf;            /* inf-norm of the matrix */
    double macheps;             /* machine epsilon */
    double max_error, gmax_error;
    double max_resid, gmax_resid;
    double max_x, gmax_x;

    int   temp;

    double timer0, timer1, timer2, timer3, timer4, timer5, timer6;
    double timer10, timer11, timer12;
    double avg_run, avg_lu, avg_comp, avg_write, avg_read, ops;	
                                /* Calculate MFLOPS */
    DATA_TYPE *p,*ptr1,*ptr2,*ptr3;/* ptr to a segment for printing */

/*  Initialize into the parallel process                            */

#ifdef MPI
    /* initialize MPI and find out number of nodes and rank */
    /*  printf("Initializing into MPI\n");  */

    MPI_Init (&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &me) ;

    MPI_Comm_size(MPI_COMM_WORLD, &nprocs_cube) ;
    /* printf("Finished Initializing into MPI\n");  */
#ifdef MPI_ERR_CHECK
    /*
    ** we want MPI to return error conditions to us
    */
    MPI_Errhandler_set(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
#endif

#else
    me = mynode();
    nprocs_cube = numnodes();
#endif

    for(i=0;i<NUMARGS;i++) buf[i]=-1;
    error = 0;

    debug_display = 0;

    if (me == 0) {
	/* check command line args */
	while ((ch = getopt(argc, argv, "n:r:s:b:d:v:m:CPRWD")) != EOF) {
	    switch (ch) {
		case 'n':
		    sscanf(optarg, "%d", &buf[0]);
		    break;
		case 'r':
		    sscanf(optarg, "%d", &buf[1]);
		    break;
		case 'b':
		    sscanf(optarg, "%d", &buf[4]);
		    break;
		case 'v':
		    sscanf(optarg, "%d", &buf[6]);
		    break;
		case 'm':
		    sscanf(optarg, "%d", &buf[7]);
		    break;
		case 'D':
                    debug_display = 1;
                    break;
		default:
		    error = 1;
	    }
	}

	if (error) {
	    fprintf(stderr, 
                "Usage: %s [-n ms] [-r pr] [-v rhs] [-b blk] [-m split]\n",
		argv[0]);
	    fprintf(stderr, "       ms    matrix size\n");
	    fprintf(stderr, "       pr    processors per row\n");
	    fprintf(stderr, "       rhs   number of right hand sides\n");
	    fprintf(stderr, "       blk   BLAS3 block size\n");
	    fprintf(stderr, "       split cache blocking size\n");
	    buf[0] = -1;
#ifdef  COUGAR
            bcast_all(me, 0, (char *) buf, mlen, BCASTTYPE);
#endif
	    exit(-1);
	}
#ifdef COUGAR
	fprintf(stderr, "COUGAR version.(heap= %d)\n", heap_size());
#endif
#ifdef MPI
       /*
       fprintf(stderr, "MPI version.(heap= %d)\n");
      */
#endif
	if (buf[0] < 0) {
	    fprintf(stderr, "Enter size of matrix: ");
	    scanf("%d", &buf[0]);
	}
	if (buf[1] < 0) {
	    fprintf(stderr, 
              "Enter number of processors to which each row is assigned ");
	    scanf("%d", &buf[1]);
	}
        buf[2] = buf[0];
	if (buf[4] < 0) {
           fprintf(stderr," Enter Blocksize  ");
            scanf("%d", &buf[4]);  
           /*  buf[4] = DEFBLKSZ;  */
	}
	if (buf[6] < 0) {
            buf[6] = 1;
	}
	if (buf[6] > 1) {
            buf[6] = 1;
	}
        if (buf[7] < 0) {
          buf[7] = MSPLIT;
        }
    }
    mlen = NUMARGS * sizeof(int);
#ifdef MPI

#ifdef DEBUG_COMM
if (me == 0) printf("MPI_Bcast args\n");
#endif

    MPI_Bcast(buf,mlen,MPI_CHAR,0,MPI_COMM_WORLD);
#ifdef DEBUG_COMM
if (me == 0) printf("DONE MPI_Bcast args\n");
#endif
   /*
   for(i=0;i<NUMARGS;i++) {
     printf(" The control variables  %d  on %d\n",buf[i],me);
   }  */
#else
    bcast_all(me, 0, (char *) buf, mlen, BCASTTYPE);
#endif
    if (buf[0] <= 0) {
	/* Node 0 received garbage input and terminated */
	exit(-1);
    }

    blksz = buf[4];

    matrix_size = buf[0];
    nrows_matrix = matrix_size;
    ncols_matrix = matrix_size;

    nprocs_row = buf[1];
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
       printf("Node %d: my row = %d but rank in col = %d\n",me,myrow,checkrow);
     if (mycol != checkcol)
       printf("Node %d: my col = %d but rank in row = %d\n",me,mycol,checkcol);
   }

#endif

    my_first_col = mesh_col(me);
    my_first_row = mesh_row(me);

    my_rows = (BLAS_INT) (nrows_matrix / nprocs_col);
    if (my_first_row < nrows_matrix % nprocs_col)
	++my_rows;
    my_cols = ncols_matrix / nprocs_row;
    if (my_first_col < ncols_matrix % nprocs_row)
	++my_cols;

    nrhs = buf[6];

#ifdef TURBO
#ifdef COMPLEX
    blksz = (blksz >> 1) << 1;
    MSPLIT = buf[7];
    if ( 2*MSPLIT*blksz + 2*blksz + 4*MSPLIT > 2032 )
        MSPLIT= (2032-2*blksz)/(2*blksz+4);
    MSPLIT= (MSPLIT >> 1) << 1;
    if (MSPLIT < 4) MSPLIT = 4;
    if (blksz < 4) blksz = 4;
#else
    blksz = 16;
    MSPLIT = 20;
#endif
#else
    blksz = buf[4];
    MSPLIT = buf[7];
#endif

#ifdef IO
    ncols_seg = (buf[2] < ncols_matrix) ? buf[2] : ncols_matrix;
    ncols_seg = ((ncols_seg + max_procs - 1) / max_procs) * max_procs;

    nsegs_row = (ncols_matrix + ncols_seg - 1) / ncols_seg;
    my_cols_seg = ncols_seg / nprocs_row;
    ncols_last = ncols_matrix - ncols_seg * (nsegs_row - 1);
    my_cols_last = my_cols - my_cols_seg * (nsegs_row - 1);
    bytes_per_seg = (my_rows + 1) * my_cols_seg * sizeof(DATA_TYPE);
#else
    ncols_seg =  ncols_matrix;
    nsegs_row = 1;
    my_cols_seg = my_cols;
    ncols_last = ncols_matrix;
    my_cols_last = my_cols;
#endif

    nrhs = (nrhs < ncols_seg) ? nrhs : ncols_seg;
    my_rhs = nrhs / nprocs_row;
    if (my_first_col < nrhs % nprocs_row) ++my_rhs;

/* test statement LAFISK - what's this all about??? */
    my_rhs = 1;
    dsg = my_rhs;
    
#ifdef COMPLEX
    ops = 8.0 * matrix_size * matrix_size * matrix_size / 3.0;
    ops += nrhs * 8.0 * matrix_size * matrix_size;
#else
    ops = 2.0 * matrix_size * matrix_size * matrix_size / 3.0;
    ops += nrhs * 2.0 * matrix_size * matrix_size;
#endif

    if (me == 0) {
      fprintf(stderr, "\nMatrix size: %d x %d\n", nrows_matrix, ncols_matrix);
#ifdef COMPLEX
      fprintf(stderr, "Data type is double precision, complex\n");
#else
      fprintf(stderr, "Data type is double precision, real\n");
#endif
      fprintf(stderr, "Number of right hand sides is %d\n",nrhs);
      fprintf(stderr, "Block size is %d\n", blksz);
      fprintf(stderr, "Number of processors: %d\n", nprocs_cube);
      fprintf(stderr, 
              "Number of processors used: %d\n", nprocs_row*nprocs_col);
      fprintf(stderr, 
              "Processor decomposition is %d x %d\n", nprocs_col, nprocs_row);
#ifdef TURBO
      fprintf(stderr, "Using cache-based gemms with %d processors/node\n",
              PROCS);
      fprintf(stderr, "Cache blocking is %d\n", MSPLIT);
#else
      fprintf(stderr, "Using libkmath gemms\n");
#endif
    }

    if (nrhs > 0) {
      temp = (my_rhs > 1) ? my_rhs : 1;
      rhs_copy = (DATA_TYPE *) ALIGNED_MALLOC((my_rows+1)*temp*sizeof(DATA_TYPE),&dumb);
      totmem += (my_rows+1)*temp*sizeof(DATA_TYPE);
      if (rhs_copy == NULL) {
	fprintf(stderr, "Node %d: Out of memory\n", me);
	exit(-1);
      }
    }

    pivot_vec = (int *) ALIGNED_MALLOC(my_cols_last * sizeof(int),&pvec);
//    pivot_vec = (int *) MALLOC_65(my_cols_last * sizeof(int), &pvec);
    totmem += my_cols_last * sizeof(int);
    if (pivot_vec == NULL) {
	fprintf(stderr, "Node %d: Out of memory\n", me);
	exit(-1);
    }

    row3 = (DATA_TYPE *) ALIGNED_MALLOC((my_cols_last + (2*blksz) + dsg) 
                                    * sizeof(DATA_TYPE),&prow3);
    totmem += (my_cols_seg + (2*blksz) + dsg) * sizeof(DATA_TYPE);
    if (row3 == NULL) {
	fprintf(stderr, "Node %d: Out of memory\n", me);
	exit(-1);
    }

    row2 = (DATA_TYPE *) ALIGNED_MALLOC((my_cols_last + (2*blksz) + dsg ) 
                                    * sizeof(DATA_TYPE), &dumb);
    totmem += (my_cols_seg + (2*blksz) + dsg) * sizeof(DATA_TYPE);
    if (row2 == NULL) {
	fprintf(stderr, "Node %d: Out of memory\n", me);
	exit(-1);
    }

    col2 = (DATA_TYPE *) ALIGNED_MALLOC((my_rows + 1) * sizeof(DATA_TYPE),&dumb);
    totmem += (my_rows + 1) * sizeof(DATA_TYPE);
    if (col2 == NULL) {
	fprintf(stderr, "Node %d: Out of memory\n", me);
	exit(-1);
    }

    /* this is a two dimensional array, its leading dimension is 
       coded into routines change with care */

    col1 = (DATA_TYPE *) ALIGNED_MALLOC(my_rows*blksz* sizeof(DATA_TYPE),&dumb);
    col1_stride = my_rows;
    totmem += my_rows*blksz* sizeof(DATA_TYPE);
    if (col1 == NULL) {
      fprintf(stderr, "Node %d: Out of memory\n", me);
      exit(-1);
    }

    col1a = (DATA_TYPE *) ALIGNED_MALLOC(my_rows*blksz* sizeof(DATA_TYPE),&pcol1a);
    col1_stride = my_rows;
    totmem += my_rows*blksz* sizeof(DATA_TYPE);
    if (col1a == NULL) {
      fprintf(stderr, "Node %d: Out of memory\n", me);
      exit(-1);
    }

    /* this is a two dimensional array, its leading dimension is 
       coded into routines change with care */

    row1 = (DATA_TYPE *) ALIGNED_MALLOC(blksz*(my_cols_last+blksz+dsg)
                                   * sizeof(DATA_TYPE), &dumb);
    row1_stride = my_cols_last+blksz+dsg;
    totmem += blksz * (my_cols_last + blksz + dsg) * sizeof(DATA_TYPE);
    if (row1 == NULL) {
	fprintf(stderr, "Node %d: Out of memory\n", me);
	exit(-1);
    }

    row1a = (DATA_TYPE *) ALIGNED_MALLOC(blksz*(my_cols_last+blksz+dsg)
                                   * sizeof(DATA_TYPE), &prow1a);
    row1_stride = my_cols_last+blksz+dsg;
    totmem += blksz * (my_cols_last + blksz + dsg) * sizeof(DATA_TYPE);
    if (row1a == NULL) {
	fprintf(stderr, "Node %d: Out of memory\n", me);
	exit(-1);
    }

    /* this is a two dimensional array, its leading dimension is 
       coded into routines change with care */

    mat = (DATA_TYPE *) ALIGNED_MALLOC(my_rows * (my_cols_last+dsg) 
                                  * sizeof(DATA_TYPE), &dumb);

    mat_stride = my_rows;
    totmem += my_rows * (my_cols_last+dsg) * sizeof(DATA_TYPE);
    if (mat == NULL) {
      fprintf(stderr, "Node %d: Out of memory\n", me);
      exit(-1);
    }


    if (me == 0) {
	fprintf(stderr, "Memory used (heap): %d\n", totmem);
    }

    /*
     * the matrix will be stored on a per processor basis.  The processor is
     * responsible for its portion of each segment. The size of its portion
     * of each segment is in bytes[] so its total file size is the sum of the
     * entries of bytes[].
     */

    timer0 = (double) 0.0;
    timer1 = (double) 0.0;

#ifdef MPI
#ifdef DEBUG_COMM
if (me == 0) printf("MPI_Barrier\n");
#endif
    MPI_Barrier(MPI_COMM_WORLD);
#ifdef DEBUG_COMM
if (me == 0) printf("DONE MPI_Barrier\n");
#endif

#else
    gsync();
#endif

    timer0 = seconds(timer0);
    init_seg(mat, 0);

    if ((nrhs > 0)) {
      rhs = mat + my_rows*my_cols_last;
      init_rhs(rhs, mat, 0);

      XCOPY(my_rows, rhs, i_one, rhs_copy, i_one);
    }
    timer0 = seconds(timer0);

    /* initialize communication routines */
    initcomm();
    
    if (me == 0)
	fprintf(stderr, "Initialization time = ");
    timing(timer0, TIMERTYPE1);

    if (me == 0)
	fprintf(stderr, "\n");

    timer0 = (double) 0.0;
    timer2 = (double) 0.0;
    timer4 = (double) 0.0;
    timer6 = (double) 0.0;

#ifdef MPI
#ifdef DEBUG_COMM
if (me == 0) printf("MPI_Barrier after initialization\n");
#endif
    MPI_Barrier(MPI_COMM_WORLD);
#ifdef DEBUG_COMM
if (me == 0) printf("Done MPI_Barrier\n");
#endif
#else
    gsync();
#endif

    if ((me == 0)) {
      fprintf(stderr, "Beginning Factor\n");
    }
    timer0 = seconds(timer0);
    timer4 = seconds(timer4);

    factor(mat);
    timer4 = seconds(timer4);

    /*
    if (me == owner(nrows_matrix-1, ncols_matrix-1));
    if (me == 0) {
      p = mat + (my_cols_last - dsg)*(my_rows) + my_rows - 1;
      p = rhs + my_rows - 1;
      fprintf(stderr, "\nme = %d, %18.10f\n       %18.10f\n       %18.10f\n       %18.10f, last pivot = %f\n\n",
           me, *(p-3), *(p-2), *(p-1),*p,*(p+1));  
    }
    */

    /* solve system if there are right hand sides */

    if ((nrhs > 0)) {
      if ((me == 0)) {
        fprintf(stderr, "Beginning solve\n");
      }
      timer6 = seconds(timer6);
      back_solve6(mat, rhs);
      timer6 = seconds(timer6);
    }

    timer0 = seconds(timer0);

    if (me == 0)
	fprintf(stderr, "\nTotal run time = ");
    avg_run = timing(timer0, TIMERTYPE3);
    if (me == 0)
	fprintf(stderr, "Time in factor() = ");
    avg_lu = timing(timer4, TIMERTYPE7);
    if (me == 0)
	fprintf(stderr, "Time in solve() = ");
    timing(timer6, TIMERTYPE8);
    if (me == 0)
	fprintf(stderr, "\nMFLOPS per node = %f, total = %f\n",
		((ops / avg_run) / 1000000.0) / nprocs_cube,
		(ops / avg_run) / 1000000.0);
    if (me == 0)
	fprintf(stderr, "MFLOPS per node in factorization = %f, total = %f\n",
		((ops / avg_lu) / 1000000.0) / nprocs_cube,
		(ops / avg_lu) / 1000000.0);

    //free(pivot_vec);
    free(pvec);
    //free(row3);
    free(prow3);
    //free(col1a);
    free(pcol1a);
    //free(row1a);
    free(prow1a);



    /* Compute quantities for the linpack check */

    if ((nrhs > 0) && (nsegs_row == 1)) {
      
      init_seg(mat, 0);

      mat_vec(mat, 0, rhs);

      ptr3 = row1;
      max_error = (double) 0.0;
      if (me == row_owner(0)) {
        for (i=0; i<my_cols; i++) {
#ifdef COMPLEX
          tresid = fabs((*ptr3).r - (double) 1.0);
          max_error = (tresid > max_error) ? tresid : max_error;
          tresid = fabs((*ptr3).i - (double) 0.0);
          max_error = (tresid > max_error) ? tresid : max_error;
#else
          tresid = fabs(*ptr3 - (double) 1.0);
          max_error = (tresid > max_error) ? tresid : max_error;
#endif
          ptr3++;
        }
      }
      gmax_error=max_error;
      gmax_error = max_all(max_error,RESIDTYPE0);
      ptr1 = rhs;
      ptr2 = rhs_copy;
      max_resid = (double) 0.0;
      if (me == col_owner(0)) {
        for (i=0; i<my_rows; i++) {
#ifdef COMPLEX
          tresid = fabs((*ptr1).r - (*ptr2).r);
          max_resid = (tresid > max_resid) ? tresid : max_resid;
          tresid = fabs((*ptr1).i - (*ptr2).i);
          max_resid = (tresid > max_resid) ? tresid : max_resid;
#else
          tresid = fabs(*ptr1 - *ptr2);
          max_resid = (tresid > max_resid) ? tresid : max_resid;
#endif
          ptr1++;
          ptr2++;
        }
      }

      if ( me == 0 ) fprintf(stderr,"Calling max_all() for gmax_resid\n");

      gmax_resid = max_all(max_resid,RESIDTYPE1);

      max_x = (double) 0.0;    /* norm(x,inf) */
      ptr3 = row1;
      if (me == row_owner(0)) {
        for (i=0; i<my_cols; i++){
#ifdef COMPLEX
          tresid = fabs((*ptr3).r);
          max_x = (tresid > max_x) ? tresid : max_x;
          tresid = fabs((*ptr3).i);
          max_x = (tresid > max_x) ? tresid : max_x;
#else
          tresid = fabs(*ptr3);
          max_x = (tresid > max_x) ? tresid : max_x;
#endif
          ptr3++;
        }
      }

      if ( me == 0 ) fprintf(stderr,"Calling max_all() for gmax_x\n");

      gmax_x = max_all(max_x,RESIDTYPE2);
      normA1 = one_norm(mat, 0);
      normAinf = inf_norm(mat, 0);
      macheps = init_eps();

      if (me == 0){
        fprintf(stderr, "\nCheck error in solution\n");
        fprintf(stderr, "  Norm(error,inf) = %12.6e\n",gmax_error);
        fprintf(stderr, "  Norm(resid,inf) = %12.6e\n",gmax_resid);
        fprintf(stderr, "  Norm(x,inf) = %12.6e\n",gmax_x);
        fprintf(stderr, "  Norm(A,1) = %12.6e\n",normA1);
        fprintf(stderr, "  Norm(A,inf) = %12.6e\n",normAinf);
        fprintf(stderr, "  Machine epsilon = %12.6e\n",macheps);
        fprintf(stderr, "  Size of matrix = %d\n",ncols_matrix);

        if ((normAinf > 0.0) && (gmax_x > 0.0))
	    fprintf(stderr, 
             "  Norm(resid,inf)/(norm(A,inf)*norm(x,inf)) = %12.6e\n",
                gmax_resid/normAinf/gmax_x);

        if (gmax_x > 0.0)
	    fprintf(stderr, 
              "  Norm(error,inf)/norm(x,inf) = %12.6e\n", 
                 gmax_error/gmax_x);

        fprintf(stderr, 
       "  n * norm(A,1) * macheps = %12.6e\n",ncols_matrix*macheps*normA1);

        if ( ((normAinf > 0.0) && (gmax_x > 0.0)) &&
            ((gmax_resid/normAinf/gmax_x) > (ncols_matrix*macheps*normA1)) )
          fprintf(stderr,"SOLUTION FAILS RESIDUAL CHECK %12.6e > %12.6e\n",
                    (gmax_resid/normAinf/gmax_x) , (ncols_matrix*macheps*normA1));
        else
          fprintf(stderr,"Solution passes residual check\n");


printf("\n");
printf(" size   -r  Mbytes     MFLOPS     LU MFLOPS  seconds\n");
printf(" ----   --- ------   ------------ ---------- ------\n");

printf("%05d  %04d  %02.3f  %05.1f/%05.1f  %05.1f/%05.1f  %03d\n",
ncols_matrix, nprocs_row, (float)totmem/(1024.0*1024.0),
((ops / avg_run) / 1000000.0) / nprocs_cube,
(ops / avg_run) / 1000000.0,
((ops / avg_lu) / 1000000.0) / nprocs_cube,
(ops / avg_lu) / 1000000.0,
(int)(avg_run+.5));



      }
    return 0;
    }


#ifdef MPI
    MPI_Finalize();
#endif
}
/*
** code to print out MPI error strings
*/
static char errstring[256];

void showMPIerr(int errnum)
{
int len;

    MPI_Error_string(errnum, errstring, &len);

    printf("(%d) MPI error: %s\n",me,errstring);
} 
