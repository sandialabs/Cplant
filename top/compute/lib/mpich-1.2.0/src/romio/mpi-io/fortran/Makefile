# Generated automatically from Makefile.in by configure.

CPLANT_TOP  = ${HOME}/cplant/src/new/top
CC          = gcc
AR          = ar crl
RANLIB      = ranlib
LIBNAME     = ${CPLANT_TOP}/compute/lib/mpich-1.2.0/alpha-linux/libmpi.a
srcdir      = ${CPLANT_TOP}/compute/lib/mpich-1.2.0/src/romio/mpi-io/fortran

INCLUDE_DIR = -I${CPLANT_TOP}/compute/lib/mpich-1.2.0/include -I../../include -I${srcdir}/../../adio/include -I${srcdir}/..
CFLAGS      = -D__MPIO_FORTRAN_SRC   -D__NO_AIO -O -DFORTRANUNDERSCORE -D__NFS -D__NEEDS_MPI_TEST -D__MPICH  -D__INT_LT_POINTER=1 -D__HAVE_LONG_LONG_64=1 -D__HAVE_MPI_LONG_LONG_INT=1 -D__HAS_MPI_INFO=1 -D__HAS_MPI_DARRAY_SUBARRAY=1 -DHAVE_WEAK_SYMBOLS=1 -DHAVE_PRAGMA_WEAK=1 $(MPIOPROFILE) $(INCLUDE_DIR)

VPATH=.:$(srcdir)

MPIO_FOBJECTS = closef.o readf.o openf.o writef.o get_extentf.o \
      ireadf.o iwritef.o iotestf.o iowaitf.o seekf.o \
      deletef.o read_allf.o read_atf.o \
      read_atallf.o iread_atf.o iwrite_atf.o get_posnf.o \
      write_allf.o write_atf.o write_atallf.o get_bytofff.o \
      set_viewf.o get_viewf.o get_groupf.o get_amodef.o \
      fsyncf.o get_atomf.o set_atomf.o set_infof.o get_infof.o \
      set_sizef.o get_sizef.o preallocf.o \
      rd_atallbf.o rd_atallef.o read_allbf.o read_allef.o wr_atallbf.o \
      wr_atallef.o write_allbf.o write_allef.o \
      get_posn_shf.o iread_shf.o read_shf.o write_shf.o \
      iwrite_shf.o seek_shf.o read_ordf.o read_ordef.o write_ordbf.o \
      read_ordbf.o write_ordf.o write_ordef.o

MPIO_TMP_PFOBJECTS = closef.p readf.p openf.p writef.p get_extentf.p \
      ireadf.p iwritef.p iotestf.p iowaitf.p seekf.p \
      deletef.p read_allf.p read_atf.p \
      read_atallf.p iread_atf.p iwrite_atf.p get_posnf.p \
      write_allf.p write_atf.p write_atallf.p get_bytofff.p \
      set_viewf.p get_viewf.p get_groupf.p get_amodef.p \
      fsyncf.p get_atomf.p set_atomf.p set_infof.p get_infof.p \
      set_sizef.p get_sizef.p preallocf.p \
      rd_atallbf.p rd_atallef.p read_allbf.p read_allef.p wr_atallbf.p \
      wr_atallef.p write_allbf.p write_allef.p \
      get_posn_shf.p iread_shf.p read_shf.p write_shf.p \
      iwrite_shf.p seek_shf.p read_ordf.p read_ordef.p write_ordbf.p \
      read_ordbf.p write_ordf.p write_ordef.p

MPIO_REAL_PFOBJECTS = _closef.o _readf.o _openf.o _get_extentf.o \
      _writef.o _set_viewf.o _seekf.o _read_atf.o \
      _ireadf.o _iwritef.o _iotestf.o _iowaitf.o _get_posnf.o \
      _deletef.o _read_allf.o \
      _read_atallf.o _iread_atf.o _iwrite_atf.o _get_bytofff.o \
      _write_allf.o _write_atf.o _write_atallf.o _get_viewf.o \
      _get_groupf.o _get_amodef.o _fsyncf.o _get_atomf.o _set_atomf.o \
      _set_sizef.o _get_sizef.o _preallocf.o _set_infof.o _get_infof.o \
      _rd_atallbf.o _rd_atallef.o _read_allbf.o _read_allef.o _wr_atallbf.o \
      _wr_atallef.o _write_allbf.o _write_allef.o \
      _get_posn_shf.o _iread_shf.o _read_shf.o _write_shf.o \
      _iwrite_shf.o _seek_shf.o _read_ordf.o _read_ordef.o _write_ordbf.o \
      _read_ordbf.o _write_ordf.o _write_ordef.o

all: $(LIBNAME)

.SUFFIXES: $(SUFFIXES) .p

.c.o:
	$(CC) $(CFLAGS) -c $<

.c.p:
	@cp $(srcdir)/$*.c _$*.c
	$(CC) $(CFLAGS) -c _$*.c
	@rm -f _$*.c

profile: $(MPIO_TMP_PFOBJECTS)
	$(AR) $(LIBNAME) $(MPIO_REAL_PFOBJECTS)
	@rm -f $(srcdir)/_*.o

$(LIBNAME): $(MPIO_FOBJECTS)
	$(AR) $(LIBNAME) $(MPIO_FOBJECTS)

clean: 
	@rm -f *.o
