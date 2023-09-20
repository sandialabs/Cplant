
# $Id: Makefile-arch,v 1.3 2001/11/20 23:24:03 pumatst Exp $
#

include ../../../../../Makefile-common

CC      = $(CPCC)
FC      = $(CPF77)

EXECSOURCE = $(shell ls ../*.c)
FEXECSOURCE  = $(shell ls ../*.f)

EXECS = $(notdir $(EXECSOURCE:.c=))
FEXECS = $(notdir $(FEXECSOURCE:.f=))


INSTALLDIR     = $(testdir)
INSTALLTARGETS = $(EXECS) $(FEXECS)

INCLUDEDIRS = -I$(CPLANT_TOP)/compute/lib/apps \
              -I$(CPLANT_TOP)/include/srvr \
              -I$(CPLANT_TOP)/compute/lib/p30/include
vpath %.f ..
vpath %.c ..

all: $(EXECS) $(FEXECS)

$(EXECS): % : %.c
	$(CC) -g $(INCLUDEDIRS) -o $@ $<

$(FEXECS): % : %.f
	$(FC) -g -o $@ $<

install uninstall: $(INSTALLTARGETS)
	@$($@)

clean:
	-rm -f $(EXECS) $(FEXECS)
