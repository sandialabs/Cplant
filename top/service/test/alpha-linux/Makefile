#
# $Id: Makefile-arch,v 1.3 2001/11/17 01:20:17 lafisk Exp $
#

include ../../../Makefile-common

SOURCES     = $(shell ls ../[!yod2]*.c )
COMPUTESOURCES    = $(CPLANT_TOP)/compute/test/current/general/ts_srvr_put.c \
             $(CPLANT_TOP)/compute/test/current/general/ts_srvr_get.c \
             $(CPLANT_TOP)/compute/test/current/general/ts_srvr_ctl.c 


EXECS              = $(notdir $(SOURCES:.c=))

COMPUTEEXECS       = $(notdir $(COMPUTESOURCES:.c=))

YODDIR      = $(CPLANT_TOP)/service/util/yod

YOD2SOURCES = $(shell ls ../yod2*.c )
YOD2OBJECTS = $(notdir $(YOD2SOURCES:.c=.o))

LIBDIR        = -L$(CPLANT_TOP)/lib/puma/$(CPLANT_ARCH)-$(OS) \
                -L$(CPLANT_TOP)/lib/comm/$(CPLANT_ARCH)-$(OS) \
                -L$(CPLANT_TOP)/service/lib/job/$(CPLANT_ARCH)-$(OS) \
                -L$(CPLANT_TOP)/compute/lib/p30/lib/$(CPLANT_ARCH)-$(OS) 

INCLUDES = -I$(CPLANT_TOP)/include\
              -I$(CPLANT_TOP)/include/config\
              -I$(CPLANT_TOP)/include/sys\
              -I$(CPLANT_TOP)/include/portals\
              -I$(CPLANT_TOP)/include/IO\
              -I$(CPLANT_TOP)/include/load\
              -I$(CPLANT_TOP)/include/srvr \
              -I$(CPLANT_TOP)/compute/lib/p30/include \
              -I$(CPLANT_TOP)/service/lib/job \
              -I$(CPLANT_TOP)/compute/OS/linux/include

INSTALLDIR = $(srvrtestdir)
INSTALLTARGETS = $(EXECS) $(COMPUTEEXECS)

LIBS          = -ljob -lsrvr -lpuma -ljob -lsrvr -lpuma -lp30api -lp30myr -lp30lib
 
ifeq ($(MAKE_DEPS),yes)
include $(DEPS)
endif

CFLAGS += -g
 
LINKFLAGS     = $(CSTATICFLAG)
STARTFILES    = $(CPLANT_TOP)/lib/start/$(CPLANT_ARCH)-$(OS)/startup_srvr.o
 
all: yod2 $(EXECS) $(COMPUTEEXECS)

#
#  groupTest doesn't use our startup.o
#
groupTest.o:  ../groupTest.c
	gcc $(CFLAGS) -static $(INCLUDES) -o groupTest ../groupTest.c

#
# sources in this directory 
#
$(EXECS): % : ../%.c 
	$(CC) $(CFLAGS) $(LINKFLAGS) $(LIBDIR) $(INCLUDES) $(STARTFILES) -o $@ $^ $(LIBS)

#
# yod2
#
$(YOD2OBJECTS): %.o : ../%.c
	gcc -c $(CFLAGS) $(INCLUDES) -o $@ $^

yod2: % : $(YOD2OBJECTS)
	$(CC) $(CFLAGS) $(LINKFLAGS) $(LIBDIR) $(INCLUDES) $(STARTFILES) -o $@ $^ $(LIBS)
#
# sources in the compute/test directory - because they can
#    also be built as Cplant apps
#
$(COMPUTEEXECS): % : $(CPLANT_TOP)/compute/test/current/general/%.c 
	$(CC) $(CFLAGS) $(LINKFLAGS) $(LIBDIR) $(INCLUDES) $(STARTFILES) -o $@ $^ $(LIBS)

install uninstall: $(INSTALLTARGETS)
	@$($@)

clean:
	-rm -f $(EXECS) $(COMPUTEEXECS) $(YOD2OBJECTS) yod2 groupTest
