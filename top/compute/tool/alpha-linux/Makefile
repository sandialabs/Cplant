#
# $Id
#

# This makefile will build either compaq alpha-linux or
# gnu based CC and F77 compile scripts according to the
# value of $(LINUX_COMPILE) in makefile common. In the
# compaq case, the .in files are gotten from the "compile"
# directory, else gnu.in versions from ./ are used

include ../../../Makefile-common

COMPILE = ../../../compile

CC            = cpcc
F77           = cpf77
COMPILERS     = $(CC) $(F77)
COMPILERS_IN  = $(CC).in $(F77).in

INSTALLDIR     = $(bindir)
INSTALLTARGETS = $(COMPILERS)

HEAD = head_p3.in
LIBS = libs_p3.in

CCOMPILER = ccc
F77COMPILER = fort
F77MATHLIB = -lcpml
F90MATHLIB = -lcpml

CFLAGS = -std0

OPTIFLAGS = -fast

define make-gnu-compiler
    cat $(HEAD) $(COMPILE)/$(LIBS) $< $(COMPILE)/tail.in > temp
    sed -e 's,DEFAULT_PATH,$(CPLANT_TOP),g' temp > $@
    chmod +x $@
    cp $@ ../../bin/$(CPLANT_ARCH)-$(OS)
    rm temp
endef

define make-compiler
    cat $(HEAD) $(COMPILE)/flags.in $(COMPILE)/$(LIBS) $< $(COMPILE)/tail.in > temp
    sed -e 's,CCOMPILER,$(CCOMPILER),g' \
        -e 's,CFLAGS,$(CFLAGS),g' \
        -e 's,OPTIFLAGS,$(OPTIFLAGS),g' \
        -e 's,F77COMPILER,$(F77COMPILER),g' \
        -e 's,F77MATHLIB,$(F77MATHLIB),g' \
        -e 's,F90MATHLIB,$(F90MATHLIB),g' \
        -e 's,DEFAULT_PATH,$(CPLANT_TOP),g' temp > $@
    chmod +x $@
    cp $@ ../../bin/$(CPLANT_ARCH)-$(OS)
    rm temp
endef

all: $(CC) $(F77)

ifeq ($(LINUX_COMPILE),gnu)
%: %.gnu.in
	$(make-gnu-compiler)
else
%: %.in
	$(make-compiler)
endif

cpcc.in : ../../../compile/cc.in 
	cp $< $@

cpf77.in : ../../../compile/f77.in
	cp $< $@

install uninstall: $(INSTALLTARGETS)
#	@$($@)

clean:
	-rm -f $(COMPILERS) $(COMPILERS_IN)

realclean: clean

