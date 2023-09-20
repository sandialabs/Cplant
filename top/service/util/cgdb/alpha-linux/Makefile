#
# $Id: Makefile-arch,v 1.1 2001/10/29 22:34:20 pumatst Exp $
#

include ../../../../Makefile-common

EXECS         = cgdb

TOPSOURCES    = $(shell ls ../*.c)
#CLIBSOURCES   = $(filter-out ../../yod/cmd_handlers_clib/CMDhandler_exit.c, \
#                $(shell ls ../../yod/cmd_handlers_clib/*.c))
#UNISTDSOURCES = $(shell ls ../../yod/cmd_handlers_unistd/*.c)
#UTLIBSOURCES  = $(shell ls ../../yod/utlib/*.c)
 
TOPOBJECTS    = $(notdir $(TOPSOURCES:.c=.o))
#CLIBOBJECTS   = $(notdir $(CLIBSOURCES:.c=.o))
#UNISTDOBJECTS = $(notdir $(UNISTDSOURCES:.c=.o))
#UTLIBOBJECTS  = $(notdir $(UTLIBSOURCES:.c=.o))
#PRIVOBJECTS   = yod_site_priv.o

ALLOBJECTS   += $(TOPOBJECTS) $(CLIBOBJECTS) $(UNISTDOBJECTS) $(UTLIBOBJECTS)
#ALLOBJECTS   += $(PRIVOBJECTS)

TOPDEPS       = $(TOPOBJECTS:.o=.d)
#CLIBDEPS      = $(CLIBOBJECTS:.o=.d)
#UNISTDDEPS    = $(UNISTDOBJECTS:.o=.d)
#UTILDEPS      = $(UTILOBJECTS:.o=.d)

DEPS         += $(TOPDEPS) $(CLIBDEPS) $(UNISTDDEPS) $(UTILDEPS)
 
vpath %.c ..:../cmd_handlers_clib:../cmd_handlers_unistd:../utlib
 
INCLUDEDIRS    += -I$(CPLANT_TOP)/service/util/yod/include \
                -I$(CPLANT_TOP)/service/util/bebopd

LIBDIR        = -L$(CPLANT_TOP)/lib/puma/$(CPLANT_ARCH)-$(OS) \
                -L$(CPLANT_TOP)/lib/comm/$(CPLANT_ARCH)-$(OS) \
                -L$(CPLANT_TOP)/compute/lib/p30/lib/$(CPLANT_ARCH)-$(OS)

LIBS          = -lsrvr -lpuma -lp30api -lp30myr -lp30lib

#LINKFLAGS     = $(STATICFLAG) 
STARTFILES    = $(CPLANT_TOP)/lib/start/$(CPLANT_ARCH)-$(OS)/startup_srvr.o

ifeq ($(OS),linux)
#LINKFLAGS    = -nostartfiles
#STARTFILES   += /usr/lib/crt1.o /usr/lib/crti.o /usr/lib/crtbegin.o
LINKFLAGS    = 
endif

ifeq ($(OS),osf)
LIBDIR       += -L$(CPLANT_TOP)/service/lib/getopt
LIBS         += -lgetopt
endif
 
INSTALLDIR     = $(bindir_sys)
INSTALLTARGETS = $(EXECS)
 
all: $(EXECS)

ifeq ($(MAKE_DEPS),yes)
include $(DEPS)
endif

$(TOPOBJECTS) : %.o : ../%.c
	$(CC) -g $(CFLAGS) $(INCLUDEDIRS) -c $<
 
$(CLIBOBJECTS) : %.o : ../cmd_handlers_clib/%.c
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $<
 
$(UNISTDOBJECTS) : %.o : ../cmd_handlers_unistd/%.c
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $<
 
$(UTLIBOBJECTS) : %.o : ../utlib/%.c
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $<
 
cgdb: $(TOPOBJECTS) $(CLIBOBJECTS) $(UNISTDOBJECTS) $(UTLIBOBJECTS)
	$(CC) $(LINKFLAGS) $(STARTFILES) $(LIBDIR) -o $@ $^ $(LIBS)
 
$(PRIVOBJECTS) : %.o : ../%.c
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $<
 
install uninstall: $(INSTALLTARGETS)
	@$($@)

dep: $(DEPS)
 
clean:
	-rm -f $(EXECS) $(ALLOBJECTS) $(DEPS)
 
