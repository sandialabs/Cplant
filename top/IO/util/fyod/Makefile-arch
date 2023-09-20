#
# $Id: Makefile-arch,v 1.1 2001/10/29 22:45:51 pumatst Exp $
#
 
include ../../../../Makefile-common

EXECS          = fyod 

#TOPSOURCES     = $(shell ls ../*.c)
TOPSOURCES     = fyod.c fyodCompat.c fyod_comm.c fyod_fyod_comm.c
YODTOPSOURCES  = $(shell ls $(CPLANT_TOP)/service/util/yod/CMDhandlerTable.c)
YODSOURCES     = $(shell ls $(CPLANT_TOP)/service/util/yod/cmd_handlers_unistd/*.c)
YODUTILSOURCES = $(CPLANT_TOP)/service/util/yod/utlib/ioBufs.c \
                 $(CPLANT_TOP)/service/util/yod/utlib/iam.c 
YODCLIBSOURCES = $(shell ls $(CPLANT_TOP)/service/util/yod/cmd_handlers_clib/*.c)

TOPOBJECTS     = $(notdir $(TOPSOURCES:.c=.o))
YODTOPOBJECTS  = $(notdir $(YODTOPSOURCES:.c=.o))
YODOBJECTS     = $(notdir $(YODSOURCES:.c=.o))
YODUTILOBJECTS = $(notdir $(YODUTILSOURCES:.c=.o))
YODCLIBOBJECTS = $(notdir $(YODCLIBSOURCES:.c=.o))
COMMOBJECTS    = $(notdir $(COMMSOURCES:.c=.o))


ALLOBJECTS     = $(TOPOBJECTS) $(YODTOPOBJECTS) $(YODOBJECTS) \
                 $(YODUTILOBJECTS) $(YODCLIBOBJECTS) $(COMMOBJECTS) 

DEPS           = $(TOPOBJECTS:.o=.d) $(YODTOPOBJECTS:.o=d) $(YODOBJECTS:.o=.d) \
                 $(YODUTILOBJECTS:.o=.d) $(YODCLIBOBJECTS:.o=.d) \
                 $(COMMOBJECTS:.o=.d) 

ifeq ($(USE_DB),yes)
DBSOURCES      = $(CPLANT_TOP)/service/util/db/db.c
DBOBJECTS      = $(notdir $(DBSOURCES:.c=.o))
ALLOBJECTS    += $(DBOBJECTS)
DEPS          += $(DBOBJECTS:.o=.d)
endif

vpath %.c ..:../cmd_handlers_unistd:$(CPLANT_TOP)/service/util/yod/cmd_handlers_unistd:\
          $(CPLANT_TOP)/service/util/yod/utlib:$(CPLANT_TOP)/service/util/yod/cmd_handlers_clib:\
          $(CPLANT_TOP)/lib/comm

INCLUDEDIRS   = -I$(CPLANT_TOP)/IO/util/fyod \
                -I$(CPLANT_TOP)/include \
                -I$(CPLANT_TOP)/include/sys \
                -I$(CPLANT_TOP)/include/config \
                -I$(CPLANT_TOP)/include/portals \
                -I$(CPLANT_TOP)/include/load \
                -I$(CPLANT_TOP)/include/srvr \
                -I$(CPLANT_TOP)/include/IO \
                -I$(CPLANT_TOP)/compute/lib/p30/include \
                -I$(CPLANT_TOP)/compute/OS/linux/include \
                -I$(CPLANT_TOP)/service/util/yod/include
LIBDIRS       = -L$(CPLANT_TOP)/lib/puma/$(CPLANT_ARCH)-$(OS) \
                -L$(CPLANT_TOP)/lib/comm/$(CPLANT_ARCH)-$(OS) \
                -L$(CPLANT_TOP)/compute/lib/p30/lib/$(CPLANT_ARCH)-$(OS)
LIBS          = -lsrvr -lpuma -lp30api -lp30myr -lp30lib

ifeq ($(USE_DB),yes)
LIBDIRS += -L$(CPLANT_TOP)/service/util/db/$(CPLANT_ARCH)-$(OS) 
LIBS += -ldb
endif

LINKFLAGS     = $(STATICFLAG) 
STARTFILES    = $(CPLANT_TOP)/lib/start/$(CPLANT_ARCH)-$(OS)/startup_srvr.o

INSTALLDIR     = $(sbindir)
INSTALLTARGETS = $(EXECS)

all: $(EXECS)

ifeq ($(MAKE_DEPS),yes)
include $(DEPS)
endif

CFLAGS += -g

$(TOPOBJECTS) : %.o : ../%.c
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $<

$(YODTOPOBJECTS) : %.o : ../../../../service/util/yod/CMDhandlerTable.c
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $<

$(YODOBJECTS) : %.o : ../../../../service/util/yod/cmd_handlers_unistd/%.c
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $<

$(YODUTILOBJECTS) : %.o : ../../../../service/util/yod/utlib/%.c
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $<

$(YODCLIBOBJECTS) : %.o : ../../../../service/util/yod/cmd_handlers_clib/%.c
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $<

$(COMMOBJECTS) : %.o : ../../../../lib/comm/%.c
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $<

$(DBOBJECTS) : %.o : ../../../../service/util/db/%.c
	$(CC) $(CFLAGS) $(INCLUDEDIRS) -c $<
                 
$(EXECS): $(TOPOBJECTS) $(YODTOPOBJECTS) $(YODOBJECTS) $(YODUTILOBJECTS) \
          $(YODCLIBOBJECTS) $(COMMOBJECTS) $(DBOBJECTS)
	$(CC) $(LINKFLAGS) $(STARTFILES) -o $@ $(LIBDIRS) $^ $(LIBS)

dep: $(DEPS)

install uninstall: $(INSTALLTARGETS)
	@$($@)

clean:
	-rm -f $(EXECS) $(ALLOBJECTS)

print:
	@echo $(TOPSOURCES)
