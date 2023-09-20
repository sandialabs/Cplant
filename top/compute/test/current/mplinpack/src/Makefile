include ../../../../../Makefile-common
#
# MPLinpack makefile
#

TOPDIR         = $(CPLANT_TOP)/compute/test/current/mplinpack
SRCDIR         = $(TOPDIR)/mplinpack
ECHO           = /usr/bin/echo

#CC             = /Net/mp/cplant/new/bin/cc
CC             = $(CPLANT_TOP)/compute/bin/$(CPLANT_ARCH)-$(OS)/cpcc -g
OPTFLAGS       = -O2
INCLUDES       = -I$(TOPDIR)/include
DEFINES        = -DCPLANT -DLINUX_PORTALS -DMPI -DPOINTERS_64_BITS
CFLAGS         = $(OPTFLAGS) $(DEFINES) $(INCLUDES)

CSOURCES       = solve.c init.c time.c malloc.c pcomm.c factor.c mat.c
COBJECTS       = $(CSOURCES:.c=.o)

#
# BLAS libraries
#

LIBGEMMDIR     = $(TOPDIR)/gemm
LIBGEMM        = $(LIBGEMMDIR)/libgemm.a

LIBAXPYDIR     = $(TOPDIR)/axpy
LIBAXPY        = $(LIBAXPYDIR)/libaxpy.a

LIBCOPYDIR     = $(TOPDIR)/copy
LIBCOPY        = $(LIBCOPYDIR)/libcopy.a

LIBDOTDIR      = $(TOPDIR)/dot
LIBDOT         = $(LIBDOTDIR)/libdot.a

LIBBLASDIR     = /usr/lib
ifeq ($(OS),linux)
LIBBLAS        = $(LIBBLASDIR)/libcxml.a
else
LIBBLAS        = $(LIBBLASDIR)/libdxml.a
endif

#
# Fortran libraries
#
FLIBS           = -lfor -lFutil -lots -lcpml

#
# mpi library
#
LIBMPI         = -lmpi

LIBDIRS        = $(LIBGEMMDIR) $(LIBAXPYDIR) $(LIBCOPYDIR) $(LIBDOTDIR)
LIBS           = $(LIBGEMM) $(LIBAXPY) $(LIBCOPY) $(LIBDOT) $(LIBBLAS)

#
# plain double precision version
#

all: mplinpack

mplinpack: $(COBJECTS) $(LIBS) Makefile
	$(CC) $(CFLAGS) -o $@ $(COBJECTS) $(LIBS) -lmpi $(FLIBS) -lm

$(COBJECTS): %.o : %.c
	$(CC) $(CFLAGS) -c $<
 
clean:
	rm -f *.o */*.o mplinpack

realclean: clean

install: mplinpack
	cp mplinpack $(testdir)

