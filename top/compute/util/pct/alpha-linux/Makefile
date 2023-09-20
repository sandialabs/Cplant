#
# $Id: Makefile-arch,v 1.2 2002/02/12 19:06:30 pumatst Exp $
#

include ../../../../Makefile-common
 
INCLUDEDIRS += -I..  -I$(CPLANT_TOP)/include/config/

ifeq ($(KERNEL_ADDR_CACHE),yes)
CFLAGS += -DKERNEL_ADDR_CACHE
endif

LINKFLAGS    = $(CSTATICFLAG) 
STARTFILES   = $(CPLANT_TOP)/lib/start/$(CPLANT_ARCH)-$(OS)/startup_srvr.o

ifeq ($(OS),linux)
#LINKFLAGS   += -nostartfiles
#STARTFILES  += /usr/lib/crt1.o /usr/lib/crti.o /usr/lib/crtbegin.o
endif

LIBDIRS      = -L$(CPLANT_TOP)/lib/puma/$(CPLANT_ARCH)-$(OS) \
               -L$(CPLANT_TOP)/lib/comm/$(CPLANT_ARCH)-$(OS) \
	       -L$(CPLANT_TOP)/compute/lib/p30/lib/$(CPLANT_ARCH)-$(OS)

LIBS         = -lsrvr -lpuma -lp30api -lp30myr -lp30lib


TOPSOURCES   = $(shell ls ../*.c)
LOADMSGSOURCE = $(CPLANT_TOP)/service/util/yod/load_msgs.c

TOPOBJECTS    = $(notdir $(TOPSOURCES:.c=.o))


LOADMSGOBJECT  = $(notdir $(LOADMSGSOURCE:.c=.o))

TOPDEPS      = $(TOPOBJECTS:.o=.d)
LOADMSGDEPS   = $(LOADMSGOBJECT:.o=.d)
DEPS         = $(TOPDEPS) $(LOADMSGDEPS)

EXECS        = pct

INSTALLDIR     = $(sbindir)
INSTALLTARGETS = $(EXECS)

vpath %.c ..:$(CPLANT_TOP)/service/util/bebopd:$(CPLANT_TOP)/service/util/yod

all: $(EXECS)

ifeq ($(MAKE_DEPS),yes)
include $(DEPS)
endif

$(TOPOBJECTS): %.o : ../%.c ../pct.h ../pct_health.h
	$(CC) -g $(CFLAGS) $(INCLUDEDIRS) -c $<

$(LOADMSGOBJECT): %.o : ../../../service/util/yod/%.c
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $<

$(EXECS): $(TOPOBJECTS) $(LOADMSGOBJECT)
	$(CC) -g $(LINKFLAGS) $(STARTFILES) -o $@ $(LIBDIRS) $^ $(LIBS)

install uninstall: $(INSTALLTARGETS)
	@$($@)

dep: $(DEPS)

clean:
	-rm -f $(TOPOBJECTS) $(EXECS) $(LOADMSGOBJECT) $(DEPS)

