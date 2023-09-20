int   me;                       /* processor id information */

int   nprocs_cube;		/* num of procs in the allocated cube */
int   nprocs_row;		/* num of procs to which a row is assigned */
int   nprocs_col;		/* num of procs to which a col is assigned */
int   max_procs;		/* max num of procs in any dimension */

int   nrows_matrix;		/* number of rows in the matrix */
int   ncols_matrix;		/* number of cols in the matrix */
int   matrix_size;		/* order of matrix=nrows_matrix=ncols_matrix */

int   my_first_row;		/* proc position in a row */
int   my_first_col;		/* proc position in a col */

BLAS_INT my_rows;			/* num of rows I own */
BLAS_INT my_cols;			/* num of cols I own */

BLAS_INT my_cols_last;
int   my_cols_seg;              /* vars for compatibility with init */
int   ncols_last;

int   nrhs;              /* number of right hand sides in the matrix */
BLAS_INT my_rhs;            /* number of right hand sides that I own */

BLAS_INT   mat_stride;          /* stride to second dimension of mat */
BLAS_INT   col1_stride;         /* stride to second dimension of col1 */
BLAS_INT   row1_stride;         /* stride to second dimension of row1 */

DATA_TYPE *mat;			/* incore storage for col being factored */
DATA_TYPE *rhs;                 /* storage for right hand sides */

DATA_TYPE *col1;         	/* ptrs to col used for updating a col */
DATA_TYPE *col2;		/* ptr to col received in message buf */
DATA_TYPE *row1;		/* ptr to diagonal row */
DATA_TYPE *row2;		/* ptr to pivot row */
DATA_TYPE *row3;
DATA_TYPE *rhs_temp;
                                /* ptr to row used for pivots */
int *pivot_vec;                 /* stores pivot information */

BLAS_INT blksz;			/* block size for BLAS 3 operations */
int   rhs_blksz;                /* agglomeration block size for backsolve */
BLAS_INT colcnt;		/* number of columns stored for BLAS 3 ops */
int   totmem = 0;		/* Total memory (heap) used */

#ifdef MPI
int  myrow,mycol;
MPI_Comm row_comm,col_comm;
#endif


/* volatile int   MSPLIT;           ZGEMM splitting parameter */

