# Generated automatically from Makefile.in by configure.

CPLANT_TOP  = ${HOME}/cplant/src/new/top

CC          = ${HOME}/cpcc
AR          = ar crl
LIBNAME     = ${CPLANT_TOP}/compute/lib/mpich-1.2.0/alpha-linux/libmpi.a
RANLIB      = ranlib
srcdir      = ${CPLANT_TOP}/compute/lib/mpich-1.2.0/src/romio/mpi-io

INCLUDE_DIR = -I${CPLANT_TOP}/compute/lib/mpich-1.2.0/include -I${srcdir} -I../include -I${srcdir}/../adio/include 
CFLAGS      =  -D__LINUX -O -DFORTRANUNDERSCORE -D__NO_AIO -DFORTRANDOUBLEUNDERSCORE -D__NFS -D__MPICH -D__NEEDS_MPI_TEST -D__INT_LT_POINTER=1 -D__HAVE_LONG_LONG_64=1 -D__HAVE_MPI_LONG_LONG_INT=1 -D__HAS_MPI_INFO=1 -D__HAS_MPI_DARRAY_SUBARRAY=1 -DHAVE_WEAK_SYMBOLS=1 -DHAVE_PRAGMA_WEAK=1  $(MPIOPROFILE) $(INCLUDE_DIR)

VPATH=.:$(srcdir)

MPIO_OBJECTS = close.o read.o open.o write.o set_view.o \
      iread.o iwrite.o iotest.o iowait.o seek.o get_posn.o \
      delete.o read_all.o read_at.o \
      read_atall.o iread_at.o iwrite_at.o write_all.o get_bytoff.o \
      write_at.o write_atall.o get_view.o get_group.o get_amode.o \
      get_extent.o fsync.o get_atom.o set_atom.o \
      set_size.o get_size.o prealloc.o file_f2c.o file_c2f.o \
      ioreq_f2c.o ioreq_c2f.o set_info.o get_info.o \
      rd_atallb.o rd_atalle.o read_allb.o read_alle.o wr_atallb.o \
      wr_atalle.o write_allb.o write_alle.o \
      get_posn_sh.o iread_sh.o read_sh.o write_sh.o \
      iwrite_sh.o seek_sh.o read_ord.o read_orde.o write_ordb.o \
      read_ordb.o write_ord.o write_orde.o

MPIO_TMP_POBJECTS = close.p read.p open.p write.p get_extent.p \
      iread.p iwrite.p iotest.p iowait.p seek.p \
      delete.p read_all.p read_at.p \
      read_atall.p iread_at.p iwrite_at.p get_posn.p \
      write_all.p write_at.p write_atall.p get_bytoff.p \
      set_view.p get_view.p get_group.p get_amode.p \
      fsync.p get_atom.p set_atom.p set_size.p get_size.p prealloc.p \
      file_f2c.p file_c2f.p ioreq_f2c.p ioreq_c2f.p set_info.p get_info.p \
      rd_atallb.p rd_atalle.p read_allb.p read_alle.p wr_atallb.p \
      wr_atalle.p write_allb.p write_alle.p \
      get_posn_sh.p iread_sh.p read_sh.p write_sh.p \
      iwrite_sh.p seek_sh.p read_ord.p read_orde.p write_ordb.p \
      read_ordb.p write_ord.p write_orde.p

MPIO_REAL_POBJECTS = _close.o _read.o _open.o _get_extent.o \
      _write.o _set_view.o _seek.o _read_at.o _prealloc.o \
      _iread.o _iwrite.o _iotest.o _iowait.o _get_posn.o \
      _delete.o _read_all.o _get_bytoff.o \
      _read_atall.o _iread_at.o _iwrite_at.o _get_group.o _get_amode.o \
      _write_all.o _write_at.o _write_atall.o _get_view.o \
      _fsync.o _get_atom.o _set_atom.o _set_size.o _get_size.o \
      _file_f2c.o _file_c2f.o _ioreq_f2c.o _ioreq_c2f.o \
      _set_info.o _get_info.o \
      _rd_atallb.o _rd_atalle.o _read_allb.o _read_alle.o _wr_atallb.o \
      _wr_atalle.o _write_allb.o _write_alle.o \
      _get_posn_sh.o _iread_sh.o _read_sh.o _write_sh.o \
      _iwrite_sh.o _seek_sh.o _read_ord.o _read_orde.o _write_ordb.o \
      _read_ordb.o _write_ord.o _write_orde.o

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
