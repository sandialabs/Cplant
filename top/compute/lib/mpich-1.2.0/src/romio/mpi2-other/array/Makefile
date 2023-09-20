# Generated automatically from Makefile.in by configure.

CPLANT_TOP  = ${HOME}/cplant/sc/new/top

CC          = gcc
AR          = ar crl
LIBNAME     = ${CPLANT_TOP}/compute/lib/mpich-1.2.0/alpha-linux/libmpi.a
RANLIB      = ranlib
srcdir      = ${CPLANT_TOP}/compute/lib/mpich-1.2.0/romio/mpi2-other/array

INCLUDE_DIR = -I/Net/pecos/puma/home/rbbrigh/cplant/src/dnsands/mpich-1.2.0/include -I../../include -I${srcdir}/../../mpi-io -I${srcdir}/../../adio/include
CFLAGS      =   -O -DFORTRANUNDERSCORE -D__NO_AIO -D__NFS -D__NEEDS_MPI_TEST -D__MPICH  -D__INT_LT_POINTER=1 -D__HAVE_LONG_LONG_64=1 -D__HAVE_MPI_LONG_LONG_INT=1 -D__HAS_MPI_INFO=1 -D__HAS_MPI_DARRAY_SUBARRAY=1 -DHAVE_WEAK_SYMBOLS=1 -DHAVE_PRAGMA_WEAK=1 $(MPIOPROFILE) $(INCLUDE_DIR)

VPATH=.:$(srcdir)

MPIO_OBJECTS = subarray.o darray.o

MPIO_TMP_POBJECTS = subarray.p darray.p

MPIO_REAL_POBJECTS = _subarray.o _darray.o

all: $(LIBNAME)

.SUFFIXES: $(SUFFIXES) .p

.c.o:
	$(CC) $(CFLAGS) -c $<

.c.p:
	@cp $(srcdir)/$*.c _$*.c
	$(CC) $(CFLAGS) -c _$*.c
	@rm -f _$*.c

profile: $(MPIO_TMP_POBJECTS)
	$(AR) $(LIBNAME) $(MPIO_REAL_POBJECTS)
	@rm -f _*.o

$(LIBNAME): $(MPIO_OBJECTS)
	$(AR) $(LIBNAME) $(MPIO_OBJECTS)

clean:
	@rm -f *.o
