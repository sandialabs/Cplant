#ifdef CBLAS 
#define XCOPY zcopy
#define XSCAL zscal
#define XAXPY zaxpy
#define IXAMAX izamax
#define XASUM dzasum
#define XDOT zdotu

#else

#define XCOPY(len,v1,s1,v2,s2) zcopy_(&len,v1,&s1,v2,&s2)
#define XSCAL(len,scal,v1,s1) zscal_(&len,&scal,v1,&s1)
#define XAXPY(len,scal,v1,s1,v2,s2) zaxpy_(&len,&scal,v1,&s1,v2,&s2)
#define IXAMAX(len,v1,s1) izamax_(&len,v1,&s1)
#define XASUM(len,v1,s1) dzasum_(&len,v1,&s1)
#define XDOT(v3,len,v1,s1,v2,s2) zdotu_(v3,&len,v1,&s1,v2,&s2)


#endif
