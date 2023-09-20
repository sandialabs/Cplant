#
# $Id: Makefile,v 1.15 2001/12/14 17:43:38 pumatst Exp $
#

include ../Makefile-common

#Default location of cplant compilers
DISTTARGETDIR  = /usr/local/cplant

RELEASE        = current

SHELL          = /bin/sh

EXECS          = c++ cc f77 f90

DISTBINDIR     = $(bindir_bld)

allinstalldirs = $(DISTBINDIR)

STARTUPDIR     = $(CPLANT_TOP)/lib/start
STARTUP        = $(CPLANT_ARCH)-$(OS)/startup.o

PUMALIBDIR     = $(CPLANT_TOP)/lib/puma
PUMALIB        = $(CPLANT_ARCH)-$(OS)/libpuma.a

SERVERLIBDIR   = $(CPLANT_TOP)/lib/comm
SERVERLIB      = $(CPLANT_ARCH)-$(OS)/libsrvr.a

GROUPLIBSDIR   = $(CPLANT_TOP)/compute/lib/apps
GROUPIOLIB     = $(CPLANT_ARCH)-$(OS)/libgrp_io.a

GROUPCLIB      = $(CPLANT_ARCH)-$(OS)/libgrp_c.a

MPILIBDIR      = $(CPLANT_TOP)/compute/lib/mpich-1.2.0
MPILIB         = $(CPLANT_TOP)-$(OS)/libmpi.a
MPIHEADERS     = ../include/mpi.h ../include/binding.h ../include/mpi_errno.h ../include/mpif.h

#ifeq ($(OS),osf)
UFORLIBDIR     = $(CPLANT_TOP)/compute/lib/apps
UFORLIB        = $(CPLANT_ARCH)-$(OS)/libUfor.a
#endif

ifeq ($(OS),osf)
F77COMPILER = f77
F90COMPILER = f90
F77MATHLIB = -lm
F90MATHLIB = -lm
CCOMPILER   = cc
CCC_LINKS   = -lexc
else
F77COMPILER = fort
F90COMPILER = fort
F77MATHLIB = -lcpml
F90MATHLIB = -lcpml
CCOMPILER   = ccc
CCC_LINKS   = 
endif

CFLAGS = -std0

OPTIFLAGS =


INSTALLTARGETS = $(EXECS)
INSTALLDIR     = $(DISTBINDIR)

LIBSFILE = libs_p3.in

define make-compiler
    cat head.in flags.in $(LIBSFILE) $< tail.in > temp
    sed -e 's,DEFAULT_PATH,$(DISTTARGETDIR),g' \
        -e 's,RELEASE,$(RELEASE),g' \
        -e 's,F77COMPILER,$(F77COMPILER),g' \
        -e 's,F90COMPILER,$(F90COMPILER),g' \
        -e 's,F77MATHLIB,$(F77MATHLIB),g' \
        -e 's,F90MATHLIB,$(F90MATHLIB),g' \
        -e 's,CCOMPILER,$(CCOMPILER),g' \
        -e 's,CFLAGS,$(CFLAGS),g' \
        -e 's,CCC_LINKS,$(CCC_LINKS),g' \
        -e 's,OPTIFLAGS,$(OPTIFLAGS),g' \
        -e 's,OSNAME,$(OS_TYPE),g' temp > $@
    chmod +x $@
    rm temp
endef
 
all: $(EXECS)

%: %.in
	$(make-compiler)

install uninstall: $(EXECS)
	@$($@)

clean:
	-rm -f $(EXECS)
