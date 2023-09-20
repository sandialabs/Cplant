#
# $Id: Makefile-arch,v 1.1 2001/10/29 22:39:36 pumatst Exp $
#

include ../../../../Makefile-common

CSOURCES      = start-tvdsvr.c
COBJECTS      = $(CSOURCES:.c=.o)

EXECS         = start-tvdsvr 

LIBDIR        = -L$(CPLANT_TOP)/lib/puma/$(CPLANT_ARCH)-$(OS) \
                -L$(CPLANT_TOP)/lib/comm/$(CPLANT_ARCH)-$(OS) \
                -L$(CPLANT_TOP)/compute/lib/p30/lib/$(CPLANT_ARCH)-$(OS)
LIBS          = -lsrvr -lpuma -lpuma -lp30api -lp30myr -lp30lib

STARTFILES    = $(CPLANT_TOP)/lib/start/$(CPLANT_ARCH)-$(OS)/startup_srvr.o

ifeq ($(OS),osf)
INCLUDEDIRS   += -I$(CPLANT_TOP)/service/include
LIBDIR       += -L$(CPLANT_TOP)/service/lib/getopt
LIBS         += -lgetopt
endif

ifeq ($(OS),linux)
#LINKFLAGS    = -nostartfiles 
#STARTFILES   += /usr/lib/crt1.o /usr/lib/crti.o /usr/lib/crtbegin.o
LINKFLAGS    = -static
endif

INSTALLDIR     = $(sbindir)
INSTALLTARGETS = $(EXECS)

vpath %.c ..

all: $(EXECS)

ifeq ($(MAKE_DEPS),yes)
include $(DEPS)
endif

%.o: %.c
	$(CC) -Wall $(CFLAGS) -g $(INCLUDEDIRS) -c $<

start-tvdsvr: % : %.o $(COBJECTS)
	$(CC) $(LINKFLAGS) $(LIBDIR) -o $@ $^ $(STARTFILES) $(LIBS)

install uninstall: $(INSTALLTARGETS)
	@$($@)

clean:
	rm -f $(EXECS) $(COBJECTS)

