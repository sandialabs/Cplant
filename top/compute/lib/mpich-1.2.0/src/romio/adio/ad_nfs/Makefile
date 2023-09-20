# Generated automatically from Makefile.in by configure.

CPLANT_TOP  = ${HOME}/cplant/src/new/top

CC          = gcc
AR          = ar crl
LIBNAME     = ${CPLANT_TOP}/compute/lib/mpich-1.2.0/alpha-linux/libmpi.a
srcdir      = ${CPLANT_TOP}/compute/lib/mpich-1.2.0/src/romio/adio/ad_nfs

INCLUDE_DIR = -I${srcdir}/../include -I${CPLANT_TOP}/compute/lib/mpich-1.2.0/include
CFLAGS      =   -O -DFORTRANUNDERSCORE -D__NO_AIO -D__NFS -D__NEEDS_MPI_TEST -D__MPICH  -D__INT_LT_POINTER=1 -D__HAVE_LONG_LONG_64=1 -D__HAVE_MPI_LONG_LONG_INT=1 -D__HAS_MPI_INFO=1 -D__HAS_MPI_DARRAY_SUBARRAY=1 -DHAVE_WEAK_SYMBOLS=1 -DHAVE_PRAGMA_WEAK=1 $(INCLUDE_DIR)

VPATH=.:$(srcdir)

AD_NFS_OBJECTS = ad_nfs_close.o ad_nfs_read.o ad_nfs_rdcoll.o \
      ad_nfs_wrcoll.o ad_nfs_open.o ad_nfs_write.o ad_nfs_done.o \
      ad_nfs_fcntl.o ad_nfs_iread.o ad_nfs_iwrite.o ad_nfs_wait.o \
      ad_nfs_flush.o ad_nfs_seek.o ad_nfs_resize.o ad_nfs_hints.o \
      ad_nfs_setsh.o ad_nfs_getsh.o 

default: $(LIBNAME)

.c.o:
	$(CC) $(CFLAGS) -c $<

$(LIBNAME): $(AD_NFS_OBJECTS)
	$(AR) $(LIBNAME) $(AD_NFS_OBJECTS)

clean:
	@rm -f *.o
