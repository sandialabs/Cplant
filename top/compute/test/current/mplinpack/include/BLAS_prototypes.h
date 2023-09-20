
#ifdef COMPLEX
dcomplex zdotu(dcomplex *z,BLAS_INT n, dcomplex *x, BLAS_INT incx, dcomplex *y, BLAS_INT incy);
void zdotu_(dcomplex *z,BLAS_INT *n, dcomplex *x, BLAS_INT *incx, dcomplex *y, BLAS_INT *incy);
void zscal(BLAS_INT n, dcomplex a, dcomplex *r, BLAS_INT m);
void zscal_(BLAS_INT *n, dcomplex *a, dcomplex *r, BLAS_INT *m);
void zaxpy(BLAS_INT n, dcomplex a, dcomplex *b, BLAS_INT j, dcomplex *r, BLAS_INT m);
void zaxpy_(BLAS_INT *n, dcomplex *a, dcomplex *b, BLAS_INT *j, dcomplex *r, BLAS_INT *m);
void zcopy(BLAS_INT n, dcomplex *a, BLAS_INT i, dcomplex *r, BLAS_INT m);
void zcopy_(BLAS_INT *n,dcomplex *a,BLAS_INT *i,dcomplex *r, BLAS_INT *m);
double dzasum(BLAS_INT n, dcomplex *x, BLAS_INT incx);
double dzasum_(BLAS_INT *n, dcomplex *x, BLAS_INT *incx);
BLAS_INT izamax(BLAS_INT n, dcomplex *x, BLAS_INT incx);
BLAS_INT izamax_(BLAS_INT *n,dcomplex *x, BLAS_INT *incx);
void zgemm_(char *transA, char *transB, BLAS_INT *num_r, BLAS_INT *num_c, BLAS_INT *colcnt, 
      dcomplex *scaler, dcomplex *ptr2, BLAS_INT *stride, dcomplex *ptr4, BLAS_INT *stride1,
      dcomplex *scaler1, dcomplex *ptr1, BLAS_INT *stride2);
#endif


void daxpy(BLAS_INT *n, double *a, double *b, BLAS_INT *j, double *r, BLAS_INT *m);
void dcopy(BLAS_INT *n, double *a, BLAS_INT *i, double *r, BLAS_INT *m);
void dscal(BLAS_INT *n, double *a, double *r, BLAS_INT *m);
void maxmgv(double *a, BLAS_INT *i, double *r, BLAS_INT *lr, BLAS_INT *n);


int dcopy_(BLAS_INT *n, double *a, BLAS_INT *i, double *r, BLAS_INT *m);
int dscal_(BLAS_INT *n, double *a, double *r, BLAS_INT *m);
int idamax_(BLAS_INT *n, double *a, BLAS_INT *incx);
double dasum_(BLAS_INT *n, double *dx, BLAS_INT *incx);
double ddot_(BLAS_INT *n, double *dx, BLAS_INT *incx, double *dy, BLAS_INT *incy);
int dgemm_(char *transA, char *transB, BLAS_INT *num_r, BLAS_INT *num_c, BLAS_INT *colcnt, 
        double *scaler, double *ptr2, BLAS_INT *stride, double *ptr4, BLAS_INT *stride1,
        double *scaler1, double *ptr1, BLAS_INT *stride2);
int daxpy_(BLAS_INT *n, double *da, double *dx, BLAS_INT *incx, double *dy, BLAS_INT *incy);
