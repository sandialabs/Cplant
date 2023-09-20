#
# $Id: Makefile-arch,v 1.4 2002/02/12 19:02:20 pumatst Exp $
#

include ../../../../Makefile-common

CSOURCES            = load_msgs.c
COBJECTS            = $(CSOURCES:.c=.o)

BEBOPDSOURCES      = bebopd_alloc.c bebopd_pbs.c bebopd_log.c bebopd_smart_alloc.c
BEBOPDOBJECTS      = $(BEBOPDSOURCES:.c=.o)

EXECSOURCES      = bebopd.c pingd.c 
EXECOBJECTS      = $(EXECSOURCES:.c=.o)


EXECS         = $(EXECOBJECTS:.o=)
DEPS          = $(BEBOPDOBJECTS:.o=.d) $(COBJECTS:.o=.d) $(EXECOBJECTS:.o=.d)

LIBDIR        = -L$(CPLANT_TOP)/lib/puma/$(CPLANT_ARCH)-$(OS) \
                -L$(CPLANT_TOP)/lib/comm/$(CPLANT_ARCH)-$(OS) \
                -L$(CPLANT_TOP)/compute/lib/p30/lib/$(CPLANT_ARCH)-$(OS) 

LIBS          = -lsrvr -lpuma -lpuma -lp30api -lp30myr -lp30lib

ifeq ($(RELIABLE),yes)
CFLAGS += -DRELIABLE
endif

#LINKFLAGS     = $(STATICFLAG) 
STARTFILES    = $(CPLANT_TOP)/lib/start/$(CPLANT_ARCH)-$(OS)/startup_srvr.o

ifeq ($(OS),osf)
INCLUDEDIRS   += -I$(CPLANT_TOP)/service/include
LIBDIR       += -L$(CPLANT_TOP)/service/lib/getopt
LIBS         += -lgetopt
endif

ifeq ($(OS),linux)
#LINKFLAGS    = -nostartfiles 
#STARTFILES   += /usr/lib/crt1.o /usr/lib/crti.o /usr/lib/crtbegin.o
LINKFLAGS    = 
endif

INSTALLDIR     = $(sbindir)
INSTALLTARGETS = $(EXECS)

vpath %.c ..:../../yod

all: $(EXECS)

ifeq ($(MAKE_DEPS),yes)
include $(DEPS)
endif

%.o: %.c
	$(CC) -Wall $(CFLAGS) -g $(INCLUDEDIRS) -c $<

bebopd: % : %.o $(BEBOPDOBJECTS) $(COBJECTS)
	$(CC) $(LINKFLAGS) $(LIBDIR) -o $@ $^ $(STARTFILES) $(LIBS)

pingd: % : %.o $(COBJECTS)
	$(CC) $(LINKFLAGS) $(LIBDIR) -o $@ $^ $(STARTFILES) $(LIBS)

install uninstall: $(INSTALLTARGETS)
	@$($@)

clean:
	rm -f $(EXECS) $(EXECOBJECTS) $(COBJECTS) $(BEBOPDOBJECTS) $(DEPS) 

