include ../../../../Makefile-common

SHELL=/bin/sh
CLASS=W
NPROCS=1
SFILE=config/suite.def

DEBUGOBJ=../debug/msgDebug.o

INSTALLDIR = $(testdir)
INSTALLTARGETS = $(shell ls bin/*)

all: suite

default: header
	@ sys/print_instructions

BT: bt
bt: header
	cd BT; $(MAKE) NPROCS=$(NPROCS) CLASS=$(CLASS)

SP: sp
sp: header
	cd SP; $(MAKE) NPROCS=$(NPROCS) CLASS=$(CLASS)

LU: lu
lu: header
	cd LU; $(MAKE) NPROCS=$(NPROCS) CLASS=$(CLASS)

MG: mg
mg: header
	cd MG; $(MAKE) NPROCS=$(NPROCS) CLASS=$(CLASS)

FT: ft
ft: header
	cd FT; $(MAKE) NPROCS=$(NPROCS) CLASS=$(CLASS)

IS: is
is: header
	cd IS; $(MAKE) NPROCS=$(NPROCS) CLASS=$(CLASS)

CG: cg
cg: header
	cd CG; $(MAKE) NPROCS=$(NPROCS) CLASS=$(CLASS)

EP: ep
ep: header
	cd EP; $(MAKE) NPROCS=$(NPROCS) CLASS=$(CLASS)

# Awk script courtesy cmg@cray.com
suite:
	rm -rf bin
	mkdir bin 
	@ awk '{ if ($$1 !~ /^#/ &&  NF > 0)                              \
	printf "$(MAKE) %s CLASS=%s NPROCS=%d\n", $$1, $$2, $$3 }' $(SFILE)  \
	| $(SHELL)


# It would be nice to make clean in each subdirectory (the targets
# are defined) but on a really clean system this won't work
# because those makefiles need config/make.def
clean:
	- rm -rf bin
	- rm -f core 
	- rm -f binDU/*
	- rm -f *~ */core */*~ */*.o */npbparams.h */*.obj */*.exe
	- rm -f MPI_dummy/test MPI_dummy/libmpi.a
	- rm -f sys/setparams sys/makesuite sys/setparams.h

veryclean: clean
	- rm config/make.def config/suite.def Part*
	- rm bin/sp.* bin/lu.* bin/mg.* bin/ft.* bin/bt.* bin/is.* bin/ep.* bin/cg.*

header:
	@ sys/print_header

kit: 
	- makekit -s100k -k30 * */* */*/*

install: 
	@ if [ ! -d $(testdir) ]; then mkdir $(testdir) ; fi ;
	@ cp bin/* $(testdir)
	@ if [ ! -d $(miscdir) ]; then mkdir $(miscdir) ; fi ;
	@ cp config/suite.def $(miscdir)

