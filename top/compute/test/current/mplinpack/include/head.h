extern void  zcopy_(&len,&v1,&s1,&v2,&s2) ;
extern void  zscal_(&len,&scal,v1,&s1);
extern void zaxpy_(&len,&scal,v1,&s1,v2,&s2);
extern int izamax_(&len,&v1,&s1);
extern void dzasum_(&len,&v1,&s1);
extern void zdotu_(&len,v1,&s1,v2,&s2);
extern void zgemm_(&flagA,&flagB,&l1,&l2,&l3,&scal1,A,&s1,B,&s2,&scal2,C,&s3);
