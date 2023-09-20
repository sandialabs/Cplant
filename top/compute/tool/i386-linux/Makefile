#
# $Id
#

include ../../../Makefile-common

CC         = cpcc
F77        = cpf77
COMPILERS  = $(CC) $(F77)

INSTALLDIR     = $(bindir)
INSTALLTARGETS = $(COMPILERS)

MPIDIR=mpich-1.2.0
LIBLIST="-lsrvr -lpuma -lgrp_io -lpuma -lenfs -lsrvr -lp30api -lp30myr -lp30lib"

# choices are gnu and gnu
CCOMPILER = gcc
C_REPEAT = "-lc"
C_STRIP = "-s"
C_STATIC = "-static"

FLIST = "-lg2c -lc -lgcc"
FCOMPILER = g77
F_STRIP = "-s"
F_STATIC = "-static"

define make-compiler
    sed -e s,DEFAULT_PATH,$(CPLANT_TOP),g \
	-e s,MPIDIR,$(MPIDIR),g \
	-e s,LIBLIST,$(LIBLIST),g \
	-e s,FLIST,$(FLIST),g \
	-e s,C_REPEAT,$(C_REPEAT),g \
	-e s,FCOMPILER,$(FCOMPILER),g \
	-e s,CCOMPILER,$(CCOMPILER),g \
	-e s,C_STRIP,$(C_STRIP),g \
	-e s,F_STRIP,$(F_STRIP),g \
	-e s,C_STATIC,$(C_STATIC),g \
	-e s,F_STATIC,$(F_STATIC),g \
	$< > $@
    chmod +x $@
    cp $@ ../../bin/$(CPLANT_ARCH)-$(OS)
endef

all: $(COMPILERS)

%: %.in
	$(make-compiler)

install uninstall: $(INSTALLTARGETS)
#	@$($@)

clean:
	-rm -f $(COMPILERS) 

realclean: clean

