#define XCOPY(len,v1,s1,v2,s2) zcopy_(&len,v1,&s1,v2,&s2)
#define XSCAL(len,scal,v1,s1) zscal_(&len,&scal,v1,&s1)
#define XAXPY(len,scal,v1,s1,v2,s2) zaxpy_(&len,&scal,v1,&s1,v2,&s2)
#define IXAMAX(len,v1,s1) izamax_(&len,v1,&s1)
#define XASUM(len,v1,s1) dzasum(&len,&v1,&s1)
#define XDOT(len,v1,s1,v2,s2) zdotu(&len,v1,&s1,v2,&s2)
#define XGEMM_ zgemm
#define XGEMM(flagA,flagB,l1,l2,l3,scal1,A,s1,B,s2,scal2,C,s3) zgemm_(&flagA,&flagB,&l1,&l2,&l3,&scal1,A,&s1,B,&s2,&scal2,C,&s3)

